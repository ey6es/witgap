cmake_minimum_required(VERSION 2.8)
project(WITGAP)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/)
make_directory(${EXECUTABLE_OUTPUT_PATH})

add_subdirectory(src/as)
add_subdirectory(src/cpp)
