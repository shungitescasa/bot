cmake_minimum_required(VERSION 3.10)
include(FetchContent)

file(GLOB_RECURSE SOURCE_FILES "src/*.cpp" "src/*.c" "src/*.h" "src/*.hpp")
list(REMOVE_ITEM SOURCE_FILES "src/main.cpp")

add_library(ScriptLib ${SOURCE_FILES})

target_include_directories(ScriptLib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# ---------
# Libraries
# ---------

# lua
target_include_directories(ScriptLib BEFORE PUBLIC ${LUA_INCLUDE_DIR})
target_link_libraries(ScriptLib PUBLIC lua)

set(SOL2_LUA_VERSION "5.4.0" CACHE STRING "The version of Lua used")
set(SOL2_BUILD_LUA FALSE CACHE BOOL "Always build Lua, do not search for it in the system")

FetchContent_Declare(
  sol
  GIT_REPOSITORY https://github.com/ThePhD/sol2.git
  GIT_TAG v3.5.0
)
FetchContent_MakeAvailable(sol)

target_link_libraries(ScriptLib PUBLIC
  Core
  sol2::sol2
)

# Script VM (executable)
add_executable(ScriptVM src/main.cpp)
set_target_properties(
  ScriptVM PROPERTIES
  DESCRIPTION ${PROJECT_DESCRIPTION}
  OUTPUT_NAME "scriptvm"
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin
)
target_link_libraries(ScriptVM PRIVATE Core ScriptLib)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(ScriptVM PRIVATE DEBUG_MODE)
endif()