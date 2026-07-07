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

# rpc
FetchContent_Declare(
  rpc
  GIT_REPOSITORY https://github.com/rpclib/rpclib.git
  GIT_TAG v2.3.0
)
FetchContent_MakeAvailable(rpc)

target_link_libraries(ScriptLib PUBLIC
  Core
  rpc
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