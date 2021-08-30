//
// request_parser.cpp
// ~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "request_parser.hpp"
#include "request.hpp"

#include <boost/algorithm/string.hpp>
namespace http {
namespace server {

request_parser::request_parser()
  : state_(method_start)
{
}

void request_parser::reset()
{
  state_ = method_start;
}

request_parser::result_type request_parser::consume(request& req, char input)
{
  switch (state_)
  {
  case method_start:
    if (!is_char(input) || is_ctl(input) || is_tspecial(input))
    {
      return bad;
    }
    else
    {
      state_ = method;
      req.method.push_back(input);
      return indeterminate;
    }
  case method:
    if (input == ' ')
    {
      state_ = uri;
      return indeterminate;
    }
    else if (!is_char(input) || is_ctl(input) || is_tspecial(input))
    {
      return bad;
    }
    else
    {
      req.method.push_back(input);
      return indeterminate;
    }
  case uri:
    if (input == ' ')
    {
      state_ = http_version_h;
      return indeterminate;
    }
    else if (is_ctl(input))
    {
      return bad;
    }
    else
    {
      req.uri.push_back(input);
      return indeterminate;
    }
  case http_version_h:
    if (input == 'H')
    {
      state_ = http_version_t_1;
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case http_version_t_1:
    if (input == 'T')
    {
      state_ = http_version_t_2;
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case http_version_t_2:
    if (input == 'T')
    {
      state_ = http_version_p;
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case http_version_p:
    if (input == 'P')
    {
      state_ = http_version_slash;
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case http_version_slash:
    if (input == '/')
    {
      req.http_version_major = 0;
      req.http_version_minor = 0;
      state_ = http_version_major_start;
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case http_version_major_start:
    if (is_digit(input))
    {
      req.http_version_major = req.http_version_major * 10 + input - '0';
      state_ = http_version_major;
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case http_version_major:
    if (input == '.')
    {
      state_ = http_version_minor_start;
      return indeterminate;
    }
    else if (is_digit(input))
    {
      req.http_version_major = req.http_version_major * 10 + input - '0';
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case http_version_minor_start:
    if (is_digit(input))
    {
      req.http_version_minor = req.http_version_minor * 10 + input - '0';
      state_ = http_version_minor;
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case http_version_minor:
    if (input == '\r')
    {
      state_ = expecting_newline_1;
      return indeterminate;
    }
    else if (is_digit(input))
    {
      req.http_version_minor = req.http_version_minor * 10 + input - '0';
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case expecting_newline_1:
    if (input == '\n')
    {
      state_ = header_line_start;
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case header_line_start:
    if (input == '\r')
    {
      state_ = expecting_newline_3;
      return indeterminate;
    }
    else if (!req.headers.empty() && (input == ' ' || input == '\t'))
    {
      state_ = header_lws;
      return indeterminate;
    }
    else if (!is_char(input) || is_ctl(input) || is_tspecial(input))
    {
      return bad;
    }
    else
    {
      req.headers.push_back(header());
      req.headers.back().name.push_back(input);
      state_ = header_name;
      return indeterminate;
    }
  case header_lws:
    if (input == '\r')
    {
      state_ = expecting_newline_2;
      return indeterminate;
    }
    else if (input == ' ' || input == '\t')
    {
      return indeterminate;
    }
    else if (is_ctl(input))
    {
      return bad;
    }
    else
    {
      state_ = header_value;
      req.headers.back().value.push_back(input);
      return indeterminate;
    }
  case header_name:
    if (input == ':')
    {
      state_ = space_before_header_value;
      return indeterminate;
    }
    else if (!is_char(input) || is_ctl(input) || is_tspecial(input))
    {
      return bad;
    }
    else
    {
      req.headers.back().name.push_back(input);
      return indeterminate;
    }
  case space_before_header_value:
    if (input == ' ')
    {
      state_ = header_value;
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case header_value:
    if (input == '\r')
    {
      state_ = expecting_newline_2;
      return indeterminate;
    }
    else if (is_ctl(input))
    {
      return bad;
    }
    else
    {
      req.headers.back().value.push_back(input);
      return indeterminate;
    }
  case expecting_newline_2:
    if (input == '\n')
    {
      state_ = header_line_start;
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case expecting_newline_3:
    return (input == '\n') ? good : bad;
  default:
    return bad;
  }
}

bool request_parser::is_char(int c)
{
  return c >= 0 && c <= 127;
}

bool request_parser::is_ctl(int c)
{
  return (c >= 0 && c <= 31) || (c == 127);
}

bool request_parser::is_tspecial(int c)
{
  switch (c)
  {
  case '(': case ')': case '<': case '>': case '@':
  case ',': case ';': case ':': case '\\': case '"':
  case '/': case '[': case ']': case '?': case '=':
  case '{': case '}': case ' ': case '\t':
    return true;
  default:
    return false;
  }
}

bool request_parser::is_digit(int c)
{
  return c >= '0' && c <= '9';
}

/**
 * @brief 编码用函数
 * @author stx
 */
int hex2int(char ch)
{
    if(48 <= ch && ch <= 57) // 0-9
        return ch - 48;
    else if(65 <= ch && ch <= 70) // A-F
        return ch - 65 + 10;
    else if(97 <= ch && ch <= 102) // a-f
        return ch - 97 + 10;
    return -1;
}

/**
 * @brief 编码用函数
 * @author stx
 */
int hex2int(std::string str)
{
    if(str.size()!=2)
        return -2;
    char ch_high = str.at(0);
    char ch_low = str.at(1);
    int high = hex2int(ch_high)*16;
    int low = hex2int(ch_low);
    return high+low;
}

/**
 * @brief 编码用函数
 * @author stx
 * @brief bugfix 参数="汉":"字"
 *      para : str="汉":"字"
 *                ="%22%E6%B1%89%22:%22%E5%AD%97%22"
 *      split: %22:%22 -> [%22:]
 *      ':' 被分在%22组中,%22:四个字符被视为一个整体
 */
std::string utf8_zh_decode(std::string str)
{
    std::vector<std::string> split_zh_temp;
    boost::split(split_zh_temp,str, boost::is_any_of("%"));
    std::string zh_result;
    for(uint i=1;i<split_zh_temp.size();i++)
    {
        if(split_zh_temp.at(i).size()<2)
        {
            zh_result += '%'; // 补上被分割的%
            zh_result += split_zh_temp.at(i).substr(0,-1); // 短于两字符不处理
            continue;
        }
        zh_result += hex2int(split_zh_temp.at(i).substr(0,2)); // 前两字符转ASCII
        zh_result += split_zh_temp.at(i).substr(2,-1); // 两字符以后的字符原样添加
    }
    return zh_result;
}

/**
 * @brief 参数解析，从url中解参数，其他逻辑使用short_url
 * @author stx
 * @todo 只有GET解析
 */
void request_parser::parse_param(request &req, std::__cxx11::string &data_)
{
    // for GET
    int index = req.uri.find_first_of("?");
    if(index >= 0)
    {
        // todo 如果?是最后一个字符好像会溢出
        // split origin uri
        req.short_uri = req.uri.substr(0, index);
        std::string param_str = req.uri.substr(index + 1, req.uri.size());

        std::vector<std::string> split_result;
        boost::split(split_result, param_str, boost::is_any_of("&"));

        for(uint i=0; i<split_result.size(); i++)
        {
            std::vector<std::string> split_result_temp;
            boost::split(split_result_temp,split_result.at(i), boost::is_any_of("="));
            if(split_result_temp.size() >= 2)
            {
//                req.params.push_back(header());
//                req.params.back().name = ;
//                req.params.back().value = ; // change vector<head> to map<string,string>
                //解决中文UTF8转码问题 %E6%B1%89 = [0xe6,0xb1,0x89] = "汉"
                if(split_result_temp.at(0).at(0)=='%')
                    split_result_temp.at(0) = utf8_zh_decode(split_result_temp.at(0));
                if(split_result_temp.at(1).at(0)=='%')
                    split_result_temp.at(1) = utf8_zh_decode(split_result_temp.at(1));
                req.params.insert(make_pair(split_result_temp.at(0),split_result_temp.at(1)));
            }
        }
    }
    else
    {
        req.short_uri = req.uri;
    }

    //TODO POST
//    data_;
}

} // namespace server
} // namespace http
