cmake_minimum_required(VERSION 3.12)
project(Tether)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules ${CMAKE_MODULE_PATH})
include(SuperColliderServerPlugin RESULT_VARIABLE server_plugin_found)
include(SuperColliderCompilerConfig RESULT_VARIABLE compiler_config_found)
if(NOT server_plugin_found OR NOT compiler_config_found)
    message(FATAL_ERROR "cmake_modules not found; vendor them from <SC_PATH>/tools/cmake_gen/")
endif()

option(SCSYNTH "Build scsynth plugins" ON)
option(SUPERNOVA "Build supernova plugins" OFF)

sc_check_sc_path("${SC_PATH}")
message(STATUS "Found SuperCollider: ${SC_PATH}")
include("${SC_PATH}/SCVersion.txt")
message(STATUS "Building for SuperCollider ${SC_VERSION}")

sc_do_initial_compiler_config()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# Default install into the user Extensions folder unless overridden.
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    if(APPLE)
        set(_ext "$ENV{HOME}/Library/Application Support/SuperCollider/Extensions")
    elseif(WIN32)
        set(_ext "$ENV{LOCALAPPDATA}/SuperCollider/Extensions")
    else()
        set(_ext "$ENV{HOME}/.local/share/SuperCollider/Extensions")
    endif()
    set(CMAKE_INSTALL_PREFIX "${_ext}" CACHE PATH "" FORCE)
endif()
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")

set(TETHER_UGENS Gendreve Diststoch)
foreach(ugen ${TETHER_UGENS})
    sc_add_server_plugin(
        "Tether"                              # dest dir under Extensions
        "${ugen}"                             # registration name
        "plugins/${ugen}.cpp"
        "classes/${ugen}.sc"
        "HelpSource/Classes/${ugen}.schelp")
endforeach()
