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
# -----------------------------------------------------------------------------
# Thư viện .so PHẢI nằm ở libdir, KHÔNG phải /usr/bin.
# Bản cũ đổ cả hai .so vào /usr/bin, vi phạm FHS §4.4 (/usr/bin chỉ chứa lệnh
# thực thi). Hệ quả thực tế: ldconfig không quét /usr/bin nên không app nào
# link động được tới chúng, và lintian bắn shared-library-in-bindir.
#
# Dùng biến GNUInstallDirs thay vì đường dẫn tuyệt đối để bản build có
# CMAKE_INSTALL_PREFIX khác (ví dụ /usr/local khi thử nghiệm) không đổ tệp
# thẳng vào hệ thống thật.
include(GNUInstallDirs)

install(TARGETS archive_engine iso_mount
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(TARGETS tizenos-mount-iso
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
