cmake_minimum_required(VERSION 3.5)

project (systemtap)

add_library(systemtap INTERFACE)
target_include_directories(systemtap INTERFACE include)
