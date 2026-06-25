cmake_minimum_required(VERSION 3.10)
include(FetchContent)

file(GLOB_RECURSE SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_library(Core)

target_sources(Core PUBLIC "${SOURCE_FILES}")

target_include_directories(Core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# -- Getting libraries

# boost
set(Boost_NO_BOOST_CMAKE ON)

find_package(Boost REQUIRED)
find_package(OpenSSL REQUIRED)

target_link_libraries(Core PUBLIC
    Boost::headers
    OpenSSL::SSL
    OpenSSL::Crypto
)