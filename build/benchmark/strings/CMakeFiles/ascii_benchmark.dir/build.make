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
include benchmark/strings/CMakeFiles/ascii_benchmark.dir/depend.make

# Include the progress variables for this target.
include benchmark/strings/CMakeFiles/ascii_benchmark.dir/progress.make

# Include the compile flags for this target's objects.
include benchmark/strings/CMakeFiles/ascii_benchmark.dir/flags.make

benchmark/strings/CMakeFiles/ascii_benchmark.dir/ascii_benchmark.cc.o: benchmark/strings/CMakeFiles/ascii_benchmark.dir/flags.make
benchmark/strings/CMakeFiles/ascii_benchmark.dir/ascii_benchmark.cc.o: ../benchmark/strings/ascii_benchmark.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/liyinbin/github/gottingen/abel/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object benchmark/strings/CMakeFiles/ascii_benchmark.dir/ascii_benchmark.cc.o"
	cd /Users/liyinbin/github/gottingen/abel/build/benchmark/strings && /Library/Developer/CommandLineTools/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/ascii_benchmark.dir/ascii_benchmark.cc.o -c /Users/liyinbin/github/gottingen/abel/benchmark/strings/ascii_benchmark.cc

benchmark/strings/CMakeFiles/ascii_benchmark.dir/ascii_benchmark.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/ascii_benchmark.dir/ascii_benchmark.cc.i"
	cd /Users/liyinbin/github/gottingen/abel/build/benchmark/strings && /Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/liyinbin/github/gottingen/abel/benchmark/strings/ascii_benchmark.cc > CMakeFiles/ascii_benchmark.dir/ascii_benchmark.cc.i

benchmark/strings/CMakeFiles/ascii_benchmark.dir/ascii_benchmark.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/ascii_benchmark.dir/ascii_benchmark.cc.s"
	cd /Users/liyinbin/github/gottingen/abel/build/benchmark/strings && /Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/liyinbin/github/gottingen/abel/benchmark/strings/ascii_benchmark.cc -o CMakeFiles/ascii_benchmark.dir/ascii_benchmark.cc.s

# Object files for target ascii_benchmark
ascii_benchmark_OBJECTS = \
"CMakeFiles/ascii_benchmark.dir/ascii_benchmark.cc.o"

# External object files for target ascii_benchmark
ascii_benchmark_EXTERNAL_OBJECTS =

bin/ascii_benchmark: benchmark/strings/CMakeFiles/ascii_benchmark.dir/ascii_benchmark.cc.o
bin/ascii_benchmark: benchmark/strings/CMakeFiles/ascii_benchmark.dir/build.make
bin/ascii_benchmark: lib/libbenchmark.a
bin/ascii_benchmark: lib/libbenchmark_main.a
bin/ascii_benchmark: lib/libabel_static.a
bin/ascii_benchmark: lib/libbenchmark.a
bin/ascii_benchmark: benchmark/strings/CMakeFiles/ascii_benchmark.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/Users/liyinbin/github/gottingen/abel/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable ../../bin/ascii_benchmark"
	cd /Users/liyinbin/github/gottingen/abel/build/benchmark/strings && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/ascii_benchmark.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
benchmark/strings/CMakeFiles/ascii_benchmark.dir/build: bin/ascii_benchmark

.PHONY : benchmark/strings/CMakeFiles/ascii_benchmark.dir/build

benchmark/strings/CMakeFiles/ascii_benchmark.dir/clean:
	cd /Users/liyinbin/github/gottingen/abel/build/benchmark/strings && $(CMAKE_COMMAND) -P CMakeFiles/ascii_benchmark.dir/cmake_clean.cmake
.PHONY : benchmark/strings/CMakeFiles/ascii_benchmark.dir/clean

benchmark/strings/CMakeFiles/ascii_benchmark.dir/depend:
	cd /Users/liyinbin/github/gottingen/abel/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/liyinbin/github/gottingen/abel /Users/liyinbin/github/gottingen/abel/benchmark/strings /Users/liyinbin/github/gottingen/abel/build /Users/liyinbin/github/gottingen/abel/build/benchmark/strings /Users/liyinbin/github/gottingen/abel/build/benchmark/strings/CMakeFiles/ascii_benchmark.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : benchmark/strings/CMakeFiles/ascii_benchmark.dir/depend

