#pragma once

#include "pch.h"

class CMainFrame : public CFrameWnd {
    DECLARE_DYNCREATE(CMainFrame)

public:
    CMainFrame() = default;

    BOOL PreCreateWindow(CREATESTRUCT& cs) override;
    BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) override;

private:
    CSplitterWnd m_splitter;
};
