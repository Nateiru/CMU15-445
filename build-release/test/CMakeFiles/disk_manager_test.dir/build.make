# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.16

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/zzy/CMU15-445

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/zzy/CMU15-445/build-release

# Include any dependencies generated for this target.
include test/CMakeFiles/disk_manager_test.dir/depend.make

# Include the progress variables for this target.
include test/CMakeFiles/disk_manager_test.dir/progress.make

# Include the compile flags for this target's objects.
include test/CMakeFiles/disk_manager_test.dir/flags.make

test/CMakeFiles/disk_manager_test.dir/storage/disk_manager_test.cpp.o: test/CMakeFiles/disk_manager_test.dir/flags.make
test/CMakeFiles/disk_manager_test.dir/storage/disk_manager_test.cpp.o: ../test/storage/disk_manager_test.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/zzy/CMU15-445/build-release/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object test/CMakeFiles/disk_manager_test.dir/storage/disk_manager_test.cpp.o"
	cd /home/zzy/CMU15-445/build-release/test && /usr/bin/clang++-12  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/disk_manager_test.dir/storage/disk_manager_test.cpp.o -c /home/zzy/CMU15-445/test/storage/disk_manager_test.cpp

test/CMakeFiles/disk_manager_test.dir/storage/disk_manager_test.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/disk_manager_test.dir/storage/disk_manager_test.cpp.i"
	cd /home/zzy/CMU15-445/build-release/test && /usr/bin/clang++-12 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/zzy/CMU15-445/test/storage/disk_manager_test.cpp > CMakeFiles/disk_manager_test.dir/storage/disk_manager_test.cpp.i

test/CMakeFiles/disk_manager_test.dir/storage/disk_manager_test.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/disk_manager_test.dir/storage/disk_manager_test.cpp.s"
	cd /home/zzy/CMU15-445/build-release/test && /usr/bin/clang++-12 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/zzy/CMU15-445/test/storage/disk_manager_test.cpp -o CMakeFiles/disk_manager_test.dir/storage/disk_manager_test.cpp.s

# Object files for target disk_manager_test
disk_manager_test_OBJECTS = \
"CMakeFiles/disk_manager_test.dir/storage/disk_manager_test.cpp.o"

# External object files for target disk_manager_test
disk_manager_test_EXTERNAL_OBJECTS =

test/disk_manager_test: test/CMakeFiles/disk_manager_test.dir/storage/disk_manager_test.cpp.o
test/disk_manager_test: test/CMakeFiles/disk_manager_test.dir/build.make
test/disk_manager_test: lib/libbustub.a
test/disk_manager_test: lib/libgtest.a
test/disk_manager_test: lib/libgmock_main.a
test/disk_manager_test: lib/libbustub_murmur3.a
test/disk_manager_test: lib/libduckdb_pg_query.a
test/disk_manager_test: lib/libfmt.a
test/disk_manager_test: lib/libfort.a
test/disk_manager_test: lib/libgmock.a
test/disk_manager_test: lib/libgtest.a
test/disk_manager_test: test/CMakeFiles/disk_manager_test.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/zzy/CMU15-445/build-release/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable disk_manager_test"
	cd /home/zzy/CMU15-445/build-release/test && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/disk_manager_test.dir/link.txt --verbose=$(VERBOSE)
	cd /home/zzy/CMU15-445/build-release/test && /usr/bin/cmake -D TEST_TARGET=disk_manager_test -D TEST_EXECUTABLE=/home/zzy/CMU15-445/build-release/test/disk_manager_test -D TEST_EXECUTOR= -D TEST_WORKING_DIR=/home/zzy/CMU15-445/build-release/test -D "TEST_EXTRA_ARGS=--gtest_color=auto;--gtest_output=xml:/home/zzy/CMU15-445/build-release/test/disk_manager_test.xml;--gtest_catch_exceptions=0" -D "TEST_PROPERTIES=TIMEOUT;120" -D TEST_PREFIX= -D TEST_SUFFIX= -D NO_PRETTY_TYPES=FALSE -D NO_PRETTY_VALUES=FALSE -D TEST_LIST=disk_manager_test_TESTS -D CTEST_FILE=/home/zzy/CMU15-445/build-release/test/disk_manager_test[1]_tests.cmake -D TEST_DISCOVERY_TIMEOUT=120 -P /usr/share/cmake-3.16/Modules/GoogleTestAddTests.cmake

# Rule to build all files generated by this target.
test/CMakeFiles/disk_manager_test.dir/build: test/disk_manager_test

.PHONY : test/CMakeFiles/disk_manager_test.dir/build

test/CMakeFiles/disk_manager_test.dir/clean:
	cd /home/zzy/CMU15-445/build-release/test && $(CMAKE_COMMAND) -P CMakeFiles/disk_manager_test.dir/cmake_clean.cmake
.PHONY : test/CMakeFiles/disk_manager_test.dir/clean

test/CMakeFiles/disk_manager_test.dir/depend:
	cd /home/zzy/CMU15-445/build-release && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/zzy/CMU15-445 /home/zzy/CMU15-445/test /home/zzy/CMU15-445/build-release /home/zzy/CMU15-445/build-release/test /home/zzy/CMU15-445/build-release/test/CMakeFiles/disk_manager_test.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : test/CMakeFiles/disk_manager_test.dir/depend

