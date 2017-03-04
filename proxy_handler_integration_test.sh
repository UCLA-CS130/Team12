#!/bin/bash

PROXY_CONFIG="port 2021; path /proxy ProxyHandler { host localhost; port 2022; } default NotFoundHandler {}"
HOST_CONFIG="port 2022; path /echo EchoHandler {} default NotFoundHandler {}"

echo "creating test configs"
echo $PROXY_CONFIG > test_proxy_config
echo $HOST_CONFIG > test_host_config

echo "running servers"
./webserver test_proxy_config & sleep 2
./webserver test_host_config & sleep 2

echo "sending requests"
curl localhost:2021/proxy/echo > test_proxy_response
curl localhost:2022/echo > test_host_response

echo "comparing responses"
RC=0
if cmp -s test_proxy_response test_host_response ; then
	echo "integration test SUCCESS"
	$RC=0
else
	echo "integration test FAIL"
	$RC=1
fi

echo "cleaning up"
rm test_proxy_config test_host_config test_proxy_response test_host_response
kill %1
kill %2

exit $RC
