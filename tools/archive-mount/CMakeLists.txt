cmake_minimum_required(VERSION 3.10)
project(ArchiveMountEngine)

set(CMAKE_C_STANDARD 11)

include_directories(include)

# Tìm libarchive
find_package(LibArchive REQUIRED)

# Thư viện dùng chung
add_library(archive_engine SHARED src/archive-engine.c)
target_link_libraries(archive_engine ${LibArchive_LIBRARIES})

add_library(iso_mount SHARED src/iso-mount.c)

# CLI tool
add_executable(tizenos-mount-iso src/iso-mount-cli.c)
target_link_libraries(tizenos-mount-iso iso_mount)

# Cài đặt
install(TARGETS archive_engine iso_mount tizenos-mount-iso
        DESTINATION /usr/bin)
