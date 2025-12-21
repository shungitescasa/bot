cmake_minimum_required(VERSION 3.10)

project(www NONE)

file(GLOB LUA_SCRIPTS "${CMAKE_SOURCE_DIR}/luamods/*.lua")

find_package(Python3 REQUIRED COMPONENTS Interpreter)

# initializing environment
set(VENV_DIR ${CMAKE_BINARY_DIR}/cmakescripts/.venv)
set(VENV_PY ${VENV_DIR}/bin/python)
set(VENV_PIP ${VENV_DIR}/bin/pip)
add_custom_command(
  OUTPUT ${VENV_PY}
  COMMAND ${Python3_EXECUTABLE} -m venv ${VENV_DIR}
  COMMAND ${VENV_PIP} install --upgrade pip
  COMMAND ${VENV_PIP} install lupa jinja2 markdown
  COMMENT "Creating Python venv and installing dependencies"
)

add_custom_target(
  python_venv
  DEPENDS ${VENV_PY}
)

add_custom_target(
    generate_lua_pages
    COMMAND ${VENV_PY} ${CMAKE_SOURCE_DIR}/cmakescripts/generate_lua_pages.py
    DEPENDS python_venv ${LUA_SCRIPTS} ${CMAKE_SOURCE_DIR}/www/partials/luascript.shtml
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)