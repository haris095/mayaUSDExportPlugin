include(GNUInstallDirs)

include($ENV{DEVKIT_LOCATION}/cmake/pluginEntry.cmake)

add_definitions(-DNT_PLUGIN)

set(PLUGIN_CMAKE_ROOT ${CMAKE_CURRENT_LIST_DIR})

set(LIBRARIES OpenMaya Foundation)

set(MAYA_USD_LOCATION "D:/workspace/mayaUsdBuilt/install/RelWithDebInfo")

SET(MAYA_USD_PYTHON_LIB_DIR "C:/Program Files/Autodesk/MayaUSD/Maya2022/0.8.0/mayausd/MayaUSD3/lib/python/mayaUsd/lib")

function(_usd_target_properties
    TARGET_NAME
)
    set(options)
    set(oneValueArgs)
    set(multiValueArgs
        INCLUDE_DIRS
        DEFINES
        LIBRARIES
    )

    cmake_parse_arguments(
        args
        "${options}"
        "${oneValueArgs}"
        "${multiValueArgs}"
        ${ARGN}
    )

    # Add additional platform-speific compile definitions
    set (platform_definitions)
    if (MSVC)
        # Depending on which parts of USD the project uses, additional definitions for windows may need
        # to be added. A explicit list of MSVC definitions USD builds with can be found in the USD source at:
        #   cmake/defaults/CXXDefaults.cmake
        #   cmake/defaults/msvcdefaults.cmake
        list(APPEND platform_definitions NOMINMAX)
    endif()

    target_compile_definitions(${TARGET_NAME}
        PRIVATE
            ${args_DEFINES}
            ${platform_definitions}
    )

    target_compile_features(${TARGET_NAME}
        PRIVATE
            cxx_std_17
    )

    # Exported include paths for this target.
    #target_include_directories(${TARGET_NAME}
        #INTERFACE
           # $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    #)

    # Project includes for building against.
    #target_include_directories(${TARGET_NAME}
        #PRIVATE
           # $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_INCLUDEDIR}>
    #)

    list(APPEND DEP_INCLUDE_DIRS 
        $ENV{DEVKIT_LOCATION}/include 
        $ENV{DEVKIT_LOCATION}/include/Python${PY_VER_AB} 
        ${PYTHON_INCLUDE_DIR} 
        ${Boost_INCLUDE_DIR}
        ${MAYA_USD_LOCATION}/include
        ${args_INCLUDE_DIRS}
        ${USD_INCLUDE_DIR}
        ${TBB_INCLUDE_DIRS}
    )

    include_directories(${DEP_INCLUDE_DIRS})

    # Setup include path for binary dir.
    # We set external includes as SYSTEM so that their warnings are muted.
    #set(_INCLUDE_DIRS "")
    #list(APPEND _INCLUDE_DIRS ${DEP_INCLUDE_DIRS} ${args_INCLUDE_DIRS} ${USD_INCLUDE_DIR} ${TBB_INCLUDE_DIRS})
    #if (ENABLE_PYTHON_SUPPORT)
        #list(APPEND _INCLUDE_DIRS ${PYTHON_INCLUDE_DIR} ${Boost_INCLUDE_DIR})
    #endif()
    #target_include_directories(${TARGET_NAME}
       # SYSTEM
        #PRIVATE
           # ${_INCLUDE_DIRS}
    #)

    # Set-up library search path.
    target_link_directories(${TARGET_NAME}
        PRIVATE
            ${USD_LIBRARY_DIR}
            ${DEVKIT_LIBRARY_DIR}
            ${MAYA_USD_LOCATION}/lib
            ${MAYA_USD_PYTHON_LIB_DIR}
    )

    set(LIBRARY_DIRS ${LIBRARY_DIRS} ${DEVKIT_LIBRARY_DIR})
    foreach(MAYA_LIB ${LIBRARIES})
        find_library(${MAYA_LIB}_PATH NAMES ${MAYA_LIB} PATHS ${LIBRARY_DIRS} NO_DEFAULT_PATH)
        set(MAYA_LIBRARIES ${MAYA_LIBRARIES} ${${MAYA_LIB}_PATH})
    endforeach(MAYA_LIB)

    set(MAYAUSD_LIB_DIR ${MAYA_USD_LOCATION}/lib)

    # Mayausd library directory
    find_path(MAYAUSD_LIBRARY_DIR mayaUsd
        PATHS
            ${MAYA_USD_LOCATION}
        PATH_SUFFIXES
            "${MAYA_LIB_SUFFIX}/"
        DOC "MayaUSD library path"
    )
    set(_MAYAUSD_LIBRARIES mayaUsd basePxrUsdPreviewSurface mayaUsd_Translators mayaUsd_Schemas
    )
    foreach(MAYAUSD_LIB ${_MAYAUSD_LIBRARIES})
        find_library(${MAYAUSD_LIB}_PATH NAMES ${MAYAUSD_LIB} PATHS ${MAYAUSD_LIBRARY_DIR})
        if(${MAYAUSD_LIB}_PATH)
            set(MAYAUSD_LIBRARIES ${MAYAUSD_LIBRARIES} ${${MAYAUSD_LIB}_PATH})
        endif()
    endforeach(MAYAUSD_LIB)

    #MayaUSD python library
    find_path(MAYAUSD_PYTHON_LIB_DIR _mayaUsd
        PATHS
            ${MAYA_USD_PYTHON_LIB_DIR}
        PATH_SUFFIXES
            "${MAYA_LIB_SUFFIX}/"
        DOC "MayaUSD library path"
    )

    set(_MAYAUSDPYTHON_LIB _mayaUsd)

    find_library(${_MAYAUSDPYTHON_LIB}_PATH NAMES ${_MAYAUSDPYTHON_LIB} PATHS ${MAYAUSD_PYTHON_LIB_DIR})
    if(${_MAYAUSDPYTHON_LIB}_PATH)
        set(MAYAUSDPYTHON_LIB ${MAYAUSDPYTHON_LIB} ${_MAYAUSDPYTHON_LIB}_PATH) 
    endif()

    # Link to libraries.
    set(_LINK_LIBRARIES "")
    list(APPEND _LINK_LIBRARIES ${args_LIBRARIES} ${TBB_LIBRARIES} ${MAYA_LIBRARIES} ${MAYAUSD_LIBRARIES})
    if (ENABLE_PYTHON_SUPPORT)
        list(APPEND _LINK_LIBRARIES ${Boost_PYTHON_LIBRARY} ${PYTHON_LIBRARIES})
    endif()
    target_link_libraries(${TARGET_NAME} ${_LINK_LIBRARIES} ${MAYAUSDPYTHON_LIB})
    set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".mll")


    install(DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/bin)
endfunction() # _usd_target_properties



# Adds a USD-based C++ executable application.
function(build_plugin PROJECT_NAME)

    set(options)

    set(PY_VER_AB 37)

    set(oneValueArgs
    )

    set(multiValueArgs
        CPPFILES
        LIBRARIES
        INCLUDE_DIRS
    )

    cmake_parse_arguments(args
        "${options}"
        "${oneValueArgs}"
        "${multiValueArgs}"
        ${ARGN}
    )

    # Define a new executable.
    add_library(${PROJECT_NAME} SHARED ${args_CPPFILES})
    #target_sources(${PROJECT_NAME} PRIVATE ${args_CPPFILES})

    #add_executable(${PROJECT_NAME} ${args_CPPFILES})

    # Apply properties.
    _usd_target_properties(${PROJECT_NAME}
        INCLUDE_DIRS
            ${args_INCLUDE_DIRS}
        LIBRARIES
            ${args_LIBRARIES}
    )

    # Install built executable.
    install(
        TARGETS ${EXECUTABLE_NAME}
        DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

endfunction() # usd_executable