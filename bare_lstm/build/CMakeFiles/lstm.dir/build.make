# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.26

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
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
RM = /usr/local/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/peter/bluefield_dpdk/bare_lstm

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/peter/bluefield_dpdk/bare_lstm/build

# Include any dependencies generated for this target.
include CMakeFiles/lstm.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/lstm.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/lstm.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/lstm.dir/flags.make

CMakeFiles/lstm.dir/lstm.cpp.o: CMakeFiles/lstm.dir/flags.make
CMakeFiles/lstm.dir/lstm.cpp.o: /home/peter/bluefield_dpdk/bare_lstm/lstm.cpp
CMakeFiles/lstm.dir/lstm.cpp.o: CMakeFiles/lstm.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/peter/bluefield_dpdk/bare_lstm/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/lstm.dir/lstm.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/lstm.dir/lstm.cpp.o -MF CMakeFiles/lstm.dir/lstm.cpp.o.d -o CMakeFiles/lstm.dir/lstm.cpp.o -c /home/peter/bluefield_dpdk/bare_lstm/lstm.cpp

CMakeFiles/lstm.dir/lstm.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/lstm.dir/lstm.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/peter/bluefield_dpdk/bare_lstm/lstm.cpp > CMakeFiles/lstm.dir/lstm.cpp.i

CMakeFiles/lstm.dir/lstm.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/lstm.dir/lstm.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/peter/bluefield_dpdk/bare_lstm/lstm.cpp -o CMakeFiles/lstm.dir/lstm.cpp.s

# Object files for target lstm
lstm_OBJECTS = \
"CMakeFiles/lstm.dir/lstm.cpp.o"

# External object files for target lstm
lstm_EXTERNAL_OBJECTS =

lstm: CMakeFiles/lstm.dir/lstm.cpp.o
lstm: CMakeFiles/lstm.dir/build.make
lstm: /home/peter/libtorch/lib/libtorch.so
lstm: /home/peter/libtorch/lib/libc10.so
lstm: /home/peter/libtorch/lib/libkineto.a
lstm: /home/peter/libtorch/lib/libc10.so
lstm: CMakeFiles/lstm.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/peter/bluefield_dpdk/bare_lstm/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable lstm"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/lstm.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/lstm.dir/build: lstm
.PHONY : CMakeFiles/lstm.dir/build

CMakeFiles/lstm.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/lstm.dir/cmake_clean.cmake
.PHONY : CMakeFiles/lstm.dir/clean

CMakeFiles/lstm.dir/depend:
	cd /home/peter/bluefield_dpdk/bare_lstm/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/peter/bluefield_dpdk/bare_lstm /home/peter/bluefield_dpdk/bare_lstm /home/peter/bluefield_dpdk/bare_lstm/build /home/peter/bluefield_dpdk/bare_lstm/build /home/peter/bluefield_dpdk/bare_lstm/build/CMakeFiles/lstm.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/lstm.dir/depend
