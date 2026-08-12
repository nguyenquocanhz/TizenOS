cmake_minimum_required(VERSION 3.16)
project(tizen-multimedia)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/media-server/CMakeLists.txt")
    add_subdirectory(media-server)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/player/CMakeLists.txt")
    add_subdirectory(player)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/camera/CMakeLists.txt")
    add_subdirectory(camera)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/album/CMakeLists.txt")
    add_subdirectory(album)
endif()
