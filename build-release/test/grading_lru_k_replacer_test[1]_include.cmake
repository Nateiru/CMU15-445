if(EXISTS "/home/zzy/CMU15-445/build-release/test/grading_lru_k_replacer_test[1]_tests.cmake")
  include("/home/zzy/CMU15-445/build-release/test/grading_lru_k_replacer_test[1]_tests.cmake")
else()
  add_test(grading_lru_k_replacer_test_NOT_BUILT grading_lru_k_replacer_test_NOT_BUILT)
endif()