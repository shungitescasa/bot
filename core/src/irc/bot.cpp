#include "core/irc/bot.hpp"

#include <openssl/tls1.h>

#include <algorithm>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/core/detail/base64.hpp>
#include <format>
#include <istream>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>

#include "core/irc/message.hpp"
#include "core/message.hpp"

namespace bot::irc {
  void IRCChatBot::send_raw(const std::string &message) {
    logger.debug("<<< " + message);
    boost::asio::write(this->socket, boost::asio::buffer(message + "\r\n"));
  }

  void IRCChatBot::send_message(const MessageSource &source,
                                const std::string &message) {
    std::string room = source.unnormalize();
    this->logger.debug(std::format("Sending '{}' to {}...", message, room));
    this->send_raw(std::format("PRIVMSG {} :{}", room, message));
  }

  void IRCChatBot::join(const MessageSource &source) {
    this->send_raw("JOIN " + source.unnormalize());
  }

  void IRCChatBot::part(const MessageSource &source) {
    this->send_raw("PART " + source.unnormalize());
  }

  void IRCChatBot::connect() {
    this->logger.info(
        std::format("Connecting to {}:{}...", this->host, this->port));

    boost::asio::ip::tcp::resolver resolver(this->io);
    boost::asio::connect(socket.next_layer(),
                         resolver.resolve(this->host, this->port));

    SSL_set_tlsext_host_name(socket.native_handle(),
                             this->host.c_str());  // SNI

    socket.handshake(boost::asio::ssl::stream_base::client);

    this->send_raw("CAP LS 302");

    boost::system::error_code ec;
    std::size_t bytes_transferred = 0;

    while (!ec) {
      bytes_transferred =
          boost::asio::read_until(this->socket, this->buffer, "\r\n", ec);

      if (ec) {
        break;
      }

      std::istream is(&buffer);
      std::string line;
      std::getline(is, line);

      if (!line.empty() && line.back() == '\r') {
        line.pop_back();
      }

      if (line.empty()) continue;
      logger.debug(">>> " + line);

      std::optional<IRCMessage> message = IRCMessage::from(line);
      if (!message.has_value()) continue;

      // -- chat message
      if (message->command == "PRIVMSG") {
        std::optional<Message<MessageType::ChatMessage>> chat_message =
            message->as_message<MessageType::ChatMessage>();

        if (chat_message && this->onChatMessage) {
          std::thread(this->onChatMessage, chat_message.value()).detach();
        }
      }
      // -- keep connection alive
      else if (message->command == "PING") {
        this->send_raw("PONG" + (message->params.empty()
                                     ? ""
                                     : (" :" + message->params.front())));
      }
      // -- connected
      else if (onConnect && message->command == "001") {
        std::thread(this->onConnect).detach();
      }
      // -- authenticating on the server
      else if (message->command == "CAP" &&
               std::ranges::contains(message->params, "LS")) {
        bool sasl = false, server_time = false;
        std::string tagCap = "";

        for (const auto &part :
             std::ranges::views::split(message->params.back(), ' ')) {
          std::string_view cap(part.begin(), part.end());

          if (cap.contains("sasl")) {
            sasl = true;
          } else if (std::string(cap) == "message-tags" ||
                     std::string(cap) == "twitch.tv/tags") {
            tagCap = cap;
          } else if (std::string(cap) == "server-time") {
            server_time = true;
          }
        }

        if (!tagCap.empty()) this->send_raw("CAP REQ :" + tagCap);
        if (server_time) this->send_raw("CAP REQ :server-time");

        if (sasl) {
          this->send_raw("CAP REQ :sasl");
          this->send_raw("AUTHENTICATE PLAIN");
        } else {
          logger.info("Authenticating with server password...");
          this->send_raw("PASS " + this->pass);
          this->send_raw("NICK " + this->nick);
        }
      }
      // SASL (process)
      else if (message->command == "AUTHENTICATE" &&
               std::ranges::contains(message->params, "+")) {
        logger.info("Authenticating with SASL...");
        std::string auth = '\0' + this->nick + '\0' + this->pass;

        // base64 encoding.....
        namespace b64 = boost::beast::detail::base64;
        std::string output;
        output.resize(b64::encoded_size(auth.size()));
        output.resize(b64::encode(output.data(), auth.data(), auth.size()));

        this->send_raw("AUTHENTICATE " + output);
      }
      // SASL (success)
      else if (message->command == "903") {
        logger.info("Finishing...");
        this->send_raw("CAP END");
        this->send_raw("NICK " + this->nick);
        this->send_raw(std::format("USER {0} 0 * :{0}", this->nick));
      }
      // SASL (error)
      else if (message->command == "904") {
        throw std::runtime_error(
            "Invalid username or password (SASL authentication failed)");
      }
      // setting actual nickname
      else if (message->command == "NICK") {
        logger.info(std::format("Renamed this bot from {} to {}",
                                this->me.login, message->params.at(0)));
        this->me.login = message->params.at(0);
      }
    }

    if (ec) {
      logger.error("Error reading from socket: " + ec.message());
    }
  }

  const MessageSource &IRCChatBot::get_me() const { return this->me; }
}
