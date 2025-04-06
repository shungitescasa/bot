cmake_minimum_required(VERSION 3.10)
include(FetchContent)

# Creating symbolic links
create_symlink_if_exists("${CMAKE_SOURCE_DIR}/localization" "${CMAKE_CURRENT_BINARY_DIR}/localization")
create_symlink_if_exists("${CMAKE_SOURCE_DIR}/luamods" "${CMAKE_CURRENT_BINARY_DIR}/luamods")

if (EXISTS "${CMAKE_SOURCE_DIR}/.env")
  create_symlink_if_exists("${CMAKE_SOURCE_DIR}/.env" "${CMAKE_CURRENT_BINARY_DIR}/.env")
endif()

add_executable(Bot)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(Bot PRIVATE DEBUG_MODE)
endif()

string(TIMESTAMP PROJECT_COMPILE_TIME "%s")

target_compile_definitions(Bot PRIVATE
  BOT_VERSION="${PROJECT_VERSION}"
  BOT_COMPILED_TIMESTAMP=${PROJECT_COMPILE_TIME}
)

set_target_properties(
  Bot PROPERTIES
  DESCRIPTION ${PROJECT_DESCRIPTION}
  OUTPUT_NAME "${PROJECT_NAME}-bot"
)

# TLS for websocket connections
set(USE_TLS ON CACHE BOOL "Enable TLS support")

file(GLOB_RECURSE SOURCE_FILES "src/*.cpp" "src/*.h" "src/*.hpp")

target_sources(Bot PRIVATE ${SOURCE_FILES})
target_include_directories(Bot PRIVATE src)

# Getting libraries

# json
FetchContent_Declare(
  json
  URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(json)

# http request maker
FetchContent_Declare(
  cpr
  GIT_REPOSITORY https://github.com/libcpr/cpr.git
  GIT_TAG 1.10.5
)
FetchContent_MakeAvailable(cpr)

# postgresql
FetchContent_Declare(
  pqxx
  GIT_REPOSITORY https://github.com/jtv/libpqxx.git
  GIT_TAG 7.9.2
)
FetchContent_MakeAvailable(pqxx)

# websockets
FetchContent_Declare(
  ixwebsocket
  GIT_REPOSITORY https://github.com/machinezone/IXWebSocket
  GIT_TAG v11.4.5
)
FetchContent_MakeAvailable(ixwebsocket)

set(SOL2_LUA_VERSION "5.4.8" CACHE STRING "The version of Lua used")
set(SOL2_BUILD_LUA FALSE CACHE BOOL "Always build Lua, do not search for it in the system")

FetchContent_Declare(
  sol
  GIT_REPOSITORY https://github.com/ThePhD/sol2.git
  GIT_TAG v3.5.0
)
FetchContent_MakeAvailable(sol)

target_link_libraries(Bot PRIVATE
    ixwebsocket::ixwebsocket
    pqxx
    nlohmann_json::nlohmann_json
    cpr::cpr
    lua
    sol2::sol2
)

