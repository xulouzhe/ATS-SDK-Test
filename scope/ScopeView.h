#pragma once

#include "pch.h"
#include "AtsAcquisition.h"
#include <memory>

// CScopeView owns the scope rendering surface. It runs a 60 Hz timer,
// pulls the latest frame from the document's engine, and redraws with GDI+.
class CScopeView : public CView {
    DECLARE_DYNCREATE(CScopeView)

protected:
    CScopeView();

public:
    void OnDraw(CDC* pDC) override;

    class CScopeDoc* GetDocument() const;

    afx_msg int  OnCreate(LPCREATESTRUCT cs);
    afx_msg void OnDestroy();
    afx_msg void OnTimer(UINT_PTR id);
    afx_msg void OnSize(UINT type, int cx, int cy);
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);

    DECLARE_MESSAGE_MAP()

private:
    void RenderTo(Gdiplus::Graphics& g, int w, int h);

    std::shared_ptr<const ats::Frame> m_latest;
    UINT_PTR m_timerId = 0;
};
