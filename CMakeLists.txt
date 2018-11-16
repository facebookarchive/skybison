cmake_minimum_required(VERSION 3.8)
project(python)

set(PYTHON_EXECUTABLE /usr/bin/python)

set(CMAKE_CXX_STANDARD 11)

add_subdirectory(third-party/googletest-1.8.0)
include_directories(third-party/googletest-1.8.0/googletest/include)
include_directories(third-party/googletest-1.8.0/googlemock/include)

#set(SOURCE_FILES main.cpp runtime/Heap.cpp runtime/Heap.h runtime/Handle.cpp runtime/Handle.h runtime/Runtime.cpp runtime/Runtime.h runtime/Globals.h runtime/Object.h runtime/Marshal.cpp runtime/Marshal.h)
set(SOURCE_FILES runtime/Heap.cpp runtime/Heap.h runtime/Handle.cpp runtime/Handle.h runtime/Runtime.cpp runtime/Runtime.h runtime/Globals.h runtime/Object.h runtime/Marshal.cpp runtime/Marshal.h runtime/Utils.h)

add_executable(python main.cpp ${SOURCE_FILES})

set(TEST_FILES runtime/MarshalTest.cpp runtime/Thread.cpp runtime/Thread.h runtime/Interpreter.cpp runtime/Interpreter.h runtime/Bytecode.h runtime/Frame.h runtime/ThreadTest.cpp)
add_executable(python-tests ${SOURCE_FILES} ${TEST_FILES})
target_link_libraries(python-tests gtest gtest_main)