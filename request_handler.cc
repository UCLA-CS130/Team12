#include "request_handler.h"
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <string>

std::unique_ptr<Request> Request::Parse(const std::string& raw_request){
	
	std::unique_ptr<Request> return_val(new Request());

	std::cout<<"start parse_request"<<std::endl;
	std::istringstream req_stream(raw_request);
	std::string req_first_line;


	//deal with the first line from the requst message 
    std::getline(req_stream, req_first_line);
	// start to decode first line 
    std::string method = "";
    std::string url = "";
    std::string http_ver = "";
    int space_count = 0;
    for (unsigned i = 0; i < req_first_line.length(); i++)
    {
        if (req_first_line[i] == ' ')
        {
            space_count++;
            continue;
        }
        if (space_count == 0)
        {
            method += toupper(req_first_line[i]);
        }
        else if (space_count == 1)
        {
            url += req_first_line[i];
        }
        else
        {
            http_ver += toupper(req_first_line[i]);
        }
    }

    return_val->m_uri = url;
   	std::cout<< "uri is: "<<return_val->m_uri <<std::endl;

    return_val->m_method = method;
    return_val->m_version = http_ver;
    return_val->m_raw_request = raw_request;

    // now deal with the headers 
    std::string line;
    std::string field;
	std::string value;
    while(std::getline(req_stream, line)&& line != "\r")
    {
    	std::size_t index = line.find_first_of(":");
    	if(index == std::string::npos)
    	{
    		std::cout << "can not parse request line : "<<line<< std::endl;
    	}
    	else
    	{
    		field = line.substr(0, index);
    		value = line.substr(index+2);
    		// delete the \r at the end
    		value = value.substr(0, value.length()-1);
    	}

    	std::cout << "push the header:  " <<field << " : " <<value << std::endl;
      if(field == "Content-Length")
      {
        return_val->m_body_size = atoi(value.c_str());
      }
    	return_val->m_headers.push_back(std::make_pair(field, value));

    }


    // deal with the body of the request 
    while(std::getline(req_stream, line))
    {
    	return_val->m_body += line;
    }
    std::cout << "the body of the request is : "<< std::endl<< return_val->m_body <<std::endl;

    return return_val;

}


std::string Request::uriHead() const
{
	std::string url = m_uri;
	if (url.length() == 0){
		return "/index";
	}
	url.erase(0,1);
    size_t pos = url.find("/");
    std::string head = url.substr(0, pos);
    std::cout<< "head of uri : /"<< head << std::endl;
	if (head == ""){
		return "/index";
	}
    return "/"+head;
}


std::string Request::uriTail() const 
{
	std::string url = m_uri;
	if (url.length() == 0){
		
	}
	url.erase(0,1);
    size_t pos = url.find("/");
	if (pos == std::string::npos){
		return "";
	}
    std::string tail = url.substr(pos);
    std::cout<< "tail of uri : "<< tail << std::endl;
    return tail;
}


std::string Request::raw_request() const {
    return m_raw_request;
}

std::string Request::method() const {
    return m_method;
}

int Request::body_size() const {
  return m_body_size;
}

std::string Request::uri() const {
    return m_uri;
}

std::string Request::version() const {
    return m_version;
}

Request::Headers Request::headers() const {
    return m_headers;
}

std::string Request::body() const {
    return m_body;
}


// =================  RESPONSE ===================================


void Response::SetStatus(const ResponseCode response_code)
{
	m_code = response_code;
	if(response_code == OK)
	{
		m_first_line = "HTTP/1.0 200 OK";
	}
	else if(response_code == NOT_FOUND)
	{
		m_first_line = "HTTP/1.0 400 Not Found";

	}
	else if(response_code== NOT_IMPLEMENTED)
	{
		m_first_line = "HTTP/1.0 501 Not Implemented";
	}
}

void Response::AddHeader(const std::string& header_name, const std::string& header_value) 
{
	m_headers.push_back(std::make_pair(header_name, header_value));
}

void Response::SetBody(const std::string& body) 
{
	m_body = body; 
}

std::string Response::ToString()
{
	// deal with the first line of the response
	std::string ret_val = m_first_line;

	for(int n= 0 ;n<m_headers.size(); n++)
	{
		std::pair<std::string, std::string> header_pair = m_headers[n];
		ret_val += header_pair.first;
		ret_val += ": ";
		ret_val +=  header_pair.second;
		ret_val += "\r\n";
	}
	ret_val += "\r\n";

	//deal with the body

	ret_val += m_body;

	return ret_val;
}



//========= HANDLERSS ===========================================


RequestHandler::Status Handler_Echo::HandleRequest(const Request& req, Response* res){
	res->SetStatus(res->OK);
	res->AddHeader("Content-type", "text/plain");
	res->AddHeader("Content-length", std::to_string((int)req.raw_request().length()));
	res->SetBody(req.raw_request());
	return OK;
}


RequestHandler::Status Handler_404::HandleRequest(const Request& req, Response* res){
	res->SetStatus(res->NOT_FOUND);
	res->AddHeader("Content-type", "text/html");
	res->AddHeader("Content-length", "79");
	res->SetBody("<html><body><h1>404 Can not file what you are looking for :(</h1></body></html>");
	return OK;
}


RequestHandler::Status Handler_500::HandleRequest(const Request& req, Response* res){
	res->SetStatus(res->INTERNAL_SERVER_ERROR);
	res->AddHeader("Content-type", "text/html");
	res->AddHeader("Content-length", "62");
	res->SetBody("<html><body><h1>500 Internal Server Error!(</h1></body></html>");
	return OK;
}


RequestHandler::Status Handler_Static::Init(const std::string& uri_prefix, const NginxConfig& config)
{
	this->uri = uri_prefix;
	for (const auto& statement : config.statements_) {
		const std::vector<std::string> tokens = statement->tokens_;
		if (tokens[0] == "root"){
			if (tokens.size() >= 2){ 
				this->rootDir = tokens[1];
				// remove the last / 
				if (rootDir.back() == '/')
				{
					rootDir = rootDir.substr(0, rootDir.length()-1);
				}
			}
		}
	}
	return OK;
}

RequestHandler::Status Handler_Static::HandleRequest(const Request& req, Response* res){
	

	std::string file_path;


	std::string file_name = req.uri();
	// uri is member of handler(prefix), kind of confusing with the request's uri 
	file_name.erase(0, uri.length());
	if (file_name.empty() || file_name == "/") {
        std::cout << "Empty file name" << std::endl;
        return RequestHandler::NOT_FOUND;
    }
    if(file_name[0]!='/')
    	file_name = "/" + file_name;
    file_path = rootDir + file_name;
    std::cout << " file name is  : "<< file_name << std::endl;

    std::cout << " file path is  : "<< file_path << std::endl;




	int file_fd = open(file_path.c_str(), O_RDONLY);
	//can not open the file
	if(file_fd == -1)
	{
		return NOT_FOUND;
	}
	struct stat file_info;
	if(fstat(file_fd, &file_info) < 0)
	{
		std::cerr << "Can not retrive file info" << file_path << ".\n";
		return ERROR;
	}

	res->SetStatus(res->OK);

  bool flg_md = false;

	// get the content type of the file 
	// Get insight from https://github.com/UCLA-CS130/Team-KBBQ/blob/master/HttpResponse.cc
	std::string file_type;
	std::string file_extension;
	size_t pos = file_name.find_last_of('.');
	if (pos == std::string::npos || (pos+1) >= file_name.size())
	{
		res->AddHeader("Content-type", "application/octet-stream");
	}
	else 
	{	
	// retreive the file extension
		file_extension = file_name.substr(pos+1);
		std::transform(file_extension.begin(), file_extension.end(), file_extension.begin(), ::tolower);
		
		if(file_extension=="gif")
			file_type = "image/gif";

		else if(file_extension=="jpeg")
			file_type = "image/jpeg";

		else if(file_extension=="jpg")
			file_type = "image/jpeg";

		else if(file_extension=="htm")
			file_type = "text/html";

		else if(file_extension=="html")
			file_type = "text/html";

		else if(file_extension=="png")
			file_type = "image/png";

		else if(file_extension=="pdf")
			file_type = "application/pdf";

    else if(file_extension=="md"){
      file_type = "text/html";
      flg_md = true;
    }

		else
			file_type = "text/plain";
	}

	std::cout<< "file type is: " <<file_type<<std::endl;
	res->AddHeader("Content-type", file_type);

	// Read entire file
	std::ifstream ifs(file_path);
	std::string content( (std::istreambuf_iterator<char>(ifs)),
                     (std::istreambuf_iterator<char>()) );
  if (flg_md){
    std::ostringstream s;
    markdown::Document doc;
    doc.read(content);
    doc.write(s);
    content = s.str();
    res->AddHeader("Content-length", std::to_string(content.length()));
  }
  else{
    res->AddHeader("Content-length", std::to_string(file_info.st_size));
  }

	res->SetBody(content);
	return OK;
}


RequestHandler::Status Handler_Status::HandleRequest(const Request& req, Response* res){
	res->SetStatus(res->OK);
	res->AddHeader("Content-type", "text/html");
	std::string statusPage = Logger::Instance()->get_statusPage();
	res->AddHeader("Content-length", std::to_string((int)statusPage.length()));
	res->SetBody(statusPage);
	return OK;
}



RequestHandler::Status Handler_Proxy::Init(const std::string& uri_prefix, const NginxConfig& config) {
  this->uri = uri_prefix;

  for (const auto& statement : config.statements_) {
    const std::vector<std::string> tokens = statement->tokens_;
    if (tokens[0] == "host") {
      if (tokens.size() >= 2) {
        this->host = tokens[1];
      }
    } else if (tokens[0] == "port") {
      if (tokens.size() >= 2) {
        this->port = tokens[1];
      }
    } else {
      return ERROR;
    }
  }

  return OK;
}


RequestHandler::Status Handler_Proxy::HandleRequest(const Request& req, Response* res){
	if (res == nullptr) {
		return ERROR;
	}

  while (true) {
    // connect to host
    boost::asio::io_service io_service;
    boost::asio::ip::tcp::resolver resolver(io_service);
    boost::asio::ip::tcp::resolver::query query(host, port);
    boost::asio::ip::tcp::resolver::iterator endpoint_iterator;
    try {
      endpoint_iterator = resolver.resolve(query);
    } catch (std::exception& e) {
      std::cerr << "Exception: " << e.what() << std::endl;
      Handler_500 handler_500;
      handler_500.HandleRequest(req, res);
      return OK;
    }
    boost::asio::ip::tcp::socket sock(io_service);
    boost::asio::connect(sock, endpoint_iterator);

    // get relative URI
    std::string request_uri = req.uri();
    std::string relative_uri;
    if (request_uri != uri 
        && uri != "/"
        && request_uri.size() > uri.size() 
        && request_uri.substr(0, uri.size()) == uri) {
      relative_uri = request_uri.substr(uri.size());
    } else {
      relative_uri = request_uri;
    }

    // send request to host
    std::string request = "GET " + relative_uri + " HTTP/1.1\r\n"
                        + "Host: " + host + ":" + port + "\r\n"
                        + "Connection: close\r\n\r\n";
    boost::asio::write(sock, 
                       boost::asio::buffer(request, request.length()));

    // get response from host
    boost::asio::streambuf buffer;
    boost::system::error_code error;
    while (boost::asio::read(sock, buffer, error)) {
      if (error) break;
    }
    std::string raw_response((std::istreambuf_iterator<char>(&buffer)), std::istreambuf_iterator<char>());
    ParseResponse(raw_response);

    if (response_code == 301 || response_code == 302) { // redirect. try new host
      for (auto h : headers) {
        if (h.first == "Location") {
          host = h.second;
        }
      }
    } else {
  		std::cerr << "sent response for " << relative_uri << std::endl;
      break;
    }
  }

  res->SetStatus(static_cast<Response::ResponseCode>(response_code));
  for (auto h : headers) {
    res->AddHeader(h.first, h.second);
  }
  res->SetBody(body);

  return OK;
}


RequestHandler::Status Handler_Proxy::ParseResponse(const std::string& raw_response){
  // clear response member variables
  http_version = "";
  response_code = 0;
  headers.clear();
  body.clear();
  state = http_version_h;

  // parse raw response, character by character
  for (char c : raw_response) {
    result_type result = consume(c);
    if (result == result_type::bad) {
      return ERROR;
    }
  }
  return OK;
}


Handler_Proxy::result_type Handler_Proxy::consume(char input){
  switch (state) {
    case http_version_h: {
      if (input == 'H') {
        state = http_version_t_1;
        return indeterminate;
      } else {
        return bad;
      }
    }
    case http_version_t_1: {
      if (input == 'T') {
        state = http_version_t_2;
        return indeterminate;
      } else {
        return bad;
      }
    }
    case http_version_t_2: {
      if (input == 'T') {
        state = http_version_p;
        return indeterminate;
      } else {
        return bad;
      }
    }
    case http_version_p: {
      if (input == 'P') {
        state = http_version_slash;
        return indeterminate;
      } else {
        return bad;
      }
    }
    case http_version_slash: {
      if (input == '/') {
        state = http_version_major_start;
        return indeterminate;
      } else {
        return bad;
      }
    }
    case http_version_major_start: {
      if (is_digit(input)) {
        http_version.push_back(input);
        state = http_version_major;
        return indeterminate;
      } else {
        return bad;
      }
    }
    case http_version_major: {
      if (input == '.') {
        http_version.push_back(input);
        state = http_version_minor_start;
        return indeterminate;
      } else if (is_digit(input)) {
        http_version.push_back(input);
        return indeterminate;
      } else {
        return bad;
      }
    }
    case http_version_minor_start: {
      if (is_digit(input)) {
        http_version.push_back(input);
        state = http_version_minor;
        return indeterminate;
      } else {
        return bad;
      }
    }
    case http_version_minor: {
      if (input == ' ') {
        state = http_code_1;
        return indeterminate;
      } else if (is_digit(input)) {
        http_version.push_back(input);
        return indeterminate;
      } else {
        return bad;
      }
    }
    case http_code_1: {
      if (is_digit(input)) {
        response_code = input - '0';
        state = http_code_2;
        return indeterminate;
      } else {
        return bad;
      }
    }
    case http_code_2: {
      if (is_digit(input)) {
        response_code = response_code * 10 + (input - '0');
        state = http_code_3;
        return indeterminate;
      } else {
        return bad;
      }
    }
    case http_code_3: {
      if (is_digit(input)) {
        response_code = response_code * 10 + (input - '0');
        state = expecting_newline_1;
        return indeterminate;
      } else {
        return bad;
      }
    }
    case expecting_newline_1: {
      if (input == '\n') {
        state = header_line_start;
        return indeterminate;
      } else {
        return indeterminate; // ignore status code message
      }
    }
    case header_line_start: {
      if (input == '\r') {
        state = expecting_newline_3;
        return indeterminate;
      } else if (!headers.empty() && (input == ' ' || input == '\t')) {
        state = header_lws;
        return indeterminate;
      } else if (!is_char(input) || is_ctl(input) || is_tspecial(input)) {
        return bad;
      } else {
        headers.push_back(std::pair<std::string, std::string>());
        headers.back().first.push_back(input);
        state = header_name;
        return indeterminate;
      }
    }
    case header_lws: {
      if (input == '\r') {
        state = expecting_newline_2;
        return indeterminate;
      } else if (input == ' ' || input == '\t') {
        return indeterminate;
      } else if (is_ctl(input)) {
        return bad;
      } else {
        state = header_value;
        headers.back().second.push_back(input);
        return indeterminate;
      }
    }
    case header_name: {
      if (input == ':') {
        state = space_before_header_value;
        return indeterminate;
      } else if (!is_char(input) || is_ctl(input) || is_tspecial(input)) {
        return bad;
      } else {
        headers.back().first.push_back(input);
        return indeterminate;
      }
    }
    case space_before_header_value: {
      if (input == ' ') {
        state = header_value;
        return indeterminate;
      } else {
        return bad;
      }
    }
    case header_value: {
      if (input == '\r') {
        state = expecting_newline_2;
        return indeterminate;
      } else if (is_ctl(input)) {
        return bad;
      } else {
        headers.back().second.push_back(input);
        return indeterminate;
      }
    }
    case expecting_newline_2: {
      if (input == '\n') {
        state = header_line_start;
        return indeterminate;
      } else {
        return bad;
      }
    }
    case expecting_newline_3: {
      if (input == '\n') {
        state = body_state;
        return good;
      } else {
        return bad;
      }
    }
    case body_state: {
      // place the rest of the text after headers into the body
      body.push_back(input);
      return indeterminate;
    }
    default: {
      return bad;
    }
  }
}


bool Handler_Proxy::is_char(int c){
  return c >= 0 && c <= 127;
}


bool Handler_Proxy::is_ctl(int c){
  return (c >= 0 && c <= 31) || (c == 127);
}


bool Handler_Proxy::is_tspecial(int c){
  switch (c) {
    case '(': case ')': case '<': case '>': case '@':
    case ',': case ';': case ':': case '\\': case '"':
    case '/': case '[': case ']': case '?': case '=':
    case '{': case '}': case ' ': case '\t':
      return true;
    default:
      return false;
  }
}


bool Handler_Proxy::is_digit(int c){
  return c >= '0' && c <= '9';
}


std::string readfile(std::string filename)
{
  std::ifstream in(filename, std::ios::in | std::ios::binary);
  if (in)
  {
    std::string contents;
    in.seekg(0, std::ios::end);
    contents.resize(in.tellg());
    in.seekg(0, std::ios::beg);
    in.read(&contents[0], contents.size());
    in.close();
    return(contents);
  }
  std::cerr << "DEBUG: NO FILE " << filename << std::endl;
  return "";
}


RequestHandler::Status Handler_Python::Init(const std::string& uri_prefix, const NginxConfig& config){
  this->uri = uri_prefix;

  for (const auto& statement : config.statements_) {
    const std::vector<std::string> tokens = statement->tokens_;
    if (tokens[0] == "pythonPage") {
      if (tokens.size() >= 2) {
        this->pythonPage = readfile((std::string)tokens[1]);
      }
    } else {
      std::cerr << "DEBUG: Python config misformatted \n";
      return ERROR;
    }
  }

  return OK;
}


char from_hex(char ch) {
    return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
}

std::string url_decode(std::string text) {
    char h;
    std::ostringstream escaped;
    escaped.fill('0');

    for (auto i = text.begin(), n = text.end(); i != n; ++i) {
        std::string::value_type c = (*i);

        if (c == '%') {
            if (i[1] && i[2]) {
                h = from_hex(i[1]) << 4 | from_hex(i[2]);
                escaped << h;
                i += 2;
            }
        } else if (c == '+') {
            escaped << ' ';
        } else {
            escaped << c;
        }
    }

    return escaped.str();
}


void replaceAll( std::string &s, const std::string &search, const std::string &replace ) {
    for( size_t pos = 0; ; pos += replace.length() ) {
        // Locate the substring to replace
        pos = s.find( search, pos );
        if( pos == std::string::npos ) break;
        // Replace by erasing and inserting
        s.erase( pos, search.length() );
        s.insert( pos, replace );
    }
}

#define MAX_LEN 1024

RequestHandler::Status Handler_Python::HandleRequest(const Request& req, Response* res){
  std::cerr << "DEBUG: Python Handler\n";
  std::cerr << "DEBUG: " << req.method() << std::endl;
  res->SetStatus(res->OK);
  if (req.method() == "GET"){
    res->AddHeader("Content-type", "text/html");
    res->AddHeader("Content-length", std::to_string(pythonPage.length()));
    res->SetBody(pythonPage);
  }
  else if (req.method() == "POST"){  
    // echo back body
    res->AddHeader("Content-type", "text");

    int req_size = req.body_size();


    // === redirect python stdout to buffer ===
    char buffer[MAX_LEN+1] = {0};
    int out_pipe[2];
    int saved_stdout;

    saved_stdout = dup(STDOUT_FILENO);  /* save stdout for display later */

    if( pipe(out_pipe) != 0 ) {          /* make a pipe */
      exit(1);
    }

    dup2(out_pipe[1], STDOUT_FILENO);   /* redirect stdout to the pipe */
    close(out_pipe[1]);

    Py_Initialize();
    // PyRun_SimpleString((req.body() + "\0").c_str());
    std::string script = req.body() + "\0";
    script = script.substr(0,req_size);
    // script = script.replace("%0A", "\n");
     std::cerr << "DEBUG: " << url_decode(script) << std::endl;
    PyRun_SimpleString(script.c_str());
    PyRun_SimpleString("print ' '");
    Py_Finalize();

    read(out_pipe[0], buffer, MAX_LEN); /* read from pipe into buffer */
    dup2(saved_stdout, STDOUT_FILENO);  /* reconnect stdout*/

    printf("DEBUG REDIRCTED: %s\n", buffer);
    std::cout << "DEBUG: TEST reconnected stdout \n";
    std::cout << "DEBUG: |" << ((std::string)buffer) << "| length "  << std::to_string(((std::string)buffer).length()) << std::endl;
    res->AddHeader("Content-length", std::to_string(((std::string)buffer).length()));
    res->SetBody(((std::string)buffer));

  }
  
  return OK;
}
