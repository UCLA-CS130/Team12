# !!! USE TABS NOT SPACES !!!

GTEST_DIR=googletest/googletest
GMOCK_DIR=googletest/googlemock
CFLAGS = -std=c++0x -g


compile_webserver:
# use -pthread to enable multithreading.
# need to link -lboost_system last.
# -pthread -lboost_system
	g++ $(CFLAGS) server_main.cc server.cc config_parser.cc request_handler.cc logging.cc -o webserver -static-libgcc -static-libstdc++ -pthread -Wl,-Bstatic -lboost_log_setup -lboost_log -lboost_thread -lboost_system 


compile_gtest:
# make googletest, NOT use variable CFLAGS to aviod redundant files
	g++ $(CFLAGS) -isystem ${GTEST_DIR}/include -I${GTEST_DIR} -pthread -c ${GTEST_DIR}/src/gtest-all.cc
	ar -rv libgtest.a gtest-all.o


compile_gmock:
	g++ $(CFLAGS) -isystem ${GTEST_DIR}/include -I${GTEST_DIR} -pthread -c ${GTEST_DIR}/src/gtest-all.cc
	g++ $(CFLAGS) -isystem ${GTEST_DIR}/include -I${GTEST_DIR} -isystem $(GMOCK_DIR)/include -I$(GMOCK_DIR) -pthread -c ${GMOCK_DIR}/src/gmock-all.cc
	ar -rv libgmock.a gmock-all.o gtest-all.o


compile_request_test: compile_gtest
	g++ $(CFLAGS) -isystem ${GTEST_DIR}/include -pthread Request.cc Request_test.cc ${GTEST_DIR}/src/gtest_main.cc libgtest.a -o request_tests


compile_parser_test: compile_gtest
# build config_parser main
	g++ config_parser.cc config_parser_main.cc $(CFLAGS) -o config_parser
# build config_parser test
	g++ $(CFLAGS) -isystem ${GTEST_DIR}/include -pthread config_parser_test.cc config_parser.cc ${GTEST_DIR}/src/gtest_main.cc libgtest.a -o config_parser_test


compile_server_test: compile_gtest
	g++ $(CFLAGS) -isystem ${GTEST_DIR}/include -pthread server_test.cc server.cc config_parser.cc ${GTEST_DIR}/src/gtest_main.cc libgtest.a -o server_tests -lboost_system -lpthread


compile_proxy_handler_test: compile_gtest
	g++ $(CFLAGS) -isystem ${GTEST_DIR}/include -isystem ${GMOCK_DIR}/include -pthread proxy_handler_test.cc request_handler.cc config_parser.cc logging.cc ${GTEST_DIR}/src/gtest_main.cc libgtest.a -o proxy_handler_test -lboost_system -lpthread


compile_all_tests: compile_parser_test compile_server_test compile_proxy_handler_test


compile_all_tests_with_coverage:  CFLAGS += -fprofile-arcs -ftest-coverage
compile_all_tests_with_coverage: compile_all_tests


run_all_tests: compile_all_tests_with_coverage
	./config_parser_test
	./server_tests
	./proxy_handler_test


run_test_coverage: run_all_tests
# add dependency to run_all_tests only when all tests pass
	gcov -r config_parser.cc
	gcov -r server.cc
	gcov -r server_test.cc


run_integration_test: compile_webserver
	python integration_test.py


build_docker_image:
	docker build -t httpserver.build .
	docker run httpserver.build > deploy/binary.tar
	tar -xvf deploy/binary.tar
	rm deploy/binary.tar
	docker build -t httpserver deploy


deploy_aws:
	./deploy_aws.sh


clean:
# use -f to ignore non-existent files
	rm -rf webserver request_tests server_tests config_parser config_parser_test server_tests *.o *.a *~ *.gcov *.gcda *.gcno *.tar

