cmake_minimum_required(VERSION 3.10)
include(FetchContent)

project(
  RedpilledBot
  VERSION 1.0
  DESCRIPTION "a silly twitch chat bot"
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_definitions(-DUSE_TLS=1)

file(GLOB_RECURSE BOT_SRC_FILES "bot/src/*.cpp" "bot/src/*.h" "bot/src/*.hpp")
file(GLOB_RECURSE WEB_SRC_FILES "web/src/*.cpp" "web/src/*.h" "web/src/*.hpp")

option(BUILD_BOT "Build the bot" ON)
option(BUILD_WEB "Build the web application" ON)

if (BUILD_BOT)
  add_executable(Bot)

  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
      target_compile_definitions(Bot PRIVATE DEBUG_MODE)
  endif()

  set_target_properties(
    Bot PROPERTIES
    DESCRIPTION ${PROJECT_DESCRIPTION}
    OUTPUT_NAME "redpilledbot"
  )

  target_sources(Bot PRIVATE ${BOT_SRC_FILES})

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

  FetchContent_Declare(
    ixwebsocket
    GIT_REPOSITORY https://github.com/machinezone/IXWebSocket
    GIT_TAG v11.4.5
  )
  FetchContent_MakeAvailable(ixwebsocket)

  target_link_libraries(Bot PRIVATE ixwebsocket::ixwebsocket pqxx nlohmann_json::nlohmann_json cpr::cpr)
endif()

if (BUILD_WEB)
  add_executable(Web)

  set_target_properties(
    Web PROPERTIES
    DESCRIPTION ${PROJECT_DESCRIPTION}
    OUTPUT_NAME "redpilledweb"
  )

  target_sources(Web PRIVATE ${WEB_SRC_FILES})

  # web framework
  FetchContent_Declare(
    crow
    GIT_REPOSITORY https://github.com/CrowCpp/Crow
    GIT_TAG v1.1.0
  )
  FetchContent_MakeAvailable(crow)

  # markdown parser
  FetchContent_Declare(
    maddy
    GIT_REPOSITORY https://github.com/progsource/maddy.git
    GIT_TAG 1.3.0
  )
  FetchContent_MakeAvailable(maddy)

  target_include_directories(Web PRIVATE ${crow_SOURCE_DIR}/include)
  target_link_libraries(Web PRIVATE maddy)
endif()
