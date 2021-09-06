//
// request.hpp
// ~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <string>
#include <vector>
#include <map>
#include "header.hpp"

namespace http {
namespace server {

/// A request received from a client.
struct request
{
  std::string method;
  std::string uri;
  int http_version_major;
  int http_version_minor;
  std::vector<header> headers;

  /**
   * @brief 添加传递参数
   * @author stx
   */
  std::string short_uri; // GET中去掉参数后的uri
  std::map<std::string,std::string> params; // 参数列表，string-string的map
  std::string content; // POST中body数据
};

} // namespace server
} // namespace http

#endif // HTTP_REQUEST_HPP
