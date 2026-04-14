# Scope MFC app — builds as a Windows subsystem executable linking MFC as a
# shared DLL. Scoped inside its own CMakeLists so CMAKE_MFC_FLAG doesn't leak
# into the console executables in the parent directory.

if(NOT WIN32)
    message(STATUS "scope/ skipped: MFC is Windows-only")
    return()
endif()

# 2 = Use MFC in a Shared DLL (the VS MFC wizard default).
set(CMAKE_MFC_FLAG 2)

# MFC headers assume Unicode.
add_compile_definitions(_UNICODE UNICODE _AFXDLL)

set(SCOPE_SOURCES
    pch.cpp
    ScopeApp.cpp
    MainFrm.cpp
    ScopeDoc.cpp
    ScopeView.cpp
    ControlPanelView.cpp
    Scope.rc
)

set(SCOPE_HEADERS
    pch.h
    ScopeApp.h
    MainFrm.h
    ScopeDoc.h
    ScopeView.h
    ControlPanelView.h
    Resource.h
)

add_executable(scope WIN32 ${SCOPE_SOURCES} ${SCOPE_HEADERS})

target_include_directories(scope PRIVATE .)

# `ats` PUBLIC-links ATSApi, so scope gets the SDK include path + lib
# transitively. gdiplus.lib comes from #pragma comment in pch.h.
target_link_libraries(scope PRIVATE ats)

# Precompiled header — big MFC speedup.
target_precompile_headers(scope PRIVATE pch.h)

# Copy ATSApi.dll next to the scope.exe the same way the console apps do.
if(DEFINED _ats_dll_src AND _ats_dll_src)
    add_custom_command(TARGET scope POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${_ats_dll_src}" "$<TARGET_FILE_DIR:scope>/ATSApi.dll")
endif()
