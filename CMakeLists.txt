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
# C:\Windows\SysWOW64 (for Win32 exes) by the ATS driver installer, so we do
# not have to ship it — the exe picks it up via the standard DLL search path.
#
# Pass -DATS_SDK_ROOT="C:/AlazarTech/ATS-SDK/26.1.0" to override auto-detection.
# -----------------------------------------------------------------------------

# Helper: return the subfolder (relative to SDK root) that contains Include/Library,
# i.e. "" for the old layout or "Samples_C" for SDK 26.x.
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

# Pick Win32 vs x64 library directory.
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(_ats_lib_dir "${_ats_lib_base}/x64")
    set(_ats_arch_label "x64 (64-bit)")
    set(_ats_system_dll "C:/Windows/System32/ATSApi.dll")
else()
    set(_ats_lib_dir "${_ats_lib_base}/Win32")
    set(_ats_arch_label "Win32 (32-bit)")
    # On 64-bit Windows the 32-bit DLL lives in SysWOW64; on 32-bit Windows in System32.
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

add_library(ATSApi INTERFACE)
target_include_directories(ATSApi INTERFACE "${_ats_include_dir}")
target_link_libraries(ATSApi INTERFACE "${ATS_API_LIB}")

# -----------------------------------------------------------------------------
# Executables.
# -----------------------------------------------------------------------------
add_executable(board_info  src/board_info.cpp)
target_include_directories(board_info PRIVATE include)
target_link_libraries(board_info PRIVATE ATSApi)

add_executable(npt_acquire src/npt_acquire.cpp)
target_include_directories(npt_acquire PRIVATE include)
target_link_libraries(npt_acquire PRIVATE ATSApi)

# -----------------------------------------------------------------------------
# Runtime DLL.
#
# Order of preference for the ATSApi.dll source:
#   1. A DLL sitting next to ATSApi.lib in the SDK (older layout sometimes has one).
#   2. The system-installed DLL (C:\Windows\System32 or SysWOW64) that the driver
#      installer puts there. This is the common case for SDK 26.x.
#   3. If neither exists, do nothing — the exe will still find the DLL at runtime
#      via the normal Windows DLL search path as long as the driver is installed.
# -----------------------------------------------------------------------------
if(WIN32)
    set(_ats_dll_src "")
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
            "The exes will still work as long as the ATS driver is installed "
            "and the DLL is on the Windows DLL search path.")
    endif()
endif()
