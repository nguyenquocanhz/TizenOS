cmake_minimum_required(VERSION 3.16)
project(tizenos-benchmark-launchpad VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)

add_executable(tizenos-benchmark-launchpad src/main.c)

install(TARGETS tizenos-benchmark-launchpad DESTINATION bin)
