cmake_minimum_required(VERSION 3.10)
include(FetchContent)

add_executable(SingleBot)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  set(BUILD_SHARED_LIBS OFF)
  target_link_options(SingleBot PRIVATE -static-libgcc -static-libstdc++)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(SingleBot PRIVATE DEBUG_MODE DEFAULT_PREFIX="~")
else()
    target_compile_definitions(SingleBot PRIVATE DEFAULT_PREFIX="!")
endif()

string(TIMESTAMP PROJECT_COMPILE_TIME "%s")

target_compile_definitions(SingleBot PRIVATE
  BOT_VERSION="${BOT_VERSION_STRING}"
  BOT_COMPILED_TIMESTAMP=${PROJECT_COMPILE_TIME}
  DEFAULT_LOCALE_ID="english"
)

set_target_properties(
  SingleBot PROPERTIES
  DESCRIPTION ${PROJECT_DESCRIPTION}
  OUTPUT_NAME "${PROJECT_NAME}-single"
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin
)

file(GLOB_RECURSE SOURCE_FILES "src/*.cpp" "src/*.c" "src/*.h" "src/*.hpp")

target_sources(SingleBot PRIVATE ${SOURCE_FILES})
target_include_directories(SingleBot PRIVATE src)

# ---------
# Libraries
# ---------
target_link_libraries(SingleBot PRIVATE
    Core
    lua
    ScriptLib
)

if (NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/bin")
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")
endif()

# Creating symbolic links
create_symlink_if_exists("${CMAKE_SOURCE_DIR}/localization" "${CMAKE_CURRENT_BINARY_DIR}/bin/localization")
create_symlink_if_exists("${CMAKE_SOURCE_DIR}/luascripts" "${CMAKE_CURRENT_BINARY_DIR}/bin/luascripts")

if (EXISTS "${CMAKE_SOURCE_DIR}/.env")
  create_symlink_if_exists("${CMAKE_SOURCE_DIR}/.env" "${CMAKE_CURRENT_BINARY_DIR}/bin/.env")
endif()