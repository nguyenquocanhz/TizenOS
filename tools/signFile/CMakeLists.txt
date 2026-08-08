cmake_minimum_required(VERSION 3.16)
project(signFile VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(OpenSSL 3.0 REQUIRED)

include_directories(${OPENSSL_INCLUDE_DIR})

add_executable(signFile src/main.c)

target_link_libraries(signFile
    PRIVATE
    OpenSSL::Crypto
    OpenSSL::SSL
)

install(TARGETS signFile DESTINATION bin)
