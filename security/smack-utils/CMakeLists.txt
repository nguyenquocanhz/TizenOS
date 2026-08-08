cmake_minimum_required(VERSION 3.10)
project(smack-utils)

add_library(smack-util SHARED smack-labels.c)
add_executable(smack-manager smack-manager.c)
target_link_libraries(smack-manager smack-util)

install(TARGETS smack-manager DESTINATION bin)
install(TARGETS smack-util DESTINATION lib)
install(FILES config/smack-default.rules DESTINATION /etc/smack/)
