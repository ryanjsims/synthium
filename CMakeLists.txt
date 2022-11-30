cmake_minimum_required(VERSION 3.12)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)

project(synthium CXX)

if(NOT TARGET spdlog)
find_package(spdlog REQUIRED)
endif()

find_package(ZLIB REQUIRED)
find_package(Glob REQUIRED)

add_library(synthium STATIC src/loader.cpp src/manager.cpp src/crc64.cpp include/synthium.h)
target_include_directories(synthium PUBLIC include)
target_link_libraries(synthium PRIVATE spdlog::spdlog)
target_link_libraries(synthium PRIVATE ZLIB::ZLIB)

add_executable(test_crc64 src/test_crc64.cpp src/crc64.cpp)
target_include_directories(test_crc64 PUBLIC include)
target_link_libraries(test_crc64 PRIVATE spdlog::spdlog)

add_executable(test_pack2 src/test_pack2.cpp)
target_include_directories(test_pack2 PUBLIC include)
target_link_libraries(test_pack2 PRIVATE spdlog::spdlog)
target_link_libraries(test_pack2 PRIVATE synthium)
if(MSVC)
target_compile_options(test_pack2 PRIVATE /W4 /WX)
else()
target_compile_options(test_pack2 PRIVATE -Wall -Werror)
endif()

add_executable(test_manager src/test_manager.cpp)
target_include_directories(test_manager PUBLIC include)
target_link_libraries(test_manager PRIVATE spdlog::spdlog)
target_link_libraries(test_manager PRIVATE synthium)
target_link_libraries(test_manager PRIVATE Glob)
if(MSVC)
target_compile_options(test_manager PRIVATE /W4 /WX)
else()
target_compile_options(test_manager PRIVATE -Wall -Werror)
endif()

find_package(Git)
add_custom_target(version
  ${CMAKE_COMMAND} -D SRC=${CMAKE_SOURCE_DIR}/include/version.h.in
                   -D DST=${CMAKE_SOURCE_DIR}/include/version.h
                   -D GIT_EXECUTABLE=${GIT_EXECUTABLE}
                   -P ${CMAKE_SOURCE_DIR}/cmake-scripts/GenerateVersionHeader.cmake
  )
add_dependencies(synthium version)