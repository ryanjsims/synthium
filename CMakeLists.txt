cmake_minimum_required(VERSION 3.12)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)

project(pack2lib CXX)

if(NOT TARGET spdlog)
find_package(spdlog REQUIRED)
endif()

find_package(ZLIB REQUIRED)

add_library(pack2 STATIC src/loader.cpp include/pack2lib.h)
target_include_directories(pack2 PUBLIC include)
target_link_libraries(pack2 PRIVATE spdlog::spdlog)
target_link_libraries(pack2 PRIVATE ZLIB::ZLIB)

add_executable(test_crc64 src/test_crc64.cpp)
target_include_directories(test_crc64 PUBLIC include)
target_link_libraries(test_crc64 PRIVATE spdlog::spdlog)

add_executable(test_pack2 src/test_pack2.cpp)
target_include_directories(test_pack2 PUBLIC include)
target_link_libraries(test_pack2 PRIVATE spdlog::spdlog)
target_link_libraries(test_pack2 PRIVATE pack2)
if(MSVC)
target_compile_options(test_pack2 PRIVATE /W4 /WX)
else()
target_compile_options(test_pack2 PRIVATE -Wall -Werror)
endif()

find_package(Git)
add_custom_target(version
  ${CMAKE_COMMAND} -D SRC=${CMAKE_SOURCE_DIR}/include/version.h.in
                   -D DST=${CMAKE_SOURCE_DIR}/include/version.h
                   -D GIT_EXECUTABLE=${GIT_EXECUTABLE}
                   -P ${CMAKE_SOURCE_DIR}/cmake-scripts/GenerateVersionHeader.cmake
  )
add_dependencies(pack2 version)