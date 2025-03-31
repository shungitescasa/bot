cmake_minimum_required(VERSION 3.10)

project(
  RedpilledBot
  VERSION 1.0
  DESCRIPTION "a silly twitch chat bot"
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_BOT "Build the bot" ON)
option(BUILD_WEB "Build the web application" ON)

if (BUILD_BOT)
  add_subdirectory(bot)
endif()

if (BUILD_WEB)
  add_subdirectory(web)
endif()
