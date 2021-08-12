#ifndef ASIO_HTTP_SERVER_H
#define ASIO_HTTP_SERVER_H
#include <string>

#include "reply.hpp"
#include "request.hpp"

typedef void (* _HTTP_SERVER_CALLBACK)(http::server::request, http::server::reply&);

class asio_http_server{
public:
    virtual void run() = 0;
    virtual void set_callback(_HTTP_SERVER_CALLBACK _pfunc_callback) = 0;
};

extern "C" asio_http_server *create_asio_http_server(const std::string& address,
                                                     const std::string& port,
                                                     const std::string& doc_root);
#endif // ASIO_HTTP_SERVER_H
