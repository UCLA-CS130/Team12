import os
import subprocess
import urllib2
import signal


#This integration test tests HTTP response code, HTTP response headers and
# HTTP repsonse message sepretly, probably need a better way to retrive the whole 
# response(headers + message)

# run the command with the example_config in a system call
server_process = subprocess.Popen("./webserver example_config", stdout=subprocess.PIPE, shell=True)

#spawn another process to send request to the server
req = urllib2.Request('http://localhost:8080',data ='this is a test request message')

response = urllib2.urlopen(req)


expected_code = 200;
expected_headers = """Content-Type: text/plain\r\n"""

expected_msg = """POST / HTTP/1.1\r
Accept-Encoding: identity\r
Content-Length: 30\r
Host: localhost:8080\r
Content-Type: application/x-www-form-urlencoded\r
Connection: close\r
User-Agent: Python-urllib/2.7\r\n\r\nthis is a test request message"""
 
response_msg = str(response.read())
response_headers = str(response.info())
response_code = response.getcode()
#print (response_code)
#print (response_headers)
#print (expected_headers)
#print (response_msg)



print("start testing")

if expected_code != response_code:
  print("Server response code error")
  #  kill the server process
  os.killpg(os.getpgid(server_process.pid), signal.SIGTERM)
  exit(1)
elif expected_headers != response_headers:
  print("Server response headers error")
  #  kill the server process
  os.killpg(os.getpgid(server_process.pid), signal.SIGTERM)
  exit(1)
elif expected_msg != response_msg:
  print("Server response message error")
  #  kill the server process
  os.killpg(os.getpgid(server_process.pid), signal.SIGTERM)
  exit(1)
else:
  print("All integration tests passed!! Congratulations!!")
  #  kill the server process
  os.killpg(os.getpgid(server_process.pid), signal.SIGTERM)
  exit(0)












