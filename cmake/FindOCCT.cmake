include(FindPackageHandleStandardArgs)

set(_occt_roots)

foreach(_var IN ITEMS OCCT_ROOT OpenCASCADE_DIR CASROOT)
    if(DEFINED ENV{${_var}} AND NOT "$ENV{${_var}}" STREQUAL "")
        list(APPEND _occt_roots "$ENV{${_var}}")
    endif()
endforeach()

function(pyraqt_occt_rewrite_transitive_libs target_name)
    if(NOT TARGET ${target_name})
        return()
    endif()

    get_target_property(_target_links ${target_name} INTERFACE_LINK_LIBRARIES)
    if(NOT _target_links)
        return()
    endif()

    set(_rewritten_links)
    foreach(_link IN LISTS _target_links)
        if(_link STREQUAL "/usr/lib/x86_64-linux-gnu/libtbb.so")
            find_library(_pyraqt_tbb_runtime
                NAMES tbb libtbb.so.12 libtbb.so
                PATHS /usr/lib/x86_64-linux-gnu /usr/lib64 /usr/local/lib /usr/local/lib64
                NO_DEFAULT_PATH
            )
            if(_pyraqt_tbb_runtime)
                list(APPEND _rewritten_links "${_pyraqt_tbb_runtime}")
            else()
                list(APPEND _rewritten_links "${_link}")
            endif()
        elseif(_link STREQUAL "/usr/lib/x86_64-linux-gnu/libtbbmalloc.so")
            find_library(_pyraqt_tbbmalloc_runtime
                NAMES tbbmalloc libtbbmalloc.so.2 libtbbmalloc.so
                PATHS /usr/lib/x86_64-linux-gnu /usr/lib64 /usr/local/lib /usr/local/lib64
                NO_DEFAULT_PATH
            )
            if(_pyraqt_tbbmalloc_runtime)
                list(APPEND _rewritten_links "${_pyraqt_tbbmalloc_runtime}")
            else()
                list(APPEND _rewritten_links "${_link}")
            endif()
        else()
            list(APPEND _rewritten_links "${_link}")
        endif()
    endforeach()

    set_target_properties(${target_name} PROPERTIES
        INTERFACE_LINK_LIBRARIES "${_rewritten_links}"
    )
endfunction()

set(_occt_candidate_config_dirs
    ${_occt_roots}
    /usr/lib/x86_64-linux-gnu/cmake/opencascade
    /usr/lib64/cmake/opencascade
    /usr/local/lib/cmake/opencascade
    /usr/local/lib64/cmake/opencascade
    /opt/occt/lib/cmake/opencascade
)

if(NOT OpenCASCADE_FOUND)
    find_package(OpenCASCADE CONFIG QUIET
        COMPONENTS FoundationClasses ModelingData ModelingAlgorithms DataExchange Visualization
        PATHS ${_occt_candidate_config_dirs}
        NO_DEFAULT_PATH
    )
endif()

if(NOT OpenCASCADE_FOUND)
    find_package(OpenCASCADE CONFIG QUIET COMPONENTS FoundationClasses ModelingData ModelingAlgorithms DataExchange Visualization)
endif()

set(_occt_required_targets
    TKernel
    TKMath
    TKG2d
    TKG3d
    TKGeomBase
    TKBRep
    TKTopAlgo
    TKPrim
    TKBO
    TKSTEP
    TKSTEP209
    TKSTEPAttr
    TKSTEPBase
    TKXSBase
    TKShHealing
    TKService
    TKV3d
    TKOpenGl
)

if(OpenCASCADE_FOUND)
    if(DEFINED OpenCASCADE_INCLUDE_DIR AND NOT "${OpenCASCADE_INCLUDE_DIR}" STREQUAL "")
        set(OCCT_INCLUDE_DIR "${OpenCASCADE_INCLUDE_DIR}")
    endif()

    set(_occt_target_list)
    foreach(_target IN ITEMS TKernel TKMath TKG2d TKG3d TKGeomBase TKBRep TKTopAlgo TKPrim TKBO TKShHealing TKService TKV3d TKOpenGl)
        if(TARGET ${_target})
            list(APPEND _occt_target_list ${_target})
        endif()
    endforeach()

    foreach(_target IN LISTS _occt_target_list)
        pyraqt_occt_rewrite_transitive_libs(${_target})
    endforeach()

    if(NOT OCCT_INCLUDE_DIR)
        get_target_property(_occt_include_dir TKernel INTERFACE_INCLUDE_DIRECTORIES)
        if(_occt_include_dir)
            list(GET _occt_include_dir 0 OCCT_INCLUDE_DIR)
        endif()
    endif()
endif()

if(NOT OCCT_INCLUDE_DIR)
    find_path(OCCT_INCLUDE_DIR
        NAMES STEPControl_Reader.hxx BRepTools.hxx
        HINTS ${_occt_roots}
        PATH_SUFFIXES
            include
            include/opencascade
            opencascade
            include/oce
            oce
            include/x86_64-linux-gnu/opencascade
    )
endif()

find_path(OCCT_DATA_EXCHANGE_INCLUDE_DIR
    NAMES STEPControl_Reader.hxx
    HINTS ${_occt_roots} /usr/include /usr/local/include
    PATH_SUFFIXES
        include
        include/opencascade
        opencascade
        include/oce
        oce
        include/x86_64-linux-gnu/opencascade
)

set(OCCT_LIBRARIES)
foreach(_target IN LISTS _occt_target_list)
    list(APPEND OCCT_LIBRARIES "${_target}")
endforeach()

set(_occt_missing_libraries)
foreach(_lib IN ITEMS TKSTEP TKSTEP209 TKSTEPAttr TKSTEPBase TKXSBase TKService TKV3d TKOpenGl)
    if(TARGET ${_lib})
        list(APPEND OCCT_LIBRARIES "${_lib}")
    else()
        find_library(OCCT_${_lib}_LIBRARY
            NAMES ${_lib} lib${_lib}.so.7 lib${_lib}.so
            HINTS ${_occt_roots}
            PATH_SUFFIXES lib lib64 lib/x86_64-linux-gnu x86_64-linux-gnu
        )
        if(OCCT_${_lib}_LIBRARY)
            list(APPEND OCCT_LIBRARIES "${OCCT_${_lib}_LIBRARY}")
        else()
            list(APPEND _occt_missing_libraries ${_lib})
        endif()
    endif()
endforeach()

list(REMOVE_DUPLICATES OCCT_LIBRARIES)

find_path(OCCT_VISUALIZATION_INCLUDE_DIR
    NAMES AIS_InteractiveContext.hxx V3d_View.hxx OpenGl_GraphicDriver.hxx Aspect_NeutralWindow.hxx
    HINTS ${_occt_roots} /usr/include /usr/local/include
    PATH_SUFFIXES
        include
        include/opencascade
        opencascade
        include/oce
        oce
        include/x86_64-linux-gnu/opencascade
)

if(OCCT_INCLUDE_DIR AND OCCT_DATA_EXCHANGE_INCLUDE_DIR AND OCCT_VISUALIZATION_INCLUDE_DIR AND OCCT_LIBRARIES)
    foreach(_required_target IN LISTS _occt_required_targets)
        list(FIND OCCT_LIBRARIES "${_required_target}" _required_index)
        if(_required_index EQUAL -1)
            list(APPEND _occt_missing_libraries "${_required_target}")
        endif()
    endforeach()
    list(REMOVE_DUPLICATES _occt_missing_libraries)
    if(NOT _occt_missing_libraries)
        set(OCCT_FOUND TRUE)
        if(NOT TARGET OCCT::OCCT)
            add_library(OCCT::OCCT INTERFACE IMPORTED)
            set_target_properties(OCCT::OCCT PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${OCCT_INCLUDE_DIR}"
                INTERFACE_LINK_LIBRARIES "${OCCT_LIBRARIES}"
            )
        endif()
    else()
        set(OCCT_FOUND FALSE)
    endif()
endif()

find_package_handle_standard_args(OCCT
    REQUIRED_VARS OCCT_INCLUDE_DIR OCCT_DATA_EXCHANGE_INCLUDE_DIR OCCT_VISUALIZATION_INCLUDE_DIR OCCT_LIBRARIES
)
