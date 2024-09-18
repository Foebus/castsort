cmake_minimum_required(VERSION 3.17)
project(InstaSort)

set(CMAKE_C_STANDARD 11)

add_executable(InstaSort main.c)
add_executable(CastSort main.c)

target_link_libraries(InstaSort m)
target_link_libraries(CastSort m)
