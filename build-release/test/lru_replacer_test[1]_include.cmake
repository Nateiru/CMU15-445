if(EXISTS "/home/zzy/CMU15-445/build-release/test/lru_replacer_test[1]_tests.cmake")
  include("/home/zzy/CMU15-445/build-release/test/lru_replacer_test[1]_tests.cmake")
else()
  add_test(lru_replacer_test_NOT_BUILT lru_replacer_test_NOT_BUILT)
endif()
