cmake_minimum_required(VERSION 3.10)
include(FetchContent)

# Creating symbolic links
create_symlink_if_exists("${CMAKE_SOURCE_DIR}/static" "${CMAKE_CURRENT_BINARY_DIR}/static")
create_symlink_if_exists("${CMAKE_SOURCE_DIR}/templates" "${CMAKE_CURRENT_BINARY_DIR}/templates")
create_symlink_if_exists("${CMAKE_SOURCE_DIR}/docs" "${CMAKE_CURRENT_BINARY_DIR}/docs")
if (EXISTS "${CMAKE_SOURCE_DIR}/.env")
    create_symlink_if_exists("${CMAKE_SOURCE_DIR}/.env" "${CMAKE_CURRENT_BINARY_DIR}/.env")
endif()

add_executable(Web)

set_target_properties(
  Web PROPERTIES
  DESCRIPTION ${PROJECT_DESCRIPTION}
  OUTPUT_NAME "${PROJECT_NAME}-web"
)

file(GLOB_RECURSE SOURCE_FILES "src/*.cpp" "src/*.h" "src/*.hpp")
target_sources(Web PRIVATE ${SOURCE_FILES})

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