cmake_minimum_required(VERSION 3.10)
include(FetchContent)

# Creating symbolic links
create_symlink_if_exists("${CMAKE_SOURCE_DIR}/localization" "${CMAKE_CURRENT_BINARY_DIR}/localization")
create_symlink_if_exists("${CMAKE_SOURCE_DIR}/luascripts" "${CMAKE_CURRENT_BINARY_DIR}/luascripts")

if (EXISTS "${CMAKE_SOURCE_DIR}/.env")
  create_symlink_if_exists("${CMAKE_SOURCE_DIR}/.env" "${CMAKE_CURRENT_BINARY_DIR}/.env")
endif()

add_executable(Bot)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(Bot PRIVATE DEBUG_MODE DEFAULT_PREFIX="~")
else()
    target_compile_definitions(Bot PRIVATE DEFAULT_PREFIX="!")
endif()

string(TIMESTAMP PROJECT_COMPILE_TIME "%s")

target_compile_definitions(Bot PRIVATE
  BOT_VERSION="${BOT_VERSION_STRING}"
  BOT_COMPILED_TIMESTAMP=${PROJECT_COMPILE_TIME}
  DEFAULT_LOCALE_ID="english"
)

set_target_properties(
  Bot PROPERTIES
  DESCRIPTION ${PROJECT_DESCRIPTION}
  OUTPUT_NAME "${PROJECT_NAME}-bot"
)

# BetterTTV Emote support
set(BUILD_BETTERTTV OFF BOOL "Enable BetterTTV emotes")
if (BUILD_BETTERTTV)
  message("-- Bulding BetterTTV parts")
  target_compile_definitions(Bot PRIVATE BUILD_BETTERTTV=1)
endif()

# TLS for websocket connections
set(USE_TLS ON CACHE BOOL "Enable TLS support")

file(GLOB_RECURSE SOURCE_FILES "src/*.cpp" "src/*.h" "src/*.hpp")

target_sources(Bot PRIVATE ${SOURCE_FILES})
target_include_directories(Bot PRIVATE src)

# DATABASE
option(USE_POSTGRES "Use PostgreSQL backend" OFF)
option(USE_MARIADB "Use MariaDB backend" ON)

if (USE_POSTGRES)
  FetchContent_Declare(
    pqxx
    GIT_REPOSITORY https://github.com/jtv/libpqxx.git
    GIT_TAG 7.10.1
  )
  FetchContent_MakeAvailable(pqxx)
  target_compile_definitions(Bot PRIVATE USE_POSTGRES)
  target_link_libraries(Bot PRIVATE pqxx)
endif()

# ev&doe it is mysql
if (USE_MARIADB)
  target_compile_definitions(Bot PRIVATE USE_MARIADB)

  # searching for mysql
  find_program(MYSQL_CONFIG_EXECUTABLE mysql_config REQUIRED)

  execute_process(COMMAND ${MYSQL_CONFIG_EXECUTABLE} --cflags
                  OUTPUT_VARIABLE MYSQL_CFLAGS
                  OUTPUT_STRIP_TRAILING_WHITESPACE)

  execute_process(COMMAND ${MYSQL_CONFIG_EXECUTABLE} --libs
                  OUTPUT_VARIABLE MYSQL_LIBS
                  OUTPUT_STRIP_TRAILING_WHITESPACE)

  target_compile_options(Bot PRIVATE ${MYSQL_CFLAGS})
  target_link_libraries(Bot PRIVATE ${MYSQL_LIBS})
endif()

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

# websockets
FetchContent_Declare(
  ixwebsocket
  GIT_REPOSITORY https://github.com/machinezone/IXWebSocket
  GIT_TAG v11.4.6
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

# emotespp
FetchContent_Declare(
  emotespp
  GIT_REPOSITORY https://git.ilt.su/lib/emotespp.git
  GIT_TAG master
)
FetchContent_MakeAvailable(emotespp)

# xml
FetchContent_Declare(
  pugixml
  GIT_REPOSITORY https://github.com/zeux/pugixml.git
  GIT_TAG v1.15
)
FetchContent_MakeAvailable(pugixml)

# fmt
FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG 12.1.0
)
FetchContent_MakeAvailable(fmt)

target_link_libraries(Bot PRIVATE
    ixwebsocket::ixwebsocket
    nlohmann_json::nlohmann_json
    cpr::cpr
    lua
    sol2::sol2
    emotespp
    pugixml
    fmt
)

