#include "core/irc/bot.hpp"

#include <openssl/tls1.h>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <istream>
#include <print>

namespace bot::irc {
  void IRCChatBot::send_raw(const std::string &message) {
    boost::asio::write(this->socket, boost::asio::buffer(message + "\r\n"));
  }

  void IRCChatBot::send_message(const std::string &room,
                                const std::string &message) {}

  void IRCChatBot::connect() {
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

      std::println("{}", line);
    }

    if (ec) {
      std::println("Error reading from socket: {}", ec.message());
    }
  }
}