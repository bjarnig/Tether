cmake_minimum_required(VERSION 3.12)
project(Tether LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# SuperCollider source tree (for plugin headers).
# Override with -DSC_PATH=/path/to/supercollider or the SC_PATH env var.
if(NOT SC_PATH)
    if(DEFINED ENV{SC_PATH})
        set(SC_PATH $ENV{SC_PATH})
    else()
        set(SC_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../supercollider")
    endif()
endif()
get_filename_component(SC_PATH "${SC_PATH}" ABSOLUTE)
if(NOT EXISTS "${SC_PATH}/include/plugin_interface/SC_PlugIn.hpp")
    message(FATAL_ERROR "SC_PlugIn.hpp not found under SC_PATH='${SC_PATH}'. "
        "Set -DSC_PATH=/path/to/supercollider source tree.")
endif()
message(STATUS "Tether using SC_PATH: ${SC_PATH}")

# api_version must match your installed scsynth or the .scx is rejected.
file(STRINGS "${SC_PATH}/include/plugin_interface/SC_InterfaceTable.h" _api_line
    REGEX "sc_api_version = ")
message(STATUS "Tether: SC source ${_api_line} — must match your scsynth build")

include_directories(
    "${SC_PATH}/include/plugin_interface"
    "${SC_PATH}/include/common"
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

set(CMAKE_SHARED_MODULE_PREFIX "")
if(APPLE OR WIN32)
    set(CMAKE_SHARED_MODULE_SUFFIX ".scx")
endif()

function(tether_plugin name source)
    add_library(${name} MODULE ${source})
    if(NOT (APPLE OR WIN32))
        target_compile_options(${name} PRIVATE -fPIC)
    endif()
endfunction()

tether_plugin(Gendreve plugins/Gendreve/Gendreve.cpp)
tether_plugin(Diststoch plugins/Diststoch/Diststoch.cpp)
