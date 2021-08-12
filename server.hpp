//
// server.hpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP

#include <asio.hpp>
#include <string>
#include "connection.hpp"
#include "connection_manager.hpp"
#include "request_handler.hpp"

/// 方便调用者引用头文件时只引用server.hpp，这两个类在回调中要用到
#include "inc/asio_http_server.h"
namespace http {
namespace server {

/// The top-level class of the HTTP server.
class server: public asio_http_server
{
public:
  server(const server&) = delete;
  server& operator=(const server&) = delete;

  /// Construct the server to listen on the specified TCP address and port, and
  /// serve up files from the given directory.
  explicit server(const std::string& address, const std::string& port,
      const std::string& doc_root);

  /// Run the server's io_context loop.
  void run();

  void set_callback(_HTTP_SERVER_CALLBACK _pfunc_callback)
  {
      request_handler_.pfunc_callback = _pfunc_callback;
  }

private:
  /// Perform an asynchronous accept operation.
  void do_accept();

  /// Wait for a request to stop the server.
  void do_await_stop();

  /// The io_context used to perform asynchronous operations.
  asio::io_context io_context_;

  /// The signal_set is used to register for process termination notifications.
  asio::signal_set signals_;

  /// Acceptor used to listen for incoming connections.
  asio::ip::tcp::acceptor acceptor_;

  /// The connection manager which owns all live connections.
  connection_manager connection_manager_;

  /// The handler for all incoming requests.
  request_handler request_handler_;
};

} // namespace server
} // namespace http

asio_http_server *create_asio_http_server(const std::string& address,
                                          const std::string& port,
                                          const std::string& doc_root)
{
    http::server::server* _pserver;
    _pserver = new http::server::server(address, port, doc_root);
    return _pserver;
}


#endif // HTTP_SERVER_HPP
