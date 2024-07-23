
add_executable(test_core test_core.cc)
target_link_libraries (test_core Ravl2 GTest::gtest_main)
gtest_add_tests(TARGET test_core )
