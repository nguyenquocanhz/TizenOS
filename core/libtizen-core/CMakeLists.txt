cmake_minimum_required(VERSION 3.10)
project(tizen-core VERSION 1.0.0 LANGUAGES C)

# C99/C11 standards
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB REQUIRED glib-2.0)
pkg_check_modules(LIBSYSTEMD REQUIRED libsystemd)

include_directories(include ${GLIB_INCLUDE_DIRS} ${LIBSYSTEMD_INCLUDE_DIRS})

set(SOURCES
    src/core.c
    src/log.c
    src/utils.c
)

add_library(tizen-core SHARED ${SOURCES})
target_link_libraries(tizen-core ${GLIB_LIBRARIES} ${LIBSYSTEMD_LIBRARIES})
set_target_properties(tizen-core PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION 1)

# Install
install(TARGETS tizen-core LIBRARY DESTINATION lib)
install(DIRECTORY include/tizen DESTINATION include)

# Pkg-config
configure_file(tizen-core.pc.in tizen-core.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/tizen-core.pc DESTINATION lib/pkgconfig)
