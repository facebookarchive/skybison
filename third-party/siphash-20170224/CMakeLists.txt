cmake_minimum_required(VERSION 2.6)
project(siphash)
add_library(
  siphash
  STATIC
  halfsiphash.c
  siphash.c
  include/siphash.h)
