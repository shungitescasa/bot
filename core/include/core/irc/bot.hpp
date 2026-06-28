#pragma once

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <cstddef>
#include <string>

#include "core/bot.hpp"
#include "core/log.hpp"

namespace bot::irc {
  class IRCChatBot : public ChatBot {
    public:
      IRCChatBot(std::string host, std::string port, std::string nick,
                 std::string pass)
          : host(host),
            port(port),
            nick(nick),
            pass(pass),
            ssl(boost::asio::ssl::context::tls_client),
            socket(this->io, this->ssl),
            logger("IRCChatBot:" + host) {}

      void send_message(const std::string &room,
                        const std::string &message) override;
      void connect() override;
      void send_raw(const std::string &message);

    private:
      const std::string host, port, nick, pass;

      boost::asio::io_context io;
      boost::asio::ssl::context ssl;
      boost::asio::ssl::stream<boost::asio::ip::tcp::socket> socket;
      boost::asio::streambuf buffer;

      Logger logger;

      // void parse_buffer(const boost::asio::streambuf &buffer);
  };
}