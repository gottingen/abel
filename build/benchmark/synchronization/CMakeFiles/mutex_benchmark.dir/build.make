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
include benchmark/synchronization/CMakeFiles/mutex_benchmark.dir/depend.make

# Include the progress variables for this target.
include benchmark/synchronization/CMakeFiles/mutex_benchmark.dir/progress.make

# Include the compile flags for this target's objects.
include benchmark/synchronization/CMakeFiles/mutex_benchmark.dir/flags.make

benchmark/synchronization/CMakeFiles/mutex_benchmark.dir/mutex_benchmark.cc.o: benchmark/synchronization/CMakeFiles/mutex_benchmark.dir/flags.make
benchmark/synchronization/CMakeFiles/mutex_benchmark.dir/mutex_benchmark.cc.o: ../benchmark/synchronization/mutex_benchmark.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/liyinbin/github/gottingen/abel/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object benchmark/synchronization/CMakeFiles/mutex_benchmark.dir/mutex_benchmark.cc.o"
	cd /Users/liyinbin/github/gottingen/abel/build/benchmark/synchronization && /Library/Developer/CommandLineTools/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/mutex_benchmark.dir/mutex_benchmark.cc.o -c /Users/liyinbin/github/gottingen/abel/benchmark/synchronization/mutex_benchmark.cc

benchmark/synchronization/CMakeFiles/mutex_benchmark.dir/mutex_benchmark.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/mutex_benchmark.dir/mutex_benchmark.cc.i"
	cd /Users/liyinbin/github/gottingen/abel/build/benchmark/synchronization && /Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/liyinbin/github/gottingen/abel/benchmark/synchronization/mutex_benchmark.cc > CMakeFiles/mutex_benchmark.dir/mutex_benchmark.cc.i

benchmark/synchronization/CMakeFiles/mutex_benchmark.dir/mutex_benchmark.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/mutex_benchmark.dir/mutex_benchmark.cc.s"
	cd /Users/liyinbin/github/gottingen/abel/build/benchmark/synchronization && /Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/liyinbin/github/gottingen/abel/benchmark/synchronization/mutex_benchmark.cc -o CMakeFiles/mutex_benchmark.dir/mutex_benchmark.cc.s

# Object files for target mutex_benchmark
mutex_benchmark_OBJECTS = \
"CMakeFiles/mutex_benchmark.dir/mutex_benchmark.cc.o"

# External object files for target mutex_benchmark
mutex_benchmark_EXTERNAL_OBJECTS =

bin/mutex_benchmark: benchmark/synchronization/CMakeFiles/mutex_benchmark.dir/mutex_benchmark.cc.o
bin/mutex_benchmark: benchmark/synchronization/CMakeFiles/mutex_benchmark.dir/build.make
bin/mutex_benchmark: lib/libbenchmark.a
bin/mutex_benchmark: lib/libbenchmark_main.a
bin/mutex_benchmark: lib/libabel_static.a
bin/mutex_benchmark: lib/libbenchmark.a
bin/mutex_benchmark: benchmark/synchronization/CMakeFiles/mutex_benchmark.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/Users/liyinbin/github/gottingen/abel/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable ../../bin/mutex_benchmark"
	cd /Users/liyinbin/github/gottingen/abel/build/benchmark/synchronization && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/mutex_benchmark.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
benchmark/synchronization/CMakeFiles/mutex_benchmark.dir/build: bin/mutex_benchmark

.PHONY : benchmark/synchronization/CMakeFiles/mutex_benchmark.dir/build

benchmark/synchronization/CMakeFiles/mutex_benchmark.dir/clean:
	cd /Users/liyinbin/github/gottingen/abel/build/benchmark/synchronization && $(CMAKE_COMMAND) -P CMakeFiles/mutex_benchmark.dir/cmake_clean.cmake
.PHONY : benchmark/synchronization/CMakeFiles/mutex_benchmark.dir/clean

benchmark/synchronization/CMakeFiles/mutex_benchmark.dir/depend:
	cd /Users/liyinbin/github/gottingen/abel/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/liyinbin/github/gottingen/abel /Users/liyinbin/github/gottingen/abel/benchmark/synchronization /Users/liyinbin/github/gottingen/abel/build /Users/liyinbin/github/gottingen/abel/build/benchmark/synchronization /Users/liyinbin/github/gottingen/abel/build/benchmark/synchronization/CMakeFiles/mutex_benchmark.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : benchmark/synchronization/CMakeFiles/mutex_benchmark.dir/depend

