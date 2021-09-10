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
#include <regex>
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

          /**
           * @brief 数据接收处理说明
           * @author liyang
           * @details 如果是get请求，原代码没有问题;如果是post请求，这个就只能接收第一次请求需要改造
           *          因为get请求是一次tcp 而post请求可能需要多次tcp.
           *          当body数据超长时调用doread方法读取数据.
           */
          std::string http_data = buffer_.data();
          // 如果buffer_数据填满，赋值后http_data尾部存在乱码，需要将多余数据摘除
          // buffer_ 的max_size() 和 size()获取长度一致
          if(http_data.size() > buffer_.max_size()) http_data.resize(buffer_.max_size());
          // 获取完数据一定要清空
          buffer_ ={'\0'};
          if(!receiving_)
          {
              receiving_ =true;//开始接收数据
              request_first = request_;
              result_first = result;
              if("POST" == request_first.method)
              {
                  // 获取header长度(加上结束符)
                  size_t header_length = http_data.find("\r\n\r\n") +4;
                  // 获取content长度
                  content_length_ = 0;
                  for(auto head :request_first.headers)
                  {
                      if(boost::algorithm::iequals(head.name,"Content-Length"))
                      {
                          content_length_ = atoi(head.value.data());
                          break;
                      }
                  }
                  // 如果存在内容则拷贝
                  if(http_data.size() > header_length)
                  {
                      request_first.content =http_data.substr(header_length,http_data.size()-header_length);
                  }
                  // 内容未拷贝完则再次读取
                  if(content_length_ > request_first.content.size())
                  {
                      do_read();
                      return;
                  }
              }
          }
          else
          {
              if("POST" == request_first.method
                      && content_length_ > request_first.content.size())
              {
                  // 拷贝内容
                  request_first.content.append(http_data.data(),http_data.size());
                  if(http_data.size() == buffer_.max_size()
                          && content_length_ > request_first.content.size())
                  {
                      do_read();
                      return;
                  }
              }
          }

          if (result_first == request_parser::good)
          {
              receiving_ = false;
              request_parser_.parse_param(request_first);
              request_handler_.handle_request(request_first, reply_);
              do_write();
          }
          else if (result_first == request_parser::bad)
          {
              receiving_ = false;
              reply_ = reply::stock_reply(reply::bad_request);
              do_write();
          }
          else
          {
              receiving_ = false;
              do_read();
          }
        }
        else if (ec != asio::error::operation_aborted)
        {
            receiving_ = false;
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
