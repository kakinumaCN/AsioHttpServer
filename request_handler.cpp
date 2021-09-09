//
// request_handler.cpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "request_handler.hpp"
#include <fstream>
#include <sstream>
#include <string>
#include "mime_types.hpp"
#include "reply.hpp"
#include "request.hpp"

/**
 * @brief 对外暴露的响应回调
 * @author stx
 */
void base_tsCallBack(http::server::request, http::server::reply&) {return;}

namespace http {
namespace server {

request_handler::request_handler(const std::string& doc_root)
  : doc_root_(doc_root)
{
    pfunc_callback = base_tsCallBack;
}

void request_handler::handle_request(const request& req, reply& rep)
{
  // Decode url to path.
  std::string request_path;
  /**
   * @brief 使用short url保证在处理回调时上层调用者可以根据path来判断接口类别
   * @author stx
   */
  if (!url_decode(req.short_uri, request_path)) // 在传参数时使用short_uri而不是uri
  {
    rep = reply::stock_reply(reply::bad_request);
    return;
  }

  // Request path must be absolute and not contain "..".
  if (request_path.empty() || request_path[0] != '/'
      || request_path.find("..") != std::string::npos)
  {
    rep = reply::stock_reply(reply::bad_request);
    return;
  }

  // If path ends in slash (i.e. is a directory) then add "index.html".
  if (request_path[request_path.size() - 1] == '/')
  {
    request_path += "index.html";
  }

  // Determine the file extension.
  std::size_t last_slash_pos = request_path.find_last_of("/");
  std::size_t last_dot_pos = request_path.find_last_of(".");
  std::string extension;
  if (last_dot_pos != std::string::npos && last_dot_pos > last_slash_pos)
  {
    extension = request_path.substr(last_dot_pos + 1);
  }

  // Open the file to send back.
  std::string full_path = doc_root_ + request_path;
  std::ifstream is(full_path.c_str(), std::ios::in | std::ios::binary);
  /**
   * @brief 404的逻辑修改
   * @details 由于asio的example默认从http doc path找返回体，而服务提供者不应每次都需要创建对应的文件并提供固定的返回值
   *          所以默认产生的404视为一种特殊情况，最后检验是否存在文件并且在404产生后【不进行return】，并紧接着触发回调
   *          如果上层调用者可以正确处理这个请求，由调用者自己来覆写响应的状态码并赋返回体
   *          如调用者不做任何判断与处理,无任何影响,正常返回404，回调只是提供一个重写404为正常返回的机会
   * @author stx
   */
  if (!is)
  {
    rep = reply::stock_reply(reply::not_found);
    // return;
  }
  else
  {
      // 如不是404，响应文件存在，则检验完毕，返回200
      rep.status = reply::ok;
  }

  // 触发回调
  pfunc_callback(req,rep);

  // Fill out the reply to be sent to the  client.
  // 如无doc的404被调用者手动置为200,返回体也应由调用者传递,否则返回默认的未被覆写的404内容
  if(rep.status == reply::ok)
  {
      if(rep.content.empty())
      {
          char buf[512];
          // 有文件返回体时才会追加内容，注意content是append的，不应在响应_rep中重复添加返回内容，防止内容混淆 is.read()
          while (is.read(buf, sizeof(buf)).gcount() > 0)
            rep.content.append(buf, is.gcount());
      }

      rep.headers.push_back(header{"Content-Length",std::to_string(rep.content.size())});
      rep.headers.push_back(header{"Content-Type",mime_types::extension_to_type(extension)});
/*
      rep.headers.resize(2);
      rep.headers[0].name = "Content-Length";
      rep.headers[0].value = std::to_string(rep.content.size());
      rep.headers[1].name = "Content-Type";
      rep.headers[1].value = mime_types::extension_to_type(extension);
      */
      //extension 截取uri的.后面部分,为/log.html在mime_types里添加charset
  }
}

bool request_handler::url_decode(const std::string& in, std::string& out)
{
  out.clear();
  out.reserve(in.size());
  for (std::size_t i = 0; i < in.size(); ++i)
  {
    if (in[i] == '%')
    {
      if (i + 3 <= in.size())
      {
        int value = 0;
        std::istringstream is(in.substr(i + 1, 2));
        if (is >> std::hex >> value)
        {
          out += static_cast<char>(value);
          i += 2;
        }
        else
        {
          return false;
        }
      }
      else
      {
        return false;
      }
    }
    else if (in[i] == '+')
    {
      out += ' ';
    }
    else
    {
      out += in[i];
    }
  }
  return true;
}

} // namespace server
} // namespace http
