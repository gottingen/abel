# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.12

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
CMAKE_COMMAND = /usr/local/bin/cmake

# The command to remove a file.
RM = /usr/local/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/liyinbin/github/gottingen/abel

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/liyinbin/github/gottingen/abel/build

# Include any dependencies generated for this target.
include test/flags/CMakeFiles/flags_flag_test.dir/depend.make

# Include the progress variables for this target.
include test/flags/CMakeFiles/flags_flag_test.dir/progress.make

# Include the compile flags for this target's objects.
include test/flags/CMakeFiles/flags_flag_test.dir/flags.make

test/flags/CMakeFiles/flags_flag_test.dir/flag_test.cc.o: test/flags/CMakeFiles/flags_flag_test.dir/flags.make
test/flags/CMakeFiles/flags_flag_test.dir/flag_test.cc.o: ../test/flags/flag_test.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/liyinbin/github/gottingen/abel/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object test/flags/CMakeFiles/flags_flag_test.dir/flag_test.cc.o"
	cd /Users/liyinbin/github/gottingen/abel/build/test/flags && /Library/Developer/CommandLineTools/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/flags_flag_test.dir/flag_test.cc.o -c /Users/liyinbin/github/gottingen/abel/test/flags/flag_test.cc

test/flags/CMakeFiles/flags_flag_test.dir/flag_test.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/flags_flag_test.dir/flag_test.cc.i"
	cd /Users/liyinbin/github/gottingen/abel/build/test/flags && /Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/liyinbin/github/gottingen/abel/test/flags/flag_test.cc > CMakeFiles/flags_flag_test.dir/flag_test.cc.i

test/flags/CMakeFiles/flags_flag_test.dir/flag_test.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/flags_flag_test.dir/flag_test.cc.s"
	cd /Users/liyinbin/github/gottingen/abel/build/test/flags && /Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/liyinbin/github/gottingen/abel/test/flags/flag_test.cc -o CMakeFiles/flags_flag_test.dir/flag_test.cc.s

# Object files for target flags_flag_test
flags_flag_test_OBJECTS = \
"CMakeFiles/flags_flag_test.dir/flag_test.cc.o"

# External object files for target flags_flag_test
flags_flag_test_EXTERNAL_OBJECTS =

bin/flags_flag_test: test/flags/CMakeFiles/flags_flag_test.dir/flag_test.cc.o
bin/flags_flag_test: test/flags/CMakeFiles/flags_flag_test.dir/build.make
bin/flags_flag_test: lib/libgtestd.a
bin/flags_flag_test: lib/libgmockd.a
bin/flags_flag_test: lib/libgtest_maind.a
bin/flags_flag_test: lib/libtesting_static.a
bin/flags_flag_test: lib/libabel_static.a
bin/flags_flag_test: lib/libgmockd.a
bin/flags_flag_test: lib/libgtestd.a
bin/flags_flag_test: test/flags/CMakeFiles/flags_flag_test.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/Users/liyinbin/github/gottingen/abel/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable ../../bin/flags_flag_test"
	cd /Users/liyinbin/github/gottingen/abel/build/test/flags && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/flags_flag_test.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
test/flags/CMakeFiles/flags_flag_test.dir/build: bin/flags_flag_test

.PHONY : test/flags/CMakeFiles/flags_flag_test.dir/build

test/flags/CMakeFiles/flags_flag_test.dir/clean:
	cd /Users/liyinbin/github/gottingen/abel/build/test/flags && $(CMAKE_COMMAND) -P CMakeFiles/flags_flag_test.dir/cmake_clean.cmake
.PHONY : test/flags/CMakeFiles/flags_flag_test.dir/clean

test/flags/CMakeFiles/flags_flag_test.dir/depend:
	cd /Users/liyinbin/github/gottingen/abel/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/liyinbin/github/gottingen/abel /Users/liyinbin/github/gottingen/abel/test/flags /Users/liyinbin/github/gottingen/abel/build /Users/liyinbin/github/gottingen/abel/build/test/flags /Users/liyinbin/github/gottingen/abel/build/test/flags/CMakeFiles/flags_flag_test.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : test/flags/CMakeFiles/flags_flag_test.dir/depend

