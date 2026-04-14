#include "pch.h"
#include "ScopeApp.h"
#include "MainFrm.h"
#include "ScopeDoc.h"
#include "ScopeView.h"
#include "Resource.h"

CScopeApp theApp;

BOOL CScopeApp::InitInstance() {
    CWinApp::InitInstance();

    // Start up GDI+.
    Gdiplus::GdiplusStartupInput gsi;
    Gdiplus::GdiplusStartup(&m_gdiplusToken, &gsi, nullptr);

    // Single-document template (Doc + CMainFrame + CScopeView).
    auto* pDocTemplate = new CSingleDocTemplate(
        IDR_MAINFRAME,
        RUNTIME_CLASS(CScopeDoc),
        RUNTIME_CLASS(CMainFrame),
        RUNTIME_CLASS(CScopeView));
    AddDocTemplate(pDocTemplate);

    // Start with an empty doc rather than parsing the command line — we
    // don't handle file open.
    CCommandLineInfo cmd;
    cmd.m_nShellCommand = CCommandLineInfo::FileNew;
    if (!ProcessShellCommand(cmd)) return FALSE;

    m_pMainWnd->ShowWindow(SW_SHOW);
    m_pMainWnd->UpdateWindow();
    return TRUE;
}

int CScopeApp::ExitInstance() {
    if (m_gdiplusToken) {
        Gdiplus::GdiplusShutdown(m_gdiplusToken);
    }
    return CWinApp::ExitInstance();
}
