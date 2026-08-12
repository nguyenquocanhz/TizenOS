cmake_minimum_required(VERSION 3.16)
project(tizen-package-manager)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tizen-store/CMakeLists.txt")
    add_subdirectory(tizen-store)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tizen-app-manager/CMakeLists.txt")
    add_subdirectory(tizen-app-manager)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pkgmgr-client/CMakeLists.txt")
    add_subdirectory(pkgmgr-client)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pkgmgr-server/CMakeLists.txt")
    add_subdirectory(pkgmgr-server)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tpk-tools/CMakeLists.txt")
    add_subdirectory(tpk-tools)
endif()
