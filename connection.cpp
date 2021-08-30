//
// connection.cpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "connection.hpp"
#include <utility>
#include <vector>
#include "connection_manager.hpp"
#include "request_handler.hpp"
#include <boost/algorithm/string.hpp>

namespace http {
namespace server {

connection::connection(asio::ip::tcp::socket socket,
    connection_manager& manager, request_handler& handler)
  : socket_(std::move(socket)),
    connection_manager_(manager),
    request_handler_(handler)
{
}

void connection::start()
{
  do_read();
}

void connection::stop()
{
  socket_.close();
}

void connection::do_read()
{
  auto self(shared_from_this());
  socket_.async_read_some(asio::buffer(buffer_),
      [this, self](std::error_code ec, std::size_t bytes_transferred)
      {
        if (!ec)
        {
          request_parser::result_type result;
          std::tie(result, std::ignore) = request_parser_.parse(
              request_, buffer_.data(), buffer_.data() + bytes_transferred);

          if (result == request_parser::good)
          {
            /**
            * @brief data_
            * @todo 拼接TCP包
            * @author stx
            */
            std::string data_(buffer_.data(), bytes_transferred);
            // 获取header长度
            uint header_length = data_.find("\r\n\r\n");
            // 获取content长度
            uint content_length=0;
            for(uint i=0;i<request_.headers.size();i++)
            {
                if(boost::algorithm::iequals(request_.headers[i].name, "content-length"))
                {
                    content_length = atoi(request_.headers[i].value.c_str());
                    break;
                }
            }
            /// 在boost中，request_parse结果分为good bad indeterminate
            /// 当为indeterminate使依旧会do_read()
            /// 需要确定是否需要手动合并tcp包
            if(data_.size() < header_length + content_length + 4) // 4 = len("\r\n\r\n")
            {
                std::string _log = "tcp包实际长度与参数不符";
                _log += "TODO 手动合并";
            }
            request_parser_.parse_param(request_, data_);
            request_handler_.handle_request(request_, reply_);
            do_write();
          }
          else if (result == request_parser::bad)
          {
            reply_ = reply::stock_reply(reply::bad_request);
            do_write();
          }
          else
          {
            do_read();
          }
        }
        else if (ec != asio::error::operation_aborted)
        {
          connection_manager_.stop(shared_from_this());
        }
      });
}

void connection::do_write()
{
  auto self(shared_from_this());
  asio::async_write(socket_, reply_.to_buffers(),
      [this, self](std::error_code ec, std::size_t)
      {
        if (!ec)
        {
          // Initiate graceful connection closure.
          asio::error_code ignored_ec;
          socket_.shutdown(asio::ip::tcp::socket::shutdown_both,
            ignored_ec);
        }

        if (ec != asio::error::operation_aborted)
        {
          connection_manager_.stop(shared_from_this());
        }
      });
}

} // namespace server
} // namespace http
