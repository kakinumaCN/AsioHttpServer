//
// mime_types.cpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "mime_types.hpp"

namespace http {
namespace server {
namespace mime_types {

struct mapping
{
  const char* extension;
  const char* mime_type;
} mappings[] =
/**
 * @brief 返回header的编码设置，utf-8解决中文乱码问题
 * @details 上层根据url的后缀判断，此处只能修改固定后缀的编码头
 * @todo 无后缀返回体编码头
 * @author stx
 */
{
  { "gif", "image/gif" },
  { "htm", "text/html" },
  { "html", "text/html; charset=UTF-8" },
  { "jpg", "image/jpeg" },
  { "png", "image/png" }
};

std::string extension_to_type(const std::string& extension)
{
  for (mapping m: mappings)
  {
    if (m.extension == extension)
    {
      return m.mime_type;
    }
  }
  ///@brief 为plain设置utf-8编码
  return "text/plain; charset=UTF-8";
}

} // namespace mime_types
} // namespace server
} // namespace http
