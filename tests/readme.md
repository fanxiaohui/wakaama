run unit test guide:

1)sudo apt-get install libcunit1 libcunit1-doc libcunit1-dev ; if not install cunit,will compile error(can't find file CUError.h)

2)cd wakaama/tests

3)rm CMakeCache.txt;  rm -rf CMakeFiles/ ; rm Makefile;  these files will be created again

4)cmake ./ ;  will generate Makefile

5)make ; will Built target lwm2munittests

6)./lwm2munittests;  run tests with detail output

////////////////log:

Suite: Suite_TLV_JSON
  Test: test of test_1() ...passed
  Test: test of test_2() ...passed
  Test: test of test_3() ...passed
  Test: test of test_4() ...passed
  Test: test of test_5() ...passed
  Test: test of test_6() ...passed
  Test: test of test_7() ...passed
  Test: test of test_8() ...passed
  Test: test of test_10() ...passed
  Test: test of test_11() ...passed
  Test: test of test_12() ...passed

Run Summary:    Type  Total    Ran Passed Failed Inactive
              suites      1      1    n/a      0        0
               tests     11     11     11      0        0
             asserts     15     15     15      0      n/a



