cmake_minimum_required(VERSION 3.12)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)

project(pack2lib)

if(NOT TARGET spdlog)
find_package(spdlog REQUIRED)
endif()

add_library(pack2 STATIC src/loader.cpp)
target_include_directories(pack2 PUBLIC include)
target_link_libraries(pack2 PRIVATE spdlog::spdlog)

add_executable(test_crc64 src/test_crc64.cpp)
target_include_directories(test_crc64 PUBLIC include)
target_link_libraries(test_crc64 PRIVATE spdlog::spdlog)

find_package(Git)
add_custom_target(version
  ${CMAKE_COMMAND} -D SRC=${CMAKE_SOURCE_DIR}/include/version.h.in
                   -D DST=${CMAKE_SOURCE_DIR}/include/version.h
                   -D GIT_EXECUTABLE=${GIT_EXECUTABLE}
                   -P ${CMAKE_SOURCE_DIR}/cmake-scripts/GenerateVersionHeader.cmake
  )
add_dependencies(pack2 version)