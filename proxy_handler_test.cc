#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "config_parser.h"
#include "request_handler.h"

class MockNginxConfigParser : public NginxConfigParser {};
class MockNginxConfig : public NginxConfig {};

class ProxyHandlerTest : public ::testing::Test {
 protected:
  ProxyHandlerTest() {
    const std::string uri_prefix = "/";
    ParseString("host www.google.com; port 80;");
    status_ = proxy_handler_.Init(uri_prefix, config_);
    request_ = Request::Parse(TEST_BUFFER);
  }

  bool ParseString(const std::string config_string) {
    std::stringstream config_stream(config_string);
    return parser_.Parse(&config_stream, &config_);
  }
  MockNginxConfigParser parser_;
  MockNginxConfig config_;

  Handler_Proxy proxy_handler_;
  
  const std::string TEST_BUFFER = "GET / HTTP/1.1\r\n\r\n";
  RequestHandler::Status status_;
  std::unique_ptr<Request> request_;
  Response response_;
};

TEST_F(ProxyHandlerTest, InitTest) {
  EXPECT_EQ(status_, RequestHandler::Status::OK);
}

TEST_F(ProxyHandlerTest, NullResponse) {
  EXPECT_EQ(proxy_handler_.HandleRequest(*request_, nullptr), RequestHandler::Status::ERROR);
}

TEST_F(ProxyHandlerTest, HandleProperRequestTest) {
  EXPECT_EQ(proxy_handler_.HandleRequest(*request_, &response_), RequestHandler::Status::OK);
}
