run unit test guide:

1)sudo apt-get install libcunit1 libcunit1-doc libcunit1-dev ; if not install cunit,will compile error(can't find file CUError.h)

2)cd wakaama/tests

3)rm CMakeCache.txt;  rm -rf CMakeFiles/ ; rm Makefile;  these files will be created again

4)cmake ./ ;  will generate Makefile

5)make ; will Built target lwm2munittests

6)make test;  will run tests


////////////////log:

impact@impact-node:~/lwm2m/wakaama/tests$ make test
Running tests...
Test project /home/impact/lwm2m/wakaama/tests
    Start 1: test_all
1/1 Test #1: test_all .........................   Passed    0.00 sec

100% tests passed, 0 tests failed out of 1


