cmake_minimum_required(VERSION 3.15)
project(ATS9351Test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -----------------------------------------------------------------------------
# Locate the AlazarTech ATS-SDK.
#
# The installer lays the SDK out under:
#   C:/AlazarTech/ATS-SDK/<version>/Include/AlazarApi.h
#   C:/AlazarTech/ATS-SDK/<version>/Library/Win32/ATSApi.lib    (32-bit)
#   C:/AlazarTech/ATS-SDK/<version>/Library/x64/ATSApi.lib      (64-bit)
#
# Pass -DATS_SDK_ROOT="C:/AlazarTech/ATS-SDK/7.4.0" on the cmake command line
# if the auto-glob below does not find your version.
# -----------------------------------------------------------------------------

if(NOT DEFINED ATS_SDK_ROOT)
    if(WIN32)
        file(GLOB _ats_sdk_candidates "C:/AlazarTech/ATS-SDK/*")
        list(SORT _ats_sdk_candidates ORDER DESCENDING)
        foreach(_cand ${_ats_sdk_candidates})
            if(IS_DIRECTORY "${_cand}" AND EXISTS "${_cand}/Include/AlazarApi.h")
                set(ATS_SDK_ROOT "${_cand}")
                break()
            endif()
        endforeach()
    endif()
endif()

if(NOT ATS_SDK_ROOT OR NOT EXISTS "${ATS_SDK_ROOT}/Include/AlazarApi.h")
    message(FATAL_ERROR
        "Could not locate the ATS-SDK. "
        "Pass -DATS_SDK_ROOT=<path> (the folder that contains Include/ and Library/).")
endif()

message(STATUS "ATS-SDK root: ${ATS_SDK_ROOT}")

# Pick Win32 vs x64 library directory.
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(_ats_lib_dir "${ATS_SDK_ROOT}/Library/x64")
    set(_ats_arch_label "x64 (64-bit)")
else()
    set(_ats_lib_dir "${ATS_SDK_ROOT}/Library/Win32")
    set(_ats_arch_label "Win32 (32-bit)")
endif()

message(STATUS "Target architecture: ${_ats_arch_label}")
message(STATUS "ATS library folder:  ${_ats_lib_dir}")

find_library(ATS_API_LIB
    NAMES ATSApi
    PATHS "${_ats_lib_dir}"
    NO_DEFAULT_PATH
    REQUIRED)

message(STATUS "ATSApi.lib resolved: ${ATS_API_LIB}")

add_library(ATSApi INTERFACE)
target_include_directories(ATSApi INTERFACE "${ATS_SDK_ROOT}/Include")
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

# Copy ATSApi.dll next to the exes on Windows so they run from the build dir.
if(WIN32)
    set(_ats_dll "${_ats_lib_dir}/ATSApi.dll")
    if(EXISTS "${_ats_dll}")
        foreach(_tgt board_info npt_acquire)
            add_custom_command(TARGET ${_tgt} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${_ats_dll}" "$<TARGET_FILE_DIR:${_tgt}>/ATSApi.dll")
        endforeach()
    endif()
endif()
