#include "pch.h"
#include "MainFrm.h"
#include "ScopeView.h"
#include "ControlPanelView.h"

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs) {
    if (!CFrameWnd::PreCreateWindow(cs)) return FALSE;
    cs.lpszName = L"ATS9351 Scope";
    cs.cx = 1200;
    cs.cy = 720;
    cs.style |= WS_CLIPCHILDREN;
    return TRUE;
}

BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT, CCreateContext* pContext) {
    // Two-pane horizontal splitter: control panel on the left (fixed-ish
    // width), scope plot on the right (takes the rest).
    if (!m_splitter.CreateStatic(this, 1, 2)) return FALSE;

    const int kPanelWidth = 230;   // pixels
    if (!m_splitter.CreateView(0, 0, RUNTIME_CLASS(CControlPanelView),
                               CSize(kPanelWidth, 400), pContext)) {
        return FALSE;
    }
    if (!m_splitter.CreateView(0, 1, RUNTIME_CLASS(CScopeView),
                               CSize(0, 0), pContext)) {
        return FALSE;
    }

    // Force the active view to be the scope, not the control panel, so
    // the main frame's title follows the scope and keyboard shortcuts
    // route sanely.
    SetActiveView(
        static_cast<CView*>(m_splitter.GetPane(0, 1)));
    return TRUE;
}
