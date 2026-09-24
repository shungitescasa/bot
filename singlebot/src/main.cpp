#include <algorithm>
#include <chrono>
#include <exception>
#include <format>
#include <memory>
#include <optional>
#include <regex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "core/bot.hpp"
#include "core/builtin.hpp"
#include "core/command.hpp"
#include "core/config.hpp"
#include "core/data/database.hpp"
#include "core/data/event.hpp"
#include "core/externalapi/twitch.hpp"
#include "core/irc/bot.hpp"
#include "core/log.hpp"
#include "core/message.hpp"
#include "core/rss.hpp"
#include "core/utils.hpp"
#include "scriptvm/builtin.hpp"
#include "scriptvm/client.hpp"

bot::Response run_command(
    bot::CommandDataVec &all_commands, bot::CommandLoader &command_loader,
    scriptvm::RPCClient &script_vm, const bot::Requester &requester,
    const bot::Message<bot::MessageType::ChatMessage> message) {
  std::optional<bot::Request> request =
      bot::Request::create(all_commands, message, requester);

  bot::Response response;

  if (request.has_value()) {
    if (command_loader.has(request->command_id)) {
      response = command_loader.run(*request);
    } else {
      response = script_vm.execute(*request);
    }
  }

  return response;
}

std::optional<std::string> handle_custom_commands(
    bot::CommandDataVec &all_commands, bot::CommandLoader &command_loader,
    scriptvm::RPCClient &script_vm,
    const bot::Message<bot::MessageType::ChatMessage> &irc_message,
    const bot::Requester &requester) {
  std::vector<std::string> parts =
      bot::utils::string::split_and_collect(irc_message.contents, ' ');

  if (parts.empty()) {
    return std::nullopt;
  }

  std::string prefix = requester.room_preferences.prefix;
  std::string cid = parts[0];
  std::string cid_without_prefix = cid;

  if (cid.size() > prefix.size() && cid.substr(0, prefix.size()) == prefix) {
    cid_without_prefix = "{prefix}" + cid.substr(prefix.size(), cid.size());
  }

  bot::data::DatabaseConnection conn = bot::data::create_connection();
  bot::data::DatabaseRows cmds = conn->exec(
      "SELECT cc.name, cc.message, cca.name AS alias_name FROM "
      "custom_commands cc "
      "LEFT JOIN custom_command_aliases cca ON cca.command_id = cc.id "
      "WHERE (cc.name = $1 OR cc.name = $2 "
      "OR cca.name = $3 OR cca.name = $4) "
      "AND (cc.room_id "
      "= $5 OR cc.is_global = TRUE)",
      {cid, cid_without_prefix, cid, cid_without_prefix,
       std::to_string(requester.room.id)});

  if (cmds.empty()) {
    return std::nullopt;
  }

  bot::data::DatabaseRow cmd = cmds[0];
  std::string cmd_name =
      cmd.at("alias_name").empty() ? cmd.at("name") : cmd.at("alias_name");
  if (cmd_name.length() > 8 && cmd_name.substr(0, 8) == "{prefix}" &&
      cmd_name.substr(0, requester.room_preferences.prefix.size()) !=
          requester.room_preferences.prefix) {
    return std::nullopt;
  }

  parts.erase(parts.begin());

  std::string initial_message = bot::utils::string::join(parts, " ");

  std::string msg = cmd.at("message");

  // parsing values
  std::regex pattern(R"(\{([^}]*)\})");
  std::string output;
  std::sregex_iterator iter(msg.begin(), msg.end(), pattern);
  std::sregex_iterator end;

  int last_pos = 0;
  for (; iter != end; ++iter) {
    auto m = *iter;
    output += msg.substr(last_pos, m.position() - last_pos);

    std::string inside = m[1].str();

    int placeholder_pos = inside.find("$1");
    if (placeholder_pos != std::string::npos) {
      inside.replace(placeholder_pos, 3, initial_message);
    }

    bot::Message<bot::MessageType::ChatMessage> m2 = irc_message;
    m2.contents = requester.room_preferences.prefix + inside;

    bot::Response response =
        run_command(all_commands, command_loader, script_vm, requester, m2);

    std::string answer = "[N/A]";

    if (response.is_single()) {
      answer = response.get_single();
    } else if (response.is_multiple()) {
      answer = "[multiple strings]";
    }

    output += answer;

    last_pos = m.position() + m.length();
  }

  output += msg.substr(last_pos);

  return output;
}

void check_timers(std::shared_ptr<bot::irc::IRCChatBot> chatbot) {
  bot::Logger logger("Timer-thread");

  while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(5));

    try {
      std::unique_ptr<bot::data::BaseDatabase> conn =
          bot::data::create_connection();

      bot::data::DatabaseRows timers = conn->exec(
          "SELECT t.id, t.message, r.name AS room_name, r.alias_id AS "
          "room_alias_id "
          "FROM timers t "
          "INNER JOIN rooms r ON r.id = t.room_id "
          "INNER JOIN room_preferences rp ON rp.id = r.id "
          "WHERE t.last_executed_at + (t.interval * INTERVAL '1 second') <= "
          "CURRENT_TIMESTAMP AND r.parted_at IS NULL AND rp.silent_mode = "
          "FALSE");

      if (timers.empty()) {
        continue;
      }

      logger.info(std::format("Running {} timers...", timers.size()));

      for (const auto &timer : timers) {
        chatbot->send_message(
            {timer.at("room_name"), std::stoi(timer.at("room_alias_id"))},
            timer.at("message"));

        conn->exec(
            "UPDATE timers SET last_executed_at = CURRENT_TIMESTAMP WHERE id = "
            "$1",
            {timer.at("id")});
      }
    } catch (std::exception &e) {
      logger.exception(e);
    }
  }
}

void join_anonymous_rooms(std::shared_ptr<bot::irc::IRCChatBot> bot) {
  bot::Logger log("join_anonymous_rooms");

  while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(5));

    try {
      bot::data::DatabaseConnection conn = bot::data::create_connection();

      bot::data::DatabaseRows rows = conn->exec(
          "SELECT r.name FROM rooms r "
          "UNION "
          "SELECT e.name FROM events e "
          "WHERE e.event_type IN ('twitch.first-message', 'twitch.message')");

      int i = 0;

      for (bot::data::DatabaseRow row : rows) {
        std::string name = row.at("name");
        if (bot->has_already_joined({name})) continue;

        if (i >= 5) {
          std::this_thread::sleep_for(std::chrono::seconds(30));
          i = 0;
        }

        bot->join({name});

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        i++;
      }
    } catch (std::exception &e) {
      log.exception(e);
    }
  }
}

int main(int argc, char *argv[]) {
  bot::Logger log("Main");
  log.info("Starting up...");

  bot::Configuration &cfg = bot::Configuration::get_instance();
  cfg.load_from_args(argc, argv);

  if (cfg.irc.host.empty() || cfg.irc.port.empty() || cfg.irc.nick.empty() ||
      cfg.irc.pass.empty()) {
    throw std::runtime_error(
        "irc.host, irc.port, irc.nick, irc.pass must be set for IRC chatbot");
  }

  std::shared_ptr<bot::irc::IRCChatBot> chatbot =
      std::make_shared<bot::irc::IRCChatBot>(cfg.irc.host, cfg.irc.port,
                                             cfg.irc.nick, cfg.irc.pass);

  bot::externalapi::twitch::HelixClient &twitch_api =
      bot::externalapi::twitch::HelixClient::get_instance();
  twitch_api.set_token(cfg.twitch.token);

  bot::RPCChatBotServer rpc_server(chatbot, cfg.rpc.client_port);

  bot::CommandLoader command_loader;
  command_loader.add(std::make_unique<bot::builtin::PingCommand>(chatbot));
  command_loader.add(
      std::make_unique<scriptvm::builtin::ScriptExecutionCommand>());
  command_loader.add(
      std::make_unique<scriptvm::builtin::ScriptRemoteCommand>());

  scriptvm::RPCClient &script_vm = scriptvm::RPCClient::get_instance();
  script_vm.connect(cfg.rpc.host, cfg.rpc.port);

  chatbot->on_connect([&]() {
    log.info("Connected!");

    // wait for NICK announce
    std::this_thread::sleep_for(std::chrono::seconds(1));

    chatbot->join(chatbot->get_me());

    bot::data::DatabaseConnection conn = bot::data::create_connection();

    // updating room names
    bot::data::DatabaseRows rows = conn->exec(
        "SELECT id, name, alias_id FROM rooms WHERE alias_id IS NOT NULL AND "
        "parted_at IS NULL");

    std::vector<int> ids;

    for (bot::data::DatabaseRow row : rows) {
      ids.push_back(std::stoi(row.at("alias_id")));
    }

    if (!ids.empty()) {
      std::vector<bot::externalapi::twitch::User> users =
          twitch_api.get_users(ids);

      for (bot::data::DatabaseRow row : rows) {
        std::string id = row.at("id");
        int alias_id = std::stoi(row.at("alias_id"));
        auto u = std::find_if(
            users.begin(), users.end(),
            [&row, &alias_id](const auto &x) { return x.id == alias_id; });

        if (u == users.end()) {
          log.info(
              std::format("External room {} (ID {}) is not found! Parting...",
                          alias_id, id));
          conn->exec(
              "UPDATE rooms SET parted_at = CURRENT_TIMESTAMP WHERE id = $1",
              {id});
          continue;
        } else if (u->login == row.at("name")) {
          continue;
        }

        log.info(std::format("External room {} is now named as {} (ID {})",
                             alias_id, u->login, id));
        conn->exec("UPDATE rooms SET name = $1 WHERE id = $2", {u->login, id});
      }
    }

    // joining rooms
    rows =
        conn->exec("SELECT name, alias_id FROM rooms WHERE parted_at IS NULL");

    int i = 0;

    for (bot::data::DatabaseRow row : rows) {
      if (i >= 5) {
        log.info("Initial JOIN cooldown... (30 seconds)");
        std::this_thread::sleep_for(std::chrono::seconds(30));
        i = 0;
      }

      std::string name = row.at("name");
      int id = row.contains("alias_id") ? std::stoi(row.at("alias_id")) : -1;
      chatbot->join({name, id});

      std::this_thread::sleep_for(std::chrono::milliseconds(500));
      i++;
    }
  });

  chatbot->on_chat_message(
      [&](bot::Message<bot::MessageType::ChatMessage> message) {
        log.debug(std::format("{} <{}>: {}", message.source.unnormalize(),
                              message.sender.login, message.contents));

        try {
          bot::data::DatabaseConnection conn = bot::data::create_connection();
          bot::Requester requester{message, conn};
          if (requester.room.parted_at.has_value() ||
              requester.sender.parted_at.has_value() ||
              requester.room_preferences.silent_mode)
            return;

          if (!script_vm.is_alive()) {
            log.info("scriptVM RPC server is not alive! Reconnecting...");
            script_vm.connect();
          }

          // combining commands
          bot::CommandDataVec remote_commands = script_vm.list();

          bot::CommandDataVec all_commands;
          all_commands.reserve(remote_commands.size() +
                               command_loader.get_commands().size());

          for (const bot::CommandBox &c : command_loader.get_commands())
            all_commands.push_back(c->data());

          all_commands.insert(all_commands.end(), remote_commands.begin(),
                              remote_commands.end());

          // getting response
          bot::Response response = run_command(all_commands, command_loader,
                                               script_vm, requester, message);

          if (response.is_single()) {
            chatbot->send_message(message.source, response.get_single());
            return;
          } else if (response.is_multiple()) {
            for (const std::string &text : response.get_multiple()) {
              chatbot->send_message(message.source, text);
              std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
            return;
          }

          // handling custom commands
          std::optional<std::string> custom_response = handle_custom_commands(
              all_commands, command_loader, script_vm, message, requester);
          if (custom_response.has_value()) {
            chatbot->send_message(message.source, custom_response.value());
            return;
          }
        } catch (std::exception &e) {
          log.exception(e);
        }
      });

  bot::RSSEventRepository event_repository;
  event_repository.on_event([log, chatbot](
                                const std::string &type,
                                const std::string &name,
                                const std::vector<bot::RSSItem> &items) {
    std::vector<bot::data::Event> events = bot::data::get_events(type, name);
    constexpr int max_events_per_announce = 5;

    for (bot::data::Event event : events) {
      int announcement_counter = 0;
      for (bot::RSSItem item : items) {
        if (announcement_counter >= max_events_per_announce) break;

        std::string event_msg = event.create_event_message(type, name, item);

        std::vector<std::string> user_lines =
            bot::utils::string::separate_by_length(event_msg, event.subs, "",
                                                   " ", 400);

        for (const std::string &line : user_lines) {
          chatbot->send_message(event.room_name, event_msg + line);
        }
      }

      if (items.size() >= max_events_per_announce) {
        chatbot->send_message(
            event.room_name,
            std::format("...and {} more announcements",
                        items.size() - max_events_per_announce));
      }
    }
  });

  std::vector<std::thread> threads;
  std::shared_ptr<bot::irc::IRCChatBot> anon;

  // anonymous chatbot
  if (!cfg.anonirc.host.empty()) {
    anon = std::make_shared<bot::irc::IRCChatBot>(
        cfg.anonirc.host, cfg.anonirc.port, cfg.anonirc.nick, cfg.anonirc.pass);

    anon->on_connect([&]() { log.info("[[ANONYMOUS]] Connected!"); });

    anon->on_chat_message(
        [&](bot::Message<bot::MessageType::ChatMessage> message) {
          log.debug(std::format("[[ANONYMOUS]] {} <{}>: {}",
                                message.source.unnormalize(),
                                message.sender.login, message.contents));

          try {
            std::vector<bot::data::Event> events;
            std::string prefix = "", room = message.source.normalize();

            // handling first messages
            if (message.sender.is_first_message) {
              events = bot::data::get_events("twitch.first-message", room);
              prefix = "🙋";
            } else if (message.sender.login == message.source.login) {
              events =
                  bot::data::get_events("twitch.message", message.sender.login);
              prefix = "🗨️";
            }

            for (bot::data::Event event : events) {
              std::string base = prefix + " " + event.message;
              if (!event.subs.empty()) {
                base.append(" · ");
              }

              int pos = base.find("{origin}");
              if (pos != std::string::npos) base.replace(pos, 8, room);

              pos = base.find("{message}");
              if (pos != std::string::npos)
                base.replace(pos, 9, message.contents);

              pos = base.find("{author}");
              if (pos != std::string::npos)
                base.replace(pos, 8, message.sender.login);

              std::vector<std::string> lines =
                  bot::utils::string::separate_by_length(base, event.subs, "",
                                                         " ", 500);

              for (const std::string &line : lines) {
                chatbot->send_message({event.room_name, event.room_alias_id},
                                      base + line);
              }
            }
          } catch (std::runtime_error &e) {
            log.exception(e);
          }
        });

    threads.push_back(std::thread(&bot::ChatBot::connect, anon));
    threads.push_back(std::thread(&join_anonymous_rooms, anon));
  }

  threads.push_back(std::thread(&bot::RPCChatBotServer::run, &rpc_server));
  threads.push_back(std::thread(&bot::ChatBot::connect, chatbot));
  threads.push_back(std::thread(&bot::ChatBot::ping_server, chatbot));
  threads.push_back(
      std::thread(&bot::RSSEventRepository::poll, event_repository));
  threads.push_back(std::thread(&check_timers, chatbot));

  for (auto &t : threads) {
    if (t.joinable()) t.join();
  }

  return 0;
}
