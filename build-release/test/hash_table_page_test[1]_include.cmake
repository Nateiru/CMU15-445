if(EXISTS "/home/zzy/CMU15-445/build-release/test/hash_table_page_test[1]_tests.cmake")
  include("/home/zzy/CMU15-445/build-release/test/hash_table_page_test[1]_tests.cmake")
else()
  add_test(hash_table_page_test_NOT_BUILT hash_table_page_test_NOT_BUILT)
endif()