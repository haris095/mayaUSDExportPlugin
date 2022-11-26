cmake_minimum_required(VERSION 3.13)

project(usdplugin)

set(project ${PROJECT_NAME})

option(ENABLE_PYTHON_SUPPORT "Enable support for python." ON)
option(USE_PYTHON_3 "Build python bindings for Python 3" ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/packages
)

#add_executable(${PROJECT_NAME})

include(Dependencies)

include(Packages)

include(FindMaya)

include(TargetBuild)

build_plugin(${PROJECT_NAME}
    CPPFILES
        list.cpp

    LIBRARIES
        tf
        gf
        sdf
        usd
        usdGeom
        vt
        usdUtils
)

