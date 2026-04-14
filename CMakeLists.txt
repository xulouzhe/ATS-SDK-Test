cmake_minimum_required(VERSION 3.15)
project(ATS9351Test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -----------------------------------------------------------------------------
# Locate the AlazarTech ATS-SDK.
#
# Layout depends on SDK version:
#   SDK 26.x (newer):   C:/AlazarTech/ATS-SDK/<ver>/Samples_C/Include/AlazarApi.h
#                       C:/AlazarTech/ATS-SDK/<ver>/Samples_C/Library/Win32/ATSApi.lib
#   SDK <= 7.x (older): C:/AlazarTech/ATS-SDK/<ver>/Include/AlazarApi.h
#                       C:/AlazarTech/ATS-SDK/<ver>/Library/Win32/ATSApi.lib
#
# Runtime: ATSApi.dll is installed into C:\Windows\System32 (for x64 exes) and
# C:\Windows\SysWOW64 (for Win32 exes) by the ATS driver installer.
#
# Pass -DATS_SDK_ROOT="C:/AlazarTech/ATS-SDK/26.1.0" to override auto-detection.
# -----------------------------------------------------------------------------

function(_ats_find_inner_dir _root _out_var)
    if(EXISTS "${_root}/Samples_C/Include/AlazarApi.h")
        set(${_out_var} "Samples_C" PARENT_SCOPE)
    elseif(EXISTS "${_root}/Include/AlazarApi.h")
        set(${_out_var} "" PARENT_SCOPE)
    else()
        set(${_out_var} "NOTFOUND" PARENT_SCOPE)
    endif()
endfunction()

if(NOT DEFINED ATS_SDK_ROOT)
    if(WIN32)
        file(GLOB _ats_sdk_candidates "C:/AlazarTech/ATS-SDK/*")
        list(SORT _ats_sdk_candidates ORDER DESCENDING)
        foreach(_cand ${_ats_sdk_candidates})
            if(IS_DIRECTORY "${_cand}")
                _ats_find_inner_dir("${_cand}" _inner)
                if(NOT _inner STREQUAL "NOTFOUND")
                    set(ATS_SDK_ROOT "${_cand}")
                    break()
                endif()
            endif()
        endforeach()
    endif()
endif()

if(NOT ATS_SDK_ROOT)
    message(FATAL_ERROR
        "Could not locate the ATS-SDK under C:/AlazarTech/ATS-SDK/. "
        "Pass -DATS_SDK_ROOT=<path> (the version folder, e.g. .../ATS-SDK/26.1.0).")
endif()

_ats_find_inner_dir("${ATS_SDK_ROOT}" _ats_inner)
if(_ats_inner STREQUAL "NOTFOUND")
    message(FATAL_ERROR
        "ATS_SDK_ROOT='${ATS_SDK_ROOT}' does not contain Include/AlazarApi.h "
        "nor Samples_C/Include/AlazarApi.h. Check the path.")
endif()

if(_ats_inner STREQUAL "")
    set(_ats_include_dir "${ATS_SDK_ROOT}/Include")
    set(_ats_lib_base    "${ATS_SDK_ROOT}/Library")
    set(_ats_layout      "classic")
else()
    set(_ats_include_dir "${ATS_SDK_ROOT}/${_ats_inner}/Include")
    set(_ats_lib_base    "${ATS_SDK_ROOT}/${_ats_inner}/Library")
    set(_ats_layout      "${_ats_inner}")
endif()

# Win32 vs x64 library directory.
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(_ats_lib_dir "${_ats_lib_base}/x64")
    set(_ats_arch_label "x64 (64-bit)")
    set(_ats_system_dll "C:/Windows/System32/ATSApi.dll")
else()
    set(_ats_lib_dir "${_ats_lib_base}/Win32")
    set(_ats_arch_label "Win32 (32-bit)")
    set(_ats_system_dll "C:/Windows/SysWOW64/ATSApi.dll")
endif()

message(STATUS "ATS-SDK root:        ${ATS_SDK_ROOT}")
message(STATUS "ATS-SDK layout:      ${_ats_layout}")
message(STATUS "ATS include folder:  ${_ats_include_dir}")
message(STATUS "Target architecture: ${_ats_arch_label}")
message(STATUS "ATS library folder:  ${_ats_lib_dir}")

find_library(ATS_API_LIB
    NAMES ATSApi
    PATHS "${_ats_lib_dir}"
    NO_DEFAULT_PATH
    REQUIRED)

message(STATUS "ATSApi.lib resolved: ${ATS_API_LIB}")

# Low-level interface target — exposes ATSApi.lib + the SDK include path.
add_library(ATSApi INTERFACE)
target_include_directories(ATSApi INTERFACE "${_ats_include_dir}")
target_link_libraries(ATSApi INTERFACE "${ATS_API_LIB}")

# -----------------------------------------------------------------------------
# libats — reusable acquisition engine used by both the CLI tests and the
# MFC scope. Anyone who links to `ats` transitively gets AlazarApi.h, the
# linker flag for ATSApi.lib, and libats/*.h on their include path.
# -----------------------------------------------------------------------------
add_library(ats STATIC
    libats/AtsTypes.h
    libats/AtsAcquisition.h
    libats/AtsAcquisition.cpp
)
target_include_directories(ats PUBLIC libats)
target_link_libraries(ats PUBLIC ATSApi)

# -----------------------------------------------------------------------------
# CLI test executables (console subsystem).
# -----------------------------------------------------------------------------
add_executable(board_info  src/board_info.cpp)
target_include_directories(board_info PRIVATE include)
target_link_libraries(board_info PRIVATE ats)

add_executable(npt_acquire src/npt_acquire.cpp)
target_link_libraries(npt_acquire PRIVATE ats)

# -----------------------------------------------------------------------------
# ATSApi.dll deploy helper — used by both console exes and the scope app.
# Expose _ats_dll_src to scope/CMakeLists.txt via normal variable scoping.
# -----------------------------------------------------------------------------
set(_ats_dll_src "")
if(WIN32)
    if(EXISTS "${_ats_lib_dir}/ATSApi.dll")
        set(_ats_dll_src "${_ats_lib_dir}/ATSApi.dll")
    elseif(EXISTS "${_ats_system_dll}")
        set(_ats_dll_src "${_ats_system_dll}")
    endif()

    if(_ats_dll_src)
        message(STATUS "ATSApi.dll source:   ${_ats_dll_src} (copied next to each exe)")
        foreach(_tgt board_info npt_acquire)
            add_custom_command(TARGET ${_tgt} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${_ats_dll_src}" "$<TARGET_FILE_DIR:${_tgt}>/ATSApi.dll")
        endforeach()
    else()
        message(STATUS
            "ATSApi.dll not found in SDK or Windows system dirs. "
            "The exes will still resolve the DLL via Windows DLL search path "
            "as long as the ATS driver is installed.")
    endif()
endif()

# -----------------------------------------------------------------------------
# MFC scope GUI app. Lives in its own subdirectory so CMAKE_MFC_FLAG is
# scoped and doesn't leak into the console exes above.
# -----------------------------------------------------------------------------
if(WIN32)
    add_subdirectory(scope)
endif()
