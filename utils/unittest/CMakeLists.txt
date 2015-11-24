########################################################################
# Experimental CMake build script for Google Test.
#
# Consider this a prototype.  It will change drastically.  For now,
# this is only for people on the cutting edge.
#
# To run the tests for Google Test itself on Linux, use 'make test' or
# ctest.  You can select which tests to run using 'ctest -R regex'.
# For more options, run 'ctest --help'.
########################################################################
#
# Project-wide settings

# Where gtest's .h files can be found.
include_directories(
  googletest/include
  )

add_danbi_library(gtest
  googletest/gtest.cc
  googletest/gtest-death-test.cc
  googletest/gtest-filepath.cc
  googletest/gtest-port.cc
  googletest/gtest-printers.cc
  googletest/gtest-test-part.cc
  googletest/gtest-typed-test.cc
  )

add_danbi_library(gtest_main
  UnitTestMain/TestMain.cpp
  )
