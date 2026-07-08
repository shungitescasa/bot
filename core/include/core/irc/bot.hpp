#pragma once

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <string>

#include "core/bot.hpp"
#include "core/log.hpp"
#include "core/message.hpp"

namespace bot::irc {
  class IRCChatBot : public EventChatBot, public ChatBot {
    public:
      IRCChatBot(std::string host, std::string port, std::string nick,
                 std::string pass)
          : host(host),
            port(port),
            nick(nick),
            pass(pass),
            me(nick),
            ssl(boost::asio::ssl::context::tls_client),
            socket(this->io, this->ssl),
            logger("IRCChatBot:" + host) {}

      void send_message(const std::string &room,
                        const std::string &message) override;
      void connect() override;
      void join(const MessageSource &source) override;

      void send_raw(const std::string &message);

      const MessageSource &get_me() const override;

    private:
      const std::string host, port, nick, pass;

      MessageSource me;

      boost::asio::io_context io;
      boost::asio::ssl::context ssl;
      boost::asio::ssl::stream<boost::asio::ip::tcp::socket> socket;
      boost::asio::streambuf buffer;

      Logger logger;

      // void parse_buffer(const boost::asio::streambuf &buffer);
  };
}