cmake_minimum_required(VERSION 3.10)

project(
  RedpilledBot
  VERSION 1.0
  DESCRIPTION "a silly twitch chat bot"
)

function(create_symlink_if_exists source target)
    if(EXISTS "${source}")
        message(STATUS "Creating symlink ${source} -> ${target}")
        execute_process(COMMAND ${CMAKE_COMMAND} -E remove -f "${target}")
        execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink "${source}" "${target}")
    else()
        message(WARNING "Source '${source}' does not exist. Skipping symlink creation.")
    endif()
endfunction()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_BOT "Build the bot" ON)
option(BUILD_WEB "Build the web application" ON)

if (BUILD_BOT)
  set(LUA_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/lib/lua/src")
  add_subdirectory(lib/lua)
  add_subdirectory(bot)
endif()

if (BUILD_WEB)
  add_subdirectory(web)
endif()
