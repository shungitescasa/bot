cmake_minimum_required(VERSION 3.10)
include(FetchContent)

file(GLOB_RECURSE SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_library(Core)

target_sources(Core PUBLIC "${SOURCE_FILES}")

target_include_directories(Core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(Core PRIVATE DEBUG_MODE)
endif()

target_compile_definitions(Core PRIVATE
  BOT_VERSION="${BOT_VERSION_STRING}"
  BOT_COMPILED_TIMESTAMP=${PROJECT_COMPILE_TIME}
  DEFAULT_PREFIX="${BOT_DEFAULT_PREFIX}"
  DEFAULT_LOCALE_ID="${BOT_DEFAULT_LOCALE_ID}"
)

# ---------
# Databases
# ---------

option(USE_POSTGRES "Use PostgreSQL backend" ON)
option(USE_MARIADB "Use MariaDB backend" OFF)

if (USE_POSTGRES)
  FetchContent_Declare(
    pqxx
    GIT_REPOSITORY https://github.com/jtv/libpqxx.git
    GIT_TAG 7.10.1
  )
  FetchContent_MakeAvailable(pqxx)
  target_compile_definitions(Core PRIVATE USE_POSTGRES)
  target_link_libraries(Core PRIVATE pqxx)
elseif (USE_MARIADB)
  target_compile_definitions(Core PRIVATE USE_MARIADB)

  # searching for mysql
  find_program(MYSQL_CONFIG_EXECUTABLE mysql_config REQUIRED)

  execute_process(COMMAND ${MYSQL_CONFIG_EXECUTABLE} --cflags
                  OUTPUT_VARIABLE MYSQL_CFLAGS
                  OUTPUT_STRIP_TRAILING_WHITESPACE)

  execute_process(COMMAND ${MYSQL_CONFIG_EXECUTABLE} --libs
                  OUTPUT_VARIABLE MYSQL_LIBS
                  OUTPUT_STRIP_TRAILING_WHITESPACE)

  target_compile_options(Core PRIVATE ${MYSQL_CFLAGS})
  target_link_libraries(Core PRIVATE ${MYSQL_LIBS})
endif()

# -- Getting libraries

# boost
set(Boost_NO_BOOST_CMAKE ON)

find_package(Boost REQUIRED)
find_package(OpenSSL REQUIRED)

# rpc
FetchContent_Declare(
  rpc
  GIT_REPOSITORY https://github.com/rpclib/rpclib.git
  GIT_TAG v2.3.0
)
FetchContent_MakeAvailable(rpc)

# lua
target_include_directories(Core BEFORE PUBLIC ${LUA_INCLUDE_DIR})
target_link_libraries(Core PUBLIC lua)

set(SOL2_LUA_VERSION "5.4.0" CACHE STRING "The version of Lua used")
set(SOL2_BUILD_LUA FALSE CACHE BOOL "Always build Lua, do not search for it in the system")

FetchContent_Declare(
  sol
  GIT_REPOSITORY https://github.com/ThePhD/sol2.git
  GIT_TAG v3.5.0
)
FetchContent_MakeAvailable(sol)

target_link_libraries(Core PUBLIC
    Boost::headers
    OpenSSL::SSL
    OpenSSL::Crypto
    rpc
    lua
    sol2::sol2
)