cmake_minimum_required(VERSION 3.10)

project(
  RedpilledBot
  VERSION 1.0
  DESCRIPTION "a silly twitch chat bot"
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(Bot)

set_target_properties(
  Bot PROPERTIES
  DESCRIPTION ${PROJECT_DESCRIPTION}
  OUTPUT_NAME "redpilled-bot_${PROJECT_VERSION}"
)

file(GLOB_RECURSE SRC_FILES "src/*.cpp" "src/*.h" "src/*.hpp")

target_sources(Bot PRIVATE ${SRC_FILES})

target_link_libraries(Bot PRIVATE ixwebsocket pqxx)
