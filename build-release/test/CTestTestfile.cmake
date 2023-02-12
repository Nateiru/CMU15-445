# CMake generated Testfile for 
# Source directory: /home/zzy/CMU15-445/test
# Build directory: /home/zzy/CMU15-445/build-release/test
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
include("/home/zzy/CMU15-445/build-release/test/binder_test[1]_include.cmake")
include("/home/zzy/CMU15-445/build-release/test/buffer_pool_manager_instance_test[1]_include.cmake")
include("/home/zzy/CMU15-445/build-release/test/clock_replacer_test[1]_include.cmake")
include("/home/zzy/CMU15-445/build-release/test/grading_buffer_pool_manager_instance_test[1]_include.cmake")
include("/home/zzy/CMU15-445/build-release/test/grading_lru_k_replacer_test[1]_include.cmake")
include("/home/zzy/CMU15-445/build-release/test/lru_k_replacer_test[1]_include.cmake")
include("/home/zzy/CMU15-445/build-release/test/lru_replacer_test[1]_include.cmake")
include("/home/zzy/CMU15-445/build-release/test/catalog_test[1]_include.cmake")
include("/home/zzy/CMU15-445/build-release/test/rwlatch_test[1]_include.cmake")
include("/home/zzy/CMU15-445/build-release/test/deadlock_detection_test[1]_include.cmake")
include("/home/zzy/CMU15-445/build-release/test/lock_manager_test[1]_include.cmake")
include("/home/zzy/CMU15-445/build-release/test/transaction_test[1]_include.cmake")
include("/home/zzy/CMU15-445/build-release/test/hash_table_page_test[1]_include.cmake")
include("/home/zzy/CMU15-445/build-release/test/hash_table_test[1]_include.cmake")
include("/home/zzy/CMU15-445/build-release/test/extendible_hash_table_test[1]_include.cmake")
include("/home/zzy/CMU15-445/build-release/test/starter_trie_test[1]_include.cmake")
include("/home/zzy/CMU15-445/build-release/test/recovery_test[1]_include.cmake")
include("/home/zzy/CMU15-445/build-release/test/b_plus_tree_concurrent_test[1]_include.cmake")
include("/home/zzy/CMU15-445/build-release/test/b_plus_tree_contention_test[1]_include.cmake")
include("/home/zzy/CMU15-445/build-release/test/b_plus_tree_delete_test[1]_include.cmake")
include("/home/zzy/CMU15-445/build-release/test/b_plus_tree_insert_test[1]_include.cmake")
include("/home/zzy/CMU15-445/build-release/test/disk_manager_test[1]_include.cmake")
include("/home/zzy/CMU15-445/build-release/test/grading_b_plus_tree_checkpoint_1_test[1]_include.cmake")
include("/home/zzy/CMU15-445/build-release/test/grading_b_plus_tree_checkpoint_2_concurrent_test[1]_include.cmake")
include("/home/zzy/CMU15-445/build-release/test/grading_b_plus_tree_checkpoint_2_sequential_test[1]_include.cmake")
include("/home/zzy/CMU15-445/build-release/test/tmp_tuple_page_test[1]_include.cmake")
include("/home/zzy/CMU15-445/build-release/test/tuple_test[1]_include.cmake")
include("/home/zzy/CMU15-445/build-release/test/type_test[1]_include.cmake")
add_test(SQLLogicTest.p3.01-seqscan "/home/zzy/CMU15-445/build-release/bin/bustub-sqllogictest" "/home/zzy/CMU15-445/test/sql/p3.01-seqscan.slt" "--verbose" "-d" "--in-memory")
set_tests_properties(SQLLogicTest.p3.01-seqscan PROPERTIES  _BACKTRACE_TRIPLES "/home/zzy/CMU15-445/test/CMakeLists.txt;83;add_test;/home/zzy/CMU15-445/test/CMakeLists.txt;0;")
add_test(SQLLogicTest.p3.02-insert "/home/zzy/CMU15-445/build-release/bin/bustub-sqllogictest" "/home/zzy/CMU15-445/test/sql/p3.02-insert.slt" "--verbose" "-d" "--in-memory")
set_tests_properties(SQLLogicTest.p3.02-insert PROPERTIES  _BACKTRACE_TRIPLES "/home/zzy/CMU15-445/test/CMakeLists.txt;83;add_test;/home/zzy/CMU15-445/test/CMakeLists.txt;0;")
add_test(SQLLogicTest.p3.03-delete "/home/zzy/CMU15-445/build-release/bin/bustub-sqllogictest" "/home/zzy/CMU15-445/test/sql/p3.03-delete.slt" "--verbose" "-d" "--in-memory")
set_tests_properties(SQLLogicTest.p3.03-delete PROPERTIES  _BACKTRACE_TRIPLES "/home/zzy/CMU15-445/test/CMakeLists.txt;83;add_test;/home/zzy/CMU15-445/test/CMakeLists.txt;0;")
add_test(SQLLogicTest.p3.04-index-scan "/home/zzy/CMU15-445/build-release/bin/bustub-sqllogictest" "/home/zzy/CMU15-445/test/sql/p3.04-index-scan.slt" "--verbose" "-d" "--in-memory")
set_tests_properties(SQLLogicTest.p3.04-index-scan PROPERTIES  _BACKTRACE_TRIPLES "/home/zzy/CMU15-445/test/CMakeLists.txt;83;add_test;/home/zzy/CMU15-445/test/CMakeLists.txt;0;")
add_test(SQLLogicTest.p3.05-empty-table "/home/zzy/CMU15-445/build-release/bin/bustub-sqllogictest" "/home/zzy/CMU15-445/test/sql/p3.05-empty-table.slt" "--verbose" "-d" "--in-memory")
set_tests_properties(SQLLogicTest.p3.05-empty-table PROPERTIES  _BACKTRACE_TRIPLES "/home/zzy/CMU15-445/test/CMakeLists.txt;83;add_test;/home/zzy/CMU15-445/test/CMakeLists.txt;0;")
add_test(SQLLogicTest.p3.06-simple-agg "/home/zzy/CMU15-445/build-release/bin/bustub-sqllogictest" "/home/zzy/CMU15-445/test/sql/p3.06-simple-agg.slt" "--verbose" "-d" "--in-memory")
set_tests_properties(SQLLogicTest.p3.06-simple-agg PROPERTIES  _BACKTRACE_TRIPLES "/home/zzy/CMU15-445/test/CMakeLists.txt;83;add_test;/home/zzy/CMU15-445/test/CMakeLists.txt;0;")
add_test(SQLLogicTest.p3.07-group-agg-1 "/home/zzy/CMU15-445/build-release/bin/bustub-sqllogictest" "/home/zzy/CMU15-445/test/sql/p3.07-group-agg-1.slt" "--verbose" "-d" "--in-memory")
set_tests_properties(SQLLogicTest.p3.07-group-agg-1 PROPERTIES  _BACKTRACE_TRIPLES "/home/zzy/CMU15-445/test/CMakeLists.txt;83;add_test;/home/zzy/CMU15-445/test/CMakeLists.txt;0;")
add_test(SQLLogicTest.p3.08-group-agg-2 "/home/zzy/CMU15-445/build-release/bin/bustub-sqllogictest" "/home/zzy/CMU15-445/test/sql/p3.08-group-agg-2.slt" "--verbose" "-d" "--in-memory")
set_tests_properties(SQLLogicTest.p3.08-group-agg-2 PROPERTIES  _BACKTRACE_TRIPLES "/home/zzy/CMU15-445/test/CMakeLists.txt;83;add_test;/home/zzy/CMU15-445/test/CMakeLists.txt;0;")
add_test(SQLLogicTest.p3.09-simple-join "/home/zzy/CMU15-445/build-release/bin/bustub-sqllogictest" "/home/zzy/CMU15-445/test/sql/p3.09-simple-join.slt" "--verbose" "-d" "--in-memory")
set_tests_properties(SQLLogicTest.p3.09-simple-join PROPERTIES  _BACKTRACE_TRIPLES "/home/zzy/CMU15-445/test/CMakeLists.txt;83;add_test;/home/zzy/CMU15-445/test/CMakeLists.txt;0;")
add_test(SQLLogicTest.p3.10-multi-way-join "/home/zzy/CMU15-445/build-release/bin/bustub-sqllogictest" "/home/zzy/CMU15-445/test/sql/p3.10-multi-way-join.slt" "--verbose" "-d" "--in-memory")
set_tests_properties(SQLLogicTest.p3.10-multi-way-join PROPERTIES  _BACKTRACE_TRIPLES "/home/zzy/CMU15-445/test/CMakeLists.txt;83;add_test;/home/zzy/CMU15-445/test/CMakeLists.txt;0;")
add_test(SQLLogicTest.p3.11-repeat-execute "/home/zzy/CMU15-445/build-release/bin/bustub-sqllogictest" "/home/zzy/CMU15-445/test/sql/p3.11-repeat-execute.slt" "--verbose" "-d" "--in-memory")
set_tests_properties(SQLLogicTest.p3.11-repeat-execute PROPERTIES  _BACKTRACE_TRIPLES "/home/zzy/CMU15-445/test/CMakeLists.txt;83;add_test;/home/zzy/CMU15-445/test/CMakeLists.txt;0;")
add_test(SQLLogicTest.p3.12-nested-index-join "/home/zzy/CMU15-445/build-release/bin/bustub-sqllogictest" "/home/zzy/CMU15-445/test/sql/p3.12-nested-index-join.slt" "--verbose" "-d" "--in-memory")
set_tests_properties(SQLLogicTest.p3.12-nested-index-join PROPERTIES  _BACKTRACE_TRIPLES "/home/zzy/CMU15-445/test/CMakeLists.txt;83;add_test;/home/zzy/CMU15-445/test/CMakeLists.txt;0;")
add_test(SQLLogicTest.p3.13-sort-limit "/home/zzy/CMU15-445/build-release/bin/bustub-sqllogictest" "/home/zzy/CMU15-445/test/sql/p3.13-sort-limit.slt" "--verbose" "-d" "--in-memory")
set_tests_properties(SQLLogicTest.p3.13-sort-limit PROPERTIES  _BACKTRACE_TRIPLES "/home/zzy/CMU15-445/test/CMakeLists.txt;83;add_test;/home/zzy/CMU15-445/test/CMakeLists.txt;0;")
add_test(SQLLogicTest.p3.14-topn "/home/zzy/CMU15-445/build-release/bin/bustub-sqllogictest" "/home/zzy/CMU15-445/test/sql/p3.14-topn.slt" "--verbose" "-d" "--in-memory")
set_tests_properties(SQLLogicTest.p3.14-topn PROPERTIES  _BACKTRACE_TRIPLES "/home/zzy/CMU15-445/test/CMakeLists.txt;83;add_test;/home/zzy/CMU15-445/test/CMakeLists.txt;0;")
add_test(SQLLogicTest.p3.15-integration-1 "/home/zzy/CMU15-445/build-release/bin/bustub-sqllogictest" "/home/zzy/CMU15-445/test/sql/p3.15-integration-1.slt" "--verbose" "-d" "--in-memory")
set_tests_properties(SQLLogicTest.p3.15-integration-1 PROPERTIES  _BACKTRACE_TRIPLES "/home/zzy/CMU15-445/test/CMakeLists.txt;83;add_test;/home/zzy/CMU15-445/test/CMakeLists.txt;0;")
add_test(SQLLogicTest.p3.16-integration-2 "/home/zzy/CMU15-445/build-release/bin/bustub-sqllogictest" "/home/zzy/CMU15-445/test/sql/p3.16-integration-2.slt" "--verbose" "-d" "--in-memory")
set_tests_properties(SQLLogicTest.p3.16-integration-2 PROPERTIES  _BACKTRACE_TRIPLES "/home/zzy/CMU15-445/test/CMakeLists.txt;83;add_test;/home/zzy/CMU15-445/test/CMakeLists.txt;0;")
add_test(SQLLogicTest.p3.leaderboard-q1 "/home/zzy/CMU15-445/build-release/bin/bustub-sqllogictest" "/home/zzy/CMU15-445/test/sql/p3.leaderboard-q1.slt" "--verbose" "-d" "--in-memory")
set_tests_properties(SQLLogicTest.p3.leaderboard-q1 PROPERTIES  _BACKTRACE_TRIPLES "/home/zzy/CMU15-445/test/CMakeLists.txt;83;add_test;/home/zzy/CMU15-445/test/CMakeLists.txt;0;")
add_test(SQLLogicTest.p3.leaderboard-q2 "/home/zzy/CMU15-445/build-release/bin/bustub-sqllogictest" "/home/zzy/CMU15-445/test/sql/p3.leaderboard-q2.slt" "--verbose" "-d" "--in-memory")
set_tests_properties(SQLLogicTest.p3.leaderboard-q2 PROPERTIES  _BACKTRACE_TRIPLES "/home/zzy/CMU15-445/test/CMakeLists.txt;83;add_test;/home/zzy/CMU15-445/test/CMakeLists.txt;0;")
add_test(SQLLogicTest.p3.leaderboard-q3 "/home/zzy/CMU15-445/build-release/bin/bustub-sqllogictest" "/home/zzy/CMU15-445/test/sql/p3.leaderboard-q3.slt" "--verbose" "-d" "--in-memory")
set_tests_properties(SQLLogicTest.p3.leaderboard-q3 PROPERTIES  _BACKTRACE_TRIPLES "/home/zzy/CMU15-445/test/CMakeLists.txt;83;add_test;/home/zzy/CMU15-445/test/CMakeLists.txt;0;")
