cmake_minimum_required(VERSION 3.10)

project(
  RedpilledBot
  VERSION 1.0
  DESCRIPTION "a silly twitch chat bot"
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(Bot)

set_target_properties(
  Bot PROPERTIES
  DESCRIPTION ${PROJECT_DESCRIPTION}
  OUTPUT_NAME "redpilled-bot_${PROJECT_VERSION}"
)

file(GLOB_RECURSE SRC_FILES "src/*.cpp" "src/*.h" "src/*.hpp")

target_sources(Bot PRIVATE ${SRC_FILES})

include(FetchContent)

FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
FetchContent_MakeAvailable(json)

include(FetchContent)
FetchContent_Declare(cpr GIT_REPOSITORY https://github.com/libcpr/cpr.git
                         GIT_TAG 1.10.5)
FetchContent_MakeAvailable(cpr)

target_link_libraries(Bot PRIVATE ixwebsocket pqxx nlohmann_json::nlohmann_json cpr::cpr)
