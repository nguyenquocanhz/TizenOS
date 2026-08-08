cmake_minimum_required(VERSION 3.10)
project(tpk-tools)

find_package(PkgConfig REQUIRED)
pkg_check_modules(ZIP REQUIRED libzip)
pkg_check_modules(CRYPTO REQUIRED libcrypto)

include_directories(include ${ZIP_INCLUDE_DIRS})
add_executable(tpk-create src/tpk-create.c)
target_link_libraries(tpk-create ${ZIP_LIBRARIES})

add_executable(tpk-extract src/tpk-extract.c)
target_link_libraries(tpk-extract ${ZIP_LIBRARIES})

add_executable(tpk-verify src/tpk-verify.c)
target_link_libraries(tpk-verify ${CRYPTO_LIBRARIES})

install(TARGETS tpk-create tpk-extract tpk-verify DESTINATION bin)
