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
include test/base/CMakeFiles/base_spinlock_test_common.dir/depend.make

# Include the progress variables for this target.
include test/base/CMakeFiles/base_spinlock_test_common.dir/progress.make

# Include the compile flags for this target's objects.
include test/base/CMakeFiles/base_spinlock_test_common.dir/flags.make

test/base/CMakeFiles/base_spinlock_test_common.dir/spinlock_test_common.cc.o: test/base/CMakeFiles/base_spinlock_test_common.dir/flags.make
test/base/CMakeFiles/base_spinlock_test_common.dir/spinlock_test_common.cc.o: ../test/base/spinlock_test_common.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/liyinbin/github/gottingen/abel/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object test/base/CMakeFiles/base_spinlock_test_common.dir/spinlock_test_common.cc.o"
	cd /Users/liyinbin/github/gottingen/abel/build/test/base && /Library/Developer/CommandLineTools/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/base_spinlock_test_common.dir/spinlock_test_common.cc.o -c /Users/liyinbin/github/gottingen/abel/test/base/spinlock_test_common.cc

test/base/CMakeFiles/base_spinlock_test_common.dir/spinlock_test_common.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/base_spinlock_test_common.dir/spinlock_test_common.cc.i"
	cd /Users/liyinbin/github/gottingen/abel/build/test/base && /Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/liyinbin/github/gottingen/abel/test/base/spinlock_test_common.cc > CMakeFiles/base_spinlock_test_common.dir/spinlock_test_common.cc.i

test/base/CMakeFiles/base_spinlock_test_common.dir/spinlock_test_common.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/base_spinlock_test_common.dir/spinlock_test_common.cc.s"
	cd /Users/liyinbin/github/gottingen/abel/build/test/base && /Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/liyinbin/github/gottingen/abel/test/base/spinlock_test_common.cc -o CMakeFiles/base_spinlock_test_common.dir/spinlock_test_common.cc.s

# Object files for target base_spinlock_test_common
base_spinlock_test_common_OBJECTS = \
"CMakeFiles/base_spinlock_test_common.dir/spinlock_test_common.cc.o"

# External object files for target base_spinlock_test_common
base_spinlock_test_common_EXTERNAL_OBJECTS =

bin/base_spinlock_test_common: test/base/CMakeFiles/base_spinlock_test_common.dir/spinlock_test_common.cc.o
bin/base_spinlock_test_common: test/base/CMakeFiles/base_spinlock_test_common.dir/build.make
bin/base_spinlock_test_common: lib/libgtestd.a
bin/base_spinlock_test_common: lib/libgmockd.a
bin/base_spinlock_test_common: lib/libgtest_maind.a
bin/base_spinlock_test_common: lib/libtesting_static.a
bin/base_spinlock_test_common: lib/libabel_static.a
bin/base_spinlock_test_common: lib/libgmockd.a
bin/base_spinlock_test_common: lib/libgtestd.a
bin/base_spinlock_test_common: test/base/CMakeFiles/base_spinlock_test_common.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/Users/liyinbin/github/gottingen/abel/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable ../../bin/base_spinlock_test_common"
	cd /Users/liyinbin/github/gottingen/abel/build/test/base && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/base_spinlock_test_common.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
test/base/CMakeFiles/base_spinlock_test_common.dir/build: bin/base_spinlock_test_common

.PHONY : test/base/CMakeFiles/base_spinlock_test_common.dir/build

test/base/CMakeFiles/base_spinlock_test_common.dir/clean:
	cd /Users/liyinbin/github/gottingen/abel/build/test/base && $(CMAKE_COMMAND) -P CMakeFiles/base_spinlock_test_common.dir/cmake_clean.cmake
.PHONY : test/base/CMakeFiles/base_spinlock_test_common.dir/clean

test/base/CMakeFiles/base_spinlock_test_common.dir/depend:
	cd /Users/liyinbin/github/gottingen/abel/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/liyinbin/github/gottingen/abel /Users/liyinbin/github/gottingen/abel/test/base /Users/liyinbin/github/gottingen/abel/build /Users/liyinbin/github/gottingen/abel/build/test/base /Users/liyinbin/github/gottingen/abel/build/test/base/CMakeFiles/base_spinlock_test_common.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : test/base/CMakeFiles/base_spinlock_test_common.dir/depend

