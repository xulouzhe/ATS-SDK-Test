#include "pch.h"
#include "ScopeView.h"
#include "ScopeDoc.h"

IMPLEMENT_DYNCREATE(CScopeView, CView)

BEGIN_MESSAGE_MAP(CScopeView, CView)
    ON_WM_CREATE()
    ON_WM_DESTROY()
    ON_WM_TIMER()
    ON_WM_SIZE()
    ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

CScopeView::CScopeView() = default;

CScopeDoc* CScopeView::GetDocument() const {
    return reinterpret_cast<CScopeDoc*>(m_pDocument);
}

int CScopeView::OnCreate(LPCREATESTRUCT cs) {
    if (CView::OnCreate(cs) == -1) return -1;
    m_timerId = SetTimer(1, 33, nullptr);   // ~30 Hz redraw
    return 0;
}

void CScopeView::OnDestroy() {
    if (m_timerId) KillTimer(m_timerId);
    CView::OnDestroy();
}

void CScopeView::OnSize(UINT type, int cx, int cy) {
    CView::OnSize(type, cx, cy);
    Invalidate(FALSE);
}

BOOL CScopeView::OnEraseBkgnd(CDC*) {
    // We fill the whole client area ourselves in OnDraw, so erase is a no-op
    // to kill flicker.
    return TRUE;
}

void CScopeView::OnTimer(UINT_PTR) {
    CScopeDoc* doc = GetDocument();
    if (!doc) return;
    if (doc->IsRunning()) {
        auto frame = doc->engine().latestFrame();
        if (frame) m_latest = frame;
    }
    Invalidate(FALSE);
}

// Double-buffered GDI+ rendering ---------------------------------------------
void CScopeView::OnDraw(CDC* pDC) {
    CRect rc; GetClientRect(&rc);
    const int w = rc.Width();
    const int h = rc.Height();
    if (w <= 0 || h <= 0) return;

    // Off-screen bitmap to avoid flicker.
    Gdiplus::Bitmap bmp(w, h, PixelFormat32bppPARGB);
    Gdiplus::Graphics g(&bmp);
    g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    g.SetCompositingQuality(Gdiplus::CompositingQualityHighQuality);
    RenderTo(g, w, h);

    Gdiplus::Graphics screen(pDC->GetSafeHdc());
    screen.DrawImage(&bmp, 0, 0, w, h);
}

// Actual scope rendering. 10 horizontal divs × 8 vertical divs (standard).
void CScopeView::RenderTo(Gdiplus::Graphics& g, int w, int h) {
    using namespace Gdiplus;

    // Backdrop
    SolidBrush bg(Color(255, 12, 12, 20));
    g.FillRectangle(&bg, 0, 0, w, h);

    // Plot margins
    constexpr int kMarginL = 60;
    constexpr int kMarginR = 16;
    constexpr int kMarginT = 12;
    constexpr int kMarginB = 30;
    const float plotX = static_cast<float>(kMarginL);
    const float plotY = static_cast<float>(kMarginT);
    const float plotW = static_cast<float>(w - kMarginL - kMarginR);
    const float plotH = static_cast<float>(h - kMarginT - kMarginB);
    if (plotW < 50 || plotH < 50) return;

    constexpr int kDivX = 10;
    constexpr int kDivY = 8;

    // Grid --------------------------------------------------------------
    Pen gridMajor(Color(255, 48, 68, 68), 1.0f);
    Pen gridMinor(Color(255, 28, 40, 40), 1.0f);

    // Minor grid (5 per div)
    for (int i = 0; i <= kDivX * 5; ++i) {
        float x = plotX + plotW * (i / float(kDivX * 5));
        g.DrawLine(&gridMinor, x, plotY, x, plotY + plotH);
    }
    for (int i = 0; i <= kDivY * 5; ++i) {
        float y = plotY + plotH * (i / float(kDivY * 5));
        g.DrawLine(&gridMinor, plotX, y, plotX + plotW, y);
    }
    // Major grid (per division)
    for (int i = 0; i <= kDivX; ++i) {
        float x = plotX + plotW * (i / float(kDivX));
        g.DrawLine(&gridMajor, x, plotY, x, plotY + plotH);
    }
    for (int i = 0; i <= kDivY; ++i) {
        float y = plotY + plotH * (i / float(kDivY));
        g.DrawLine(&gridMajor, plotX, y, plotX + plotW, y);
    }
    // Center crosshair
    Pen axisPen(Color(255, 90, 140, 140), 1.0f);
    g.DrawLine(&axisPen, plotX, plotY + plotH / 2, plotX + plotW, plotY + plotH / 2);
    g.DrawLine(&axisPen, plotX + plotW / 2, plotY, plotX + plotW / 2, plotY + plotH);

    // Text setup --------------------------------------------------------
    FontFamily ff(L"Segoe UI");
    Font       font(&ff, 10.0f, FontStyleRegular, UnitPixel);
    SolidBrush textBrush(Color(255, 200, 220, 220));
    StringFormat sf;

    CScopeDoc* doc = GetDocument();

    // Frame info (time/div, volts/div in big text across the top)
    if (m_latest && m_latest->sampleRateHz > 0 && m_latest->samplesPerRecord > 0) {
        const double totalSec = m_latest->samplesPerRecord / m_latest->sampleRateHz;
        const double perDivSec = totalSec / kDivX;
        WCHAR tdiv[64];
        if      (perDivSec >= 1e-3) swprintf_s(tdiv, L"%.3f ms/div", perDivSec * 1e3);
        else if (perDivSec >= 1e-6) swprintf_s(tdiv, L"%.3f us/div", perDivSec * 1e6);
        else                        swprintf_s(tdiv, L"%.3f ns/div", perDivSec * 1e9);
        PointF pt(static_cast<float>(kMarginL), 0.0f);
        g.DrawString(tdiv, -1, &font, pt, &sf, &textBrush);

        WCHAR vdivA[64] = L"";
        if (doc->displayA().enabled) {
            double fsA = m_latest->fullScaleVoltsA / doc->displayA().zoom;
            double perDivV = (2.0 * fsA) / kDivY;
            if      (perDivV >= 1.0)  swprintf_s(vdivA, L"A: %.3f V/div",  perDivV);
            else if (perDivV >= 1e-3) swprintf_s(vdivA, L"A: %.1f mV/div", perDivV * 1e3);
            else                      swprintf_s(vdivA, L"A: %.1f uV/div", perDivV * 1e6);
            PointF pa(static_cast<float>(kMarginL) + 140.0f, 0.0f);
            SolidBrush yellow(Color(255, 220, 200, 60));
            g.DrawString(vdivA, -1, &font, pa, &sf, &yellow);
        }

        WCHAR vdivB[64] = L"";
        if (doc->displayB().enabled) {
            double fsB = m_latest->fullScaleVoltsB / doc->displayB().zoom;
            double perDivV = (2.0 * fsB) / kDivY;
            if      (perDivV >= 1.0)  swprintf_s(vdivB, L"B: %.3f V/div",  perDivV);
            else if (perDivV >= 1e-3) swprintf_s(vdivB, L"B: %.1f mV/div", perDivV * 1e3);
            else                      swprintf_s(vdivB, L"B: %.1f uV/div", perDivV * 1e6);
            PointF pb(static_cast<float>(kMarginL) + 280.0f, 0.0f);
            SolidBrush cyan(Color(255, 60, 200, 220));
            g.DrawString(vdivB, -1, &font, pb, &sf, &cyan);
        }
    }

    // "No data" message when idle
    if (!m_latest) {
        PointF centre(plotX + plotW / 2 - 60.0f, plotY + plotH / 2 - 6.0f);
        g.DrawString(L"Press Run to acquire", -1, &font, centre, &sf, &textBrush);
        return;
    }

    // Helper lambda: plot one channel's first record as a connected polyline.
    // Uses simple downsampling (stride) if samples exceed pixel width.
    auto plotChannel = [&](const std::vector<uint16_t>& data,
                           unsigned samplesPerRecord,
                           double fullScaleVolts,
                           const CScopeDoc::Display& disp,
                           Color color) {
        if (!disp.enabled || data.empty() || samplesPerRecord == 0) return;
        Pen pen(color, 1.2f);

        // Compute effective V/full-scale after software zoom
        const double effFS = fullScaleVolts / disp.zoom;
        if (effFS <= 0) return;

        // Use the first record only for display (stable)
        unsigned N = samplesPerRecord;
        if (N > data.size()) N = static_cast<unsigned>(data.size());
        if (N < 2) return;

        const int pixN = static_cast<int>(plotW);
        const int step = (static_cast<int>(N) + pixN - 1) / pixN;   // >=1
        const size_t pts = (N + step - 1) / step;
        if (pts < 2) return;

        std::vector<PointF> poly;
        poly.reserve(pts);
        for (unsigned i = 0; i < N; i += step) {
            const uint16_t code = data[i];
            // Unsigned full-scale: 0x0000 → -FS, 0x8000 → 0V, 0xFFFF → +FS.
            // Apply software zoom by remapping to the zoomed FS range.
            const double v = (static_cast<double>(code) - 32768.0) / 32768.0
                             * fullScaleVolts;
            double y = 0.5 - (v / (2.0 * effFS));        // 0 at top, 1 at bottom
            y -= disp.offsetDiv / 8.0;                   // offset in divisions
            if (y < 0.0) y = 0.0;
            if (y > 1.0) y = 1.0;
            const double x = static_cast<double>(i) / (N - 1);
            poly.emplace_back(static_cast<float>(plotX + x * plotW),
                              static_cast<float>(plotY + y * plotH));
        }
        g.DrawLines(&pen, poly.data(), static_cast<INT>(poly.size()));
    };

    // CH A — yellow, CH B — cyan (classic scope colours)
    plotChannel(m_latest->channelA, m_latest->samplesPerRecord,
                m_latest->fullScaleVoltsA, doc->displayA(),
                Color(255, 230, 210, 50));
    plotChannel(m_latest->channelB, m_latest->samplesPerRecord,
                m_latest->fullScaleVoltsB, doc->displayB(),
                Color(255, 50, 220, 230));
}
