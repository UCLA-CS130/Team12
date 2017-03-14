#ifndef REQUEST_HANDLER
#define REQUEST_HANDLER

#include "config_parser.h"
#include "logging.h"
#include <string>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <algorithm>
#include <Python.h>
#include "markdown.h"


class Request {
 public:
  static std::unique_ptr<Request> Parse(const std::string& raw_request);

  std::string raw_request() const;
  std::string method() const;
  std::string uri() const;
  std::string version() const;

  using Headers = std::vector<std::pair<std::string, std::string>>;
  Headers headers() const;
  std::string body() const;
  int body_size() const;

  // "/static/foo/bar" => "/static" + "/foo/bar"
  std::string uriHead() const;
  std::string uriTail() const;

  private:
  	std::string m_raw_request;
  	std::string m_method;
  	std::string m_uri;
  	std::string m_version;
  	std::vector<std::pair<std::string, std::string>> m_headers; 
  	std::string m_body;
    int m_body_size;

};

// Represents an HTTP response.
//
// Usage:
//   Response r;
//   r.SetStatus(RESPONSE_200);
//   r.SetBody(...);
//   return r.ToString();
//
// Constructed by the RequestHandler, after which the server should call ToString
// to serialize.
class Response {
 public:
  enum ResponseCode {
    //Informational
	CONTINUE=100,
	SWITCHING,

	//Success
	OK=200,
	CREATED,
	ACCEPTED,
	NO_CONTENT=204,

	//Redirection
	MOVED_PERMANENTLY=301,
	MOVED_TEMPORARILY,
	NOT_MODIFIED=304,

	//Client Error
	BAD_REQUEST=400,
	UNAUTHORIZED,
	FORBIDDEN=403,
	NOT_FOUND,
	METHOD_NOT_ALLOWED,
	NOT_ACCEPTABLE,

	//Server Error
	INTERNAL_SERVER_ERROR=500,
	NOT_IMPLEMENTED,
	SERVICE_UNAVAILABLE=503
  };
  
  void SetStatus(const ResponseCode response_code);
  void AddHeader(const std::string& header_name, const std::string& header_value);
  void SetBody(const std::string& body);
  std::string ToString();

 private:
 	ResponseCode m_code;
 	std::vector<std::pair<std::string, std::string>> m_headers;
 	std::string m_body;
 	// add a new private member, easier to deal with in ToString()
 	std::string m_first_line;

};


// Represents the parent of all request handlers. Implementations should expect to
// be long lived and created at server constrution.
class RequestHandler {
 public:
	enum Status {
		OK = 0,
		NOT_FOUND,
		ERROR
	};
  
  virtual Status Init(const std::string& uri_prefix, const NginxConfig& config) = 0;
  virtual Status HandleRequest(const Request& request, Response* response) = 0;
};


// Different types of derived handlers listed below for now
// for convenience
// may put in separate files later

class Handler_Echo: public RequestHandler {
	public:
		virtual Status Init(const std::string& uri_prefix, const NginxConfig& config){
			this->uri = uri_prefix;
			return OK;
		}
		virtual Status HandleRequest(const Request& request, Response* response);

	private:
		std::string uri;
};


class Handler_Static: public RequestHandler {
	public:
		virtual Status Init(const std::string& uri_prefix, const NginxConfig& config);
		virtual Status HandleRequest(const Request& request, Response* response);
		
	private:
		std::string uri;
		std::string rootDir;
};


class Handler_404: public RequestHandler {
	public:
		virtual Status Init(const std::string& uri_prefix, const NginxConfig& config){
			this->uri = uri_prefix;
			return OK;
		}
		virtual Status HandleRequest(const Request& request, Response* response);

	private:
		std::string uri;
};


class Handler_500: public RequestHandler {
	public:
		virtual Status Init(const std::string& uri_prefix, const NginxConfig& config){
			this->uri = uri_prefix;
			return OK;
		}
		virtual Status HandleRequest(const Request& request, Response* response);

	private:
		std::string uri;
};


class Handler_Status: public RequestHandler {
	public:
		virtual Status Init(const std::string& uri_prefix, const NginxConfig& config){
			this->uri = uri_prefix;
			return OK;
		}
		virtual Status HandleRequest(const Request& request, Response* response);

	private:
		std::string uri;
};

class Handler_Proxy : public RequestHandler {
	public:
		Status Init(const std::string& uri_prefix, const NginxConfig& config);
		Status HandleRequest(const Request& request, Response* response);

	private:
    // handler configuration
		std::string uri;
		std::string host;
		std::string port;

    // store the parsed response
    std::string http_version;
    int response_code;
    using Headers = std::vector<std::pair<std::string, std::string>>;
    Headers headers;
    std::string body;

    // the following are used for parsing the response

    enum result_type { good, bad, indeterminate };

    Status ParseResponse(const std::string& raw_response);
    result_type consume(char input);

    // Check if a byte is an HTTP character.
    static bool is_char(int c);

    // Check if a byte is an HTTP control character.
    static bool is_ctl(int c);

    // Check if a byte is defined as an HTTP tspecial character.
    static bool is_tspecial(int c);

    // Check if a byte is a digit.
    static bool is_digit(int c);

    // The current state of the parser.
    enum response_parser_state {
      http_version_h,
      http_version_t_1,
      http_version_t_2,
      http_version_p,
      http_version_slash,
      http_version_major_start,
      http_version_major,
      http_version_minor_start,
      http_version_minor,
      http_code_1,
      http_code_2,
      http_code_3,
      expecting_newline_1,
      header_line_start,
      header_lws,
      header_name,
      space_before_header_value,
      header_value,
      expecting_newline_2,
      expecting_newline_3,
      body_state
    } state;
};


class Handler_Python: public RequestHandler {
    public:
        virtual Status Init(const std::string& uri_prefix, const NginxConfig& config);
        virtual Status HandleRequest(const Request& request, Response* response);

    private:
        std::string uri;
        std::string pythonPage;
};

#endif
