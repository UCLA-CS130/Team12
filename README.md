# CS130 winter 2017

## Echo server, using boost.
+ *to build the webserver*
`$ make compile_webserver`

+ *to run*
`$ sudo ./webserver example_config`

+ *clean up*
`make clean`

### (Tests need to be rewriten because of internal API change. Only webserver is OK to run.)

+ *run unit tests* (add sudo if not write permission)
```
$ make run_all_tests
```

+ *run test coverage (gcov)* (depends on success of `$ make run_all_tests`)
```
$ make run_test_coverage
```

+ *run integration test* 
```
$ make run_integration_test
```

## Code Layout

+ *Config Format* <br />
see example_config <br />
Currently, all path must be of one word; <br />
"/" will give index page (not implemented yet); <br />

+ *Creating Handlers* <br />
Important function that parses config: `get_server_parameters` in `server.cc` <br />
Handlers are created and stored in a member variable in `class Server` called `uri2handler`, <br />
which is a std::map from uri to handler pointer. e.g. "/static" -> Hander_Static <br />

+ *Connections* <br />
Each connection is separated into a new thread. <br />
In `server.cc`, function `Server::run_server()` will spawn `Server::session(socket_ptr sock)` into a new thread. <br />

+ *Handling Requests* <br />
Request are received in `Server::session(socket_ptr sock)` in each separate thread. <br />
The uri is parsered in to "head" and "tail". (see additional methods in `class Request` in `request_handler.h`)<br />
e.g. "/static/foo/bar" => "/static" + "/foo/bar". <br />
"head" is used to find the corresponding handler, by looking it up in `uri2handler` <br />
(files are read into memory at once, need to consider large files later) <br />

+ *Logging*  <br />
Implemented using singleton, `Logger`. <br />
The method `get_statusPage` will generate a simple html status page as a string. <br />


## Docker 

+ Build Docker Image <br />
`make build_docker_image`

+ Deploy to AWS <br />
`$ ./deploy_aws.sh`

+ Send request to server instance on AWS <br />
e.g. `$ http://52.53.150.174:2020/status`