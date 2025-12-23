cmake_minimum_required(VERSION 3.10)

project(
  RedpilledBot
  VERSION 1.0
  DESCRIPTION "a silly twitch chat bot"
)

function(create_symlink_if_exists source target)
    if(EXISTS "${source}")
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            message(STATUS "Creating symlink ${source} -> ${target}")
            execute_process(COMMAND ${CMAKE_COMMAND} -E remove -f "${target}")
            execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink "${source}" "${target}")
        else()
            message(STATUS "Copying ${source} -> ${target}")
            if(IS_DIRECTORY "${source}")
                execute_process(COMMAND ${CMAKE_COMMAND} -E remove_directory "${target}")
                execute_process(COMMAND ${CMAKE_COMMAND} -E copy_directory "${source}" "${target}")
            else()
                execute_process(COMMAND ${CMAKE_COMMAND} -E remove -f "${target}")
                configure_file("${source}" "${target}" COPYONLY)
            endif()
        endif()
    else()
        message(WARNING "Source '${source}' does not exist. Skipping symlink creation.")
    endif()
endfunction()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# getting Git information
find_package(Git QUIET)
if (GIT_FOUND)
    # get git tag
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_TAG
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if(NOT GIT_TAG)
        execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_COMMIT_HASH
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        set(BOT_VERSION_STRING "commit ${GIT_COMMIT_HASH}")
    else()
        execute_process(
            COMMAND ${GIT_EXECUTABLE} describe --tags --long --dirty --always
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_DESCRIBE
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        set(BOT_VERSION_STRING "commit ${GIT_DESCRIBE}")
    endif()
else()
    set(BOT_VERSION_STRING "v${PROJECT_VERSION}")
endif()

set(LUA_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/lib/lua/src")
add_subdirectory(lib/lua)
add_subdirectory(bot)

add_subdirectory(www)
