if(EXISTS "/home/zzy/CMU15-445/build-release/test/tuple_test[1]_tests.cmake")
  include("/home/zzy/CMU15-445/build-release/test/tuple_test[1]_tests.cmake")
else()
  add_test(tuple_test_NOT_BUILT tuple_test_NOT_BUILT)
endif()