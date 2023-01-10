cmake_minimum_required(VERSION 3.23)
project(cpp_gltf)

set(CMAKE_CXX_STANDARD 17)

set(source_dir ${CMAKE_CURRENT_SOURCE_DIR})
file(GLOB source_files ${source_dir}/*.cpp)
file(GLOB header_files ${source_dir}/*.h)

add_executable(cpp_gltf ${source_files} ${header_files})