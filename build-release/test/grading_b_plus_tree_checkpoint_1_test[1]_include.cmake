if(EXISTS "/home/zzy/CMU15-445/build-release/test/grading_b_plus_tree_checkpoint_1_test[1]_tests.cmake")
  include("/home/zzy/CMU15-445/build-release/test/grading_b_plus_tree_checkpoint_1_test[1]_tests.cmake")
else()
  add_test(grading_b_plus_tree_checkpoint_1_test_NOT_BUILT grading_b_plus_tree_checkpoint_1_test_NOT_BUILT)
endif()
