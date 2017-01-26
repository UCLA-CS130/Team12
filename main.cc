// Copied from https://github.com/dkawashima/CS-3-Boost-Echo-Static-Server/blob/dkawashi-nginx/
//
// Copyright (c) 2003-2012 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "config_parser.h"
#include <cstdlib>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <thread>
#include <utility>
#include <string>


using boost::asio::ip::tcp;

const int max_length = 4096; //why fixed length?

typedef boost::shared_ptr<tcp::socket> socket_ptr;

void session(socket_ptr sock)
{
  try
  {
    for (;;)
    {
      char data[max_length];
      boost::system::error_code error;
      size_t length = sock->read_some(boost::asio::buffer(data), error);
      if (error == boost::asio::error::eof)
        break; // Connection closed cleanly by peer.
      else if (error)
        throw boost::system::system_error(error); // Some other error.
		  
      //Hard-coded HTTP response for now
      const char* http_header = "HTTP/1.1 200 OK \r\nContent-Type: text/plain\r\n";
      const char* stupid_resp = "ECHOOOOOOOOOO";
      int header_len = std::strlen(http_header);
      int resp_len = std::strlen(stupid_resp);

      boost::asio::write(*sock, boost::asio::buffer(data, length));
      boost::asio::write(*sock, boost::asio::buffer(http_header, header_len));
      boost::asio::write(*sock, boost::asio::buffer(stupid_resp, resp_len));

      return;;
    }

  }
  catch (std::exception& e)
  {
    std::cerr << "Exception in thread: " << e.what() << "\n";
  }
}

void server(boost::asio::io_service& io_service, short port)
{
  tcp::acceptor a(io_service, tcp::endpoint(tcp::v4(), port));
  for (;;)
  {
    /*
    boost::asio::basic_streambuf buffer;
    boost::asio::read_until(buffer, request, "\n");
    */
    socket_ptr sock(new tcp::socket(io_service));
    a.accept(*sock);
    
    std::thread t(session, std::move(sock));
    t.detach();
    //socket_ptr sock1 = std::move(sock);
    //session(std::move(sock));
    //std::thread t(boost::bind(session, sock));
    //boost::bind(session, sock);
  }
}

/* copied from yichi example lines 3-14 */
static int getPort(const NginxConfig &config) { // Gets port from config_file
  for (const auto& statement : config.statements_) {
    bool kl = true;

    //DEBUG: print statement
    std::cout << "DEBUG: statement\n";
    for (const std::string& token : statement->tokens_) {

      // DEBUG: print all tokens
      std::cout << "DEBUG:" << token << std::endl;
      if (!kl) {
        try { return stoi(token); } catch (...) {}
      }
      kl = (token != "listen");
    }
  }
  return -1;
}

int main(int argc, char* argv[])
{
  using namespace std; 
  try
  {
    if (argc != 2)
    {
      std::cerr << "Usage: webserver <config_file>\n";      
      return 1;
    }
    /* copied from yichi server.main lines 17 to 21 */
    NginxConfigParser config_parser;
    NginxConfig config;
    if (!config_parser.Parse(argv[1], &config)) {
      return -1;
    }
    int port_ = getPort(config);
    std::cout << port_ << "\n";
    boost::asio::io_service io;
    server(io, port_);
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}