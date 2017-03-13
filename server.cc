// Copyright (c) 2003-2012 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "server.h"
#include "request_handler.h"
#include "logging.h"


struct server_parameters{
  int port = -1;
  std::map <std::string, RequestHandler*> uri2handler;
};


server_parameters get_server_parameters(const NginxConfig &config) {
  server_parameters sp;

  for (const auto& statement : config.statements_) {
    const std::vector<std::string> tokens = statement->tokens_;
      if (tokens[0] == "port"){
        if (tokens.size() >= 2){ 
          sp.port = stoi(tokens[1]); 
        } 
        else{ 
          std::cerr << "CONFIG ERROR: PORT " << tokens.size() << std::endl;
          sp.port = -1; 
        }      
      }
      else if (tokens[0] == "path"){
        if (tokens[2] == "EchoHandler"){
          RequestHandler* h = new Handler_Echo;
          (void*) h->Init(tokens[1], *(statement->child_block_));
          sp.uri2handler[tokens[1]] = h;
          Logger::Instance()->add2handlerMap(tokens[1], tokens[2]);
        }
        else if (tokens[2] == "StaticHandler"){
          RequestHandler* h = new Handler_Static;
          (void*) h->Init(tokens[1], *(statement->child_block_));
          sp.uri2handler[tokens[1]] = h;
          Logger::Instance()->add2handlerMap(tokens[1], tokens[2]);
        }
        else if (tokens[2] == "ProxyHandler"){
          RequestHandler* h = new Handler_Proxy;
          (void*) h->Init(tokens[1], *(statement->child_block_));
          sp.uri2handler[tokens[1]] = h;
          Logger::Instance()->add2handlerMap(tokens[1], tokens[2]);
        }
        else if (tokens[2] == "PythonHandler"){
          RequestHandler* h = new Handler_Python;
          (void*) h->Init(tokens[1], *(statement->child_block_));
          sp.uri2handler[tokens[1]] = h;
          Logger::Instance()->add2handlerMap(tokens[1], tokens[2]);
        }
        else{
          continue;
        }
      }
      else if (tokens[0] == "default"){
        RequestHandler* h = new Handler_404;
        (void*) h->Init("/404", *(statement->child_block_));
        sp.uri2handler["/404"] = h;
        Logger::Instance()->add2handlerMap("/404", tokens[1]);
      }
      else if (tokens[0] == "error"){
        RequestHandler* h = new Handler_500;
        (void*) h->Init("/500", *(statement->child_block_));
        sp.uri2handler["/500"] = h;
        Logger::Instance()->add2handlerMap("/500", tokens[1]);
      }
      else if (tokens[0] == "status"){
        RequestHandler* h = new Handler_Status;
        (void*) h->Init(tokens[1], *(statement->child_block_));
        sp.uri2handler[tokens[1]] = h;
        Logger::Instance()->add2handlerMap(tokens[1], tokens[2]);
      }
  }

  std::cout << "DEBUG: URI to Handler Map";
  for (auto it=sp.uri2handler.begin(); it!=sp.uri2handler.end(); ++it){
    std::cout << it->first << " => " << it->second << '\n';
  }
  return sp;
}


Server::Server(const NginxConfig &config)
{
  server_parameters sp = get_server_parameters(config);

  this->port = sp.port;
  this->uri2handler = sp.uri2handler;
}

//helpper function to find the longest prefix  
// 
std::string Server::find_longest_prefix(std::string uri) 
{

    std::string longest = "";
    size_t last = uri.find_last_of("/");
    std::string prefix = uri;
    for (auto it : uri2handler) 
    {
        std::string map = it.first;

        // if can find the map in the prefix and map is followed by "/" or map is the prefix 
        if (prefix.find(map) == 0 && (prefix.find("/", map.length()) == map.length() || 
            map.length() == prefix.length() || map == "/")) 
        {
            if (map.length() > longest.length()) 
            {
                longest = map;
            }
        }
    }

    std::cout << " The longest prefix is ::::::::: " << longest <<std::endl;
    return longest;
}

void Server::session(socket_ptr sock)
{
  const int max_length = 1024;

  try
  {
    // TODO: implement timeout for session
    for (;;)
    {
      char data[max_length];
      boost::system::error_code error;
      size_t length = sock->read_some(boost::asio::buffer(data), error);
      if (error == boost::asio::error::eof)
        break; // Connection closed cleanly by peer.
      else if (error)
        throw boost::system::system_error(error); // Some other error.

      // change the logger 
      Logger::Instance()->incNumOfReq();
      
      std::unique_ptr<Request> req = Request::Parse(data);
      Response res;

      RequestHandler::Status handle_stat;
      auto it_uh = uri2handler.find(find_longest_prefix(req->uri()));
      if (it_uh == uri2handler.end())
      {
        handle_stat = uri2handler["/404"]->HandleRequest(*req, &res);
      }
      else
      {
        handle_stat = it_uh->second->HandleRequest(*req, &res);
      }

      if (handle_stat == RequestHandler::NOT_FOUND){
        (void*) uri2handler["/404"]->HandleRequest(*req, &res);
      }
      else if (handle_stat == RequestHandler::ERROR){
        (void*) uri2handler["/500"]->HandleRequest(*req, &res);
      }
      
      boost::asio::write(*sock, boost::asio::buffer(res.ToString()));
    }

  }
  catch (std::exception& e)
  {
    std::cerr << "Exception in thread: " << e.what() << "\n";
  }
}


void Server::run_server()
{
  boost::asio::io_service io_service;
  tcp::acceptor a(io_service, tcp::endpoint(tcp::v4(), port));

  // Reuse port (server useually )
  // TODO: Implement catch signal to cleanly shut down server
  // http://stackoverflow.com/questions/6364681/how-to-handle-control-c-in-a-boost-tcp-udp-server
  a.set_option(tcp::acceptor::reuse_address(true));

  std::cout << "Listening on port " << port << "\n";
  for (;;)
  {
    socket_ptr sock(new tcp::socket(io_service));
    a.accept(*sock);
    
    // use std thread for non-blocking.
    std::thread t(&Server::session, this, std::move(sock));
    t.detach();
  }
}