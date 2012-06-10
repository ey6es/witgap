cmake_minimum_required(VERSION 2.8)
project(WITGAP)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/etc/cmake/)

add_subdirectory(etc)
add_subdirectory(src/as)
add_subdirectory(src/cpp)
