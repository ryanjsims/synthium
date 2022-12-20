cmake_minimum_required(VERSION 3.12)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)

project(synthium CXX)

# ---------------------------------------------------------------------------------------
# Set SYNTHIUM_MASTER_PROJECT to ON if we are building spdlog
# ---------------------------------------------------------------------------------------
# Check if synthium is being used directly or via add_subdirectory, but allow overriding
if(NOT DEFINED SYNTHIUM_MASTER_PROJECT)
    if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
        set(SYNTHIUM_MASTER_PROJECT ON)
    else()
        set(SYNTHIUM_MASTER_PROJECT OFF)
    endif()
endif()

if(NOT TARGET spdlog)
find_package(spdlog REQUIRED)
endif()

add_subdirectory(external/zlib)
add_library(ZLIB::ZLIB ALIAS zlibstatic)

include(GenerateExportHeader)

add_library(synthium STATIC src/loader.cpp src/manager.cpp src/crc64.cpp src/utils.cpp include/synthium/synthium.h)
target_include_directories(synthium PUBLIC 
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>  
  $<INSTALL_INTERFACE:include>
  PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external/zlib/>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/external/zlib/>  
)
target_link_libraries(synthium PRIVATE spdlog::spdlog ZLIB::ZLIB)
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_link_libraries(synthium PRIVATE tbb)
endif()
set_target_properties(synthium PROPERTIES PUBLIC_HEADER "include/synthium/synthium.h;include/synthium/loader.h;include/synthium/manager.h;include/synthium/version.h")
set_target_properties(synthium PROPERTIES DEBUG_POSTFIX d)

find_package(Git)
add_custom_target(synthium_version
  ${CMAKE_COMMAND} -D SRC=${CMAKE_CURRENT_SOURCE_DIR}/include/synthium/version.h.in
                   -D DST=${CMAKE_CURRENT_SOURCE_DIR}/include/synthium/version.h
                   -D GIT_EXECUTABLE=${GIT_EXECUTABLE}
                   -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake-scripts/GenerateVersionHeader.cmake
  )
add_dependencies(synthium synthium_version)

if(GIT_EXECUTABLE)
  # Generate a git-describe version string from Git repository tags
  execute_process(
    COMMAND ${GIT_EXECUTABLE} describe --tags --dirty --match "v*"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_DESCRIBE_VERSION
    RESULT_VARIABLE GIT_DESCRIBE_ERROR_CODE
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  if(NOT GIT_DESCRIBE_ERROR_CODE)
    set(SYNTHIUM_VERSION ${GIT_DESCRIBE_VERSION})
  endif()
endif()

if(${SYNTHIUM_MASTER_PROJECT})
generate_export_header(synthium)

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

add_subdirectory(external)

install(
  TARGETS synthium 
  EXPORT synthiumTargets
  PUBLIC_HEADER DESTINATION include/synthium
)


set(ConfigPackageLocation lib/cmake/synthium)
include(CMakePackageConfigHelpers)
configure_package_config_file( 
  "cmake/synthiumConfig.cmake.in" 
  "synthiumConfig.cmake"
  INSTALL_DESTINATION ${ConfigPackageLocation}
  PATH_VARS
    ConfigPackageLocation
  )

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/synthium/synthiumConfigVersion.cmake"
  VERSION ${SYNTHIUM_VERSION}
  COMPATIBILITY AnyNewerVersion
)

#export(EXPORT synthiumTargets
#  FILE "${CMAKE_CURRENT_BINARY_DIR}/synthium/synthiumTargets.cmake"
#  NAMESPACE Upstream::
#)

install(EXPORT synthiumTargets
  FILE
    synthiumTargets.cmake
  NAMESPACE
    synthium::
  DESTINATION
    ${ConfigPackageLocation}
)
install(
  FILES
  "${CMAKE_CURRENT_BINARY_DIR}/synthiumConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/synthium/synthiumConfigVersion.cmake"
  DESTINATION
    ${ConfigPackageLocation}
  COMPONENT
    Devel
)
endif()

if(NOT TARGET synthium::synthium)
  add_library(synthium::synthium ALIAS synthium)
endif()