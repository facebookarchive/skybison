cmake_minimum_required(VERSION 3.7)
project(python)

set(PYTHON_EXECUTABLE /usr/bin/python)

set(CMAKE_CXX_STANDARD 11)

add_subdirectory(third-party/googletest-1.8.0)
include_directories(third-party/googletest-1.8.0/googletest/include)
include_directories(third-party/googletest-1.8.0/googlemock/include)

set(SOURCE_FILES runtime/bytecode.h runtime/frame.h runtime/globals.h runtime/handle.cpp runtime/handle.h runtime/heap.cpp runtime/heap.h runtime/interpreter.cpp runtime/interpreter.h runtime/marshal.cpp runtime/marshal.h runtime/objects.h runtime/runtime.cpp runtime/runtime.h runtime/thread.cpp runtime/thread.h runtime/utils.h runtime/os.h runtime/os.cpp)

add_executable(python main.cpp ${SOURCE_FILES})

set(TEST_FILES runtime/marshal-test.cpp runtime/thread-test.cpp runtime/os-test.cpp runtime/heap-test.cpp)
add_executable(python-tests ${SOURCE_FILES} ${TEST_FILES})
target_link_libraries(python-tests gtest gtest_main)
