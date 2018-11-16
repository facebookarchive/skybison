cmake_minimum_required(VERSION 3.0)

project(siphash)

add_library(
  siphash
  STATIC
  halfsiphash.c
  siphash.c)

target_include_directories(
  siphash
  PUBLIC
  include)
