# ==============================================================================
# TizenOS Core System - Master CMake Build System
# ==============================================================================
# Build system chính cho toàn bộ các thành phần lõi TizenOS trên nền Debian 12
# Hỗ trợ biên dịch native x86_64 và cross-compile ARM64.
# ==============================================================================

cmake_minimum_required(VERSION 3.16)

project(tizenos-core-system
    VERSION 1.0.0
    DESCRIPTION "Debian-based TizenOS Core System"
    LANGUAGES C CXX
)

# ---- Standard C Configuration ------------------------------------------------
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# Compile flags bổ sung
add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
    -Wformat=2
    -Wconversion
    -fstack-protector-strong
    -D_GNU_SOURCE
)

# Default build type: Release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the build type" FORCE)
endif()

# ---- Target Architecture Detection ------------------------------------------
if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
    set(TIZENOS_ARCH "arm64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    set(TIZENOS_ARCH "x86_64")
else()
    set(TIZENOS_ARCH "${CMAKE_SYSTEM_PROCESSOR}")
endif()

message(STATUS "=== TizenOS Building for Target Architecture: ${TIZENOS_ARCH} (${CMAKE_BUILD_TYPE}) ===")

# ---- Package Options ---------------------------------------------------------
option(BUILD_BASE_GPU        "Build TizenOS Base GPU Management"       ON)
option(BUILD_CORE_LIBS       "Build TizenOS Core Libraries"           ON)
option(BUILD_SECURITY        "Build TizenOS Security Framework"        ON)
option(BUILD_APP_FRAMEWORK   "Build TizenOS Application Framework"     ON)
option(BUILD_PACKAGE_MGR     "Build TizenOS Package Manager"          ON)
option(BUILD_MULTIMEDIA      "Build TizenOS Multimedia Framework"      ON)
option(BUILD_SHELL           "Build TizenOS Desktop Shell"            ON)
option(BUILD_TOOLS           "Build TizenOS Tools (signFile, etc.)"   ON)
option(BUILD_DOCS            "Build Documentation"                    ON)

# ---- Subdirectories ----------------------------------------------------------

# 1. Base GPU Management
if(BUILD_BASE_GPU AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/base/gpu/CMakeLists.txt")
    message(STATUS "Adding Component: base/gpu")
    add_subdirectory(base/gpu)
endif()

# 2. Core Libraries (libtizen-core, libtizen-dbus)
if(BUILD_CORE_LIBS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/core/libtizen-core/CMakeLists.txt")
        message(STATUS "Adding Component: core/libtizen-core")
        add_subdirectory(core/libtizen-core)
    endif()

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/core/libtizen-dbus/CMakeLists.txt")
        message(STATUS "Adding Component: core/libtizen-dbus")
        add_subdirectory(core/libtizen-dbus)
    endif()
endif()

# 3. Security Framework (smack-utils, cynara, security-manager)
if(BUILD_SECURITY)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/security/smack-utils/CMakeLists.txt")
        message(STATUS "Adding Component: security/smack-utils")
        add_subdirectory(security/smack-utils)
    endif()

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/security/cynara/CMakeLists.txt")
        message(STATUS "Adding Component: security/cynara")
        add_subdirectory(security/cynara)
    endif()

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/security/security-manager/CMakeLists.txt")
        message(STATUS "Adding Component: security/security-manager")
        add_subdirectory(security/security-manager)
    endif()
endif()

# 4. Application Framework (nếu có)
if(BUILD_APP_FRAMEWORK AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/app-framework/CMakeLists.txt")
    message(STATUS "Adding Component: app-framework")
    add_subdirectory(app-framework)
endif()

# 5. Package Manager (nếu có)
if(BUILD_PACKAGE_MGR AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/package-manager/CMakeLists.txt")
    message(STATUS "Adding Component: package-manager")
    add_subdirectory(package-manager)
endif()

# 6. Multimedia Framework (nếu có)
if(BUILD_MULTIMEDIA AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/multimedia/CMakeLists.txt")
    message(STATUS "Adding Component: multimedia")
    add_subdirectory(multimedia)
endif()

# 7. Shell (nếu có)
if(BUILD_SHELL AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/shell/CMakeLists.txt")
    message(STATUS "Adding Component: shell")
    add_subdirectory(shell)
endif()

# 8. Tools (signFile & archive-mount)
if(BUILD_TOOLS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tools/signFile/CMakeLists.txt")
        message(STATUS "Adding Component: tools/signFile")
        add_subdirectory(tools/signFile)
    endif()
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tools/archive-mount/CMakeLists.txt")
        message(STATUS "Adding Component: tools/archive-mount")
        add_subdirectory(tools/archive-mount)
    endif()
endif()

# ---- Summary -----------------------------------------------------------------
message(STATUS "=== TizenOS Master Build Configuration Complete ===")
