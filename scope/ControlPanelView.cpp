#include "pch.h"
#include "ControlPanelView.h"
#include "ScopeDoc.h"

#include <AlazarCmd.h>   // SAMPLE_RATE_*, INPUT_RANGE_*, TRIG_CHAN_*

IMPLEMENT_DYNCREATE(CControlPanelView, CFormView)

BEGIN_MESSAGE_MAP(CControlPanelView, CFormView)
    ON_BN_CLICKED(IDC_RUN_STOP,          &CControlPanelView::OnRunStop)
    ON_CBN_SELCHANGE(IDC_SAMPLE_RATE,        &CControlPanelView::OnSampleRateChanged)
    ON_CBN_SELCHANGE(IDC_SAMPLES_PER_RECORD, &CControlPanelView::OnSamplesPerRecChanged)
    ON_BN_CLICKED(IDC_ENABLE_A,          &CControlPanelView::OnChannelAChanged)
    ON_CBN_SELCHANGE(IDC_RANGE_A,            &CControlPanelView::OnRangeAChanged)
    ON_CBN_SELCHANGE(IDC_COUPLING_A,         &CControlPanelView::OnCouplingAChanged)
    ON_BN_CLICKED(IDC_ENABLE_B,          &CControlPanelView::OnChannelBChanged)
    ON_CBN_SELCHANGE(IDC_RANGE_B,            &CControlPanelView::OnRangeBChanged)
    ON_CBN_SELCHANGE(IDC_COUPLING_B,         &CControlPanelView::OnCouplingBChanged)
    ON_CBN_SELCHANGE(IDC_TRIGGER_SOURCE,     &CControlPanelView::OnTriggerSourceChanged)
    ON_CBN_SELCHANGE(IDC_TRIGGER_SLOPE,      &CControlPanelView::OnTriggerSlopeChanged)
    ON_WM_HSCROLL()
END_MESSAGE_MAP()

CControlPanelView::CControlPanelView() : CFormView(IDD) {}

CScopeDoc* CControlPanelView::GetDocument() const {
    return reinterpret_cast<CScopeDoc*>(m_pDocument);
}

void CControlPanelView::DoDataExchange(CDataExchange* pDX) {
    CFormView::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_RUN_STOP,            m_btnRunStop);
    DDX_Control(pDX, IDC_STATUS,              m_lblStatus);
    DDX_Control(pDX, IDC_SAMPLE_RATE,         m_cmbSampleRate);
    DDX_Control(pDX, IDC_SAMPLES_PER_RECORD,  m_cmbSamplesPerRec);
    DDX_Control(pDX, IDC_ENABLE_A,            m_chkEnableA);
    DDX_Control(pDX, IDC_RANGE_A,             m_cmbRangeA);
    DDX_Control(pDX, IDC_COUPLING_A,          m_cmbCouplingA);
    DDX_Control(pDX, IDC_ZOOM_A,              m_sldZoomA);
    DDX_Control(pDX, IDC_ZOOM_A_LABEL,        m_lblZoomA);
    DDX_Control(pDX, IDC_OFFSET_A,            m_sldOffsetA);
    DDX_Control(pDX, IDC_ENABLE_B,            m_chkEnableB);
    DDX_Control(pDX, IDC_RANGE_B,             m_cmbRangeB);
    DDX_Control(pDX, IDC_COUPLING_B,          m_cmbCouplingB);
    DDX_Control(pDX, IDC_ZOOM_B,              m_sldZoomB);
    DDX_Control(pDX, IDC_ZOOM_B_LABEL,        m_lblZoomB);
    DDX_Control(pDX, IDC_TRIGGER_SOURCE,      m_cmbTrigSource);
    DDX_Control(pDX, IDC_TRIGGER_SLOPE,       m_cmbTrigSlope);
    DDX_Control(pDX, IDC_TRIGGER_LEVEL,       m_sldTrigLevel);
    DDX_Control(pDX, IDC_TRIGGER_LEVEL_LABEL, m_lblTrigLevel);
}

// Sample-rate & range tables. Each entry is (label, AlazarCmd constant).
namespace {
    struct Entry { const wchar_t* label; unsigned id; };

    const Entry kRates[] = {
        { L"1 kS/s",      SAMPLE_RATE_1KSPS },
        { L"10 kS/s",     SAMPLE_RATE_10KSPS },
        { L"100 kS/s",    SAMPLE_RATE_100KSPS },
        { L"1 MS/s",      SAMPLE_RATE_1MSPS },
        { L"10 MS/s",     SAMPLE_RATE_10MSPS },
        { L"50 MS/s",     SAMPLE_RATE_50MSPS },
        { L"100 MS/s",    SAMPLE_RATE_100MSPS },
        { L"125 MS/s",    SAMPLE_RATE_125MSPS },
        { L"200 MS/s",    SAMPLE_RATE_200MSPS },
        { L"250 MS/s",    SAMPLE_RATE_250MSPS },
        { L"500 MS/s",    SAMPLE_RATE_500MSPS },
    };
    constexpr unsigned kSampleCounts[] = { 256, 512, 1024, 2048, 4096, 8192, 16384 };

    // ATS9351 HW range is fixed at ±400 mV. Shown for completeness.
    const Entry kRanges[] = {
        { L"\u00b1400 mV (hardware fixed)", INPUT_RANGE_PM_400_MV },
    };
    const Entry kCouplings[] = {
        { L"DC", 0 },  // maps to Coupling::DC
        { L"AC", 1 },  // maps to Coupling::AC
    };
    const Entry kTrigSources[] = {
        { L"Channel A", TRIG_CHAN_A },
        { L"Channel B", TRIG_CHAN_B },
    };
    const Entry kTrigSlopes[] = {
        { L"Positive (rising)",  0 },
        { L"Negative (falling)", 1 },
    };

    // Utility: find index matching id; returns 0 if not found.
    template <size_t N>
    int findIdx(const Entry (&arr)[N], unsigned id) {
        for (size_t i = 0; i < N; ++i) if (arr[i].id == id) return (int)i;
        return 0;
    }
}

void CControlPanelView::PopulateCombos() {
    for (const auto& e : kRates)      m_cmbSampleRate.AddString(e.label);
    for (unsigned n : kSampleCounts) {
        WCHAR buf[16]; swprintf_s(buf, L"%u", n);
        m_cmbSamplesPerRec.AddString(buf);
    }
    for (const auto& e : kRanges) {
        m_cmbRangeA.AddString(e.label);
        m_cmbRangeB.AddString(e.label);
    }
    for (const auto& e : kCouplings) {
        m_cmbCouplingA.AddString(e.label);
        m_cmbCouplingB.AddString(e.label);
    }
    for (const auto& e : kTrigSources) m_cmbTrigSource.AddString(e.label);
    for (const auto& e : kTrigSlopes)  m_cmbTrigSlope.AddString(e.label);

    // Zoom slider: integer 1..20  →  1.0x .. 20.0x.
    m_sldZoomA.SetRange(1, 20, TRUE);
    m_sldZoomA.SetPos(1);
    m_sldZoomA.SetTicFreq(1);
    m_sldZoomB.SetRange(1, 20, TRUE);
    m_sldZoomB.SetPos(1);
    m_sldZoomB.SetTicFreq(1);

    // Offset slider: -40..+40  →  -4..+4 divisions.
    m_sldOffsetA.SetRange(-40, 40, TRUE);
    m_sldOffsetA.SetPos(0);
    m_sldOffsetA.SetTicFreq(10);

    // Trigger level: 0..255 (board uses 8-bit level).
    m_sldTrigLevel.SetRange(0, 255, TRUE);
    m_sldTrigLevel.SetPos(150);
    m_sldTrigLevel.SetTicFreq(16);
}

void CControlPanelView::OnInitialUpdate() {
    CFormView::OnInitialUpdate();
    if (!m_initialised) {
        PopulateCombos();
        m_initialised = true;
    }
    PullFromDoc();
}

void CControlPanelView::OnUpdate(CView*, LPARAM, CObject*) {
    if (CScopeDoc* doc = GetDocument()) {
        m_lblStatus.SetWindowText(doc->Status());
        m_btnRunStop.SetWindowText(doc->IsRunning() ? L"Stop" : L"Run");
    }
}

void CControlPanelView::PullFromDoc() {
    CScopeDoc* doc = GetDocument();
    if (!doc) return;
    const auto& cfg = doc->config();

    m_cmbSampleRate.SetCurSel(findIdx(kRates, cfg.sampleRateId));

    int spIdx = 0;
    for (unsigned i = 0; i < _countof(kSampleCounts); ++i)
        if (kSampleCounts[i] == cfg.postTriggerSamples) spIdx = (int)i;
    m_cmbSamplesPerRec.SetCurSel(spIdx);

    m_cmbRangeA.SetCurSel(findIdx(kRanges, cfg.inputRangeA));
    m_cmbRangeB.SetCurSel(findIdx(kRanges, cfg.inputRangeB));
    m_cmbCouplingA.SetCurSel(cfg.couplingA == ats::Coupling::DC ? 0 : 1);
    m_cmbCouplingB.SetCurSel(cfg.couplingB == ats::Coupling::DC ? 0 : 1);

    m_chkEnableA.SetCheck(doc->displayA().enabled ? BST_CHECKED : BST_UNCHECKED);
    m_chkEnableB.SetCheck(doc->displayB().enabled ? BST_CHECKED : BST_UNCHECKED);

    m_cmbTrigSource.SetCurSel(findIdx(kTrigSources, cfg.triggerChannel));
    m_cmbTrigSlope.SetCurSel(cfg.triggerSlope == ats::TriggerSlope::Positive ? 0 : 1);

    m_sldTrigLevel.SetPos(cfg.triggerLevelCode);
    WCHAR t[16]; swprintf_s(t, L"%u", cfg.triggerLevelCode);
    m_lblTrigLevel.SetWindowText(t);

    m_lblStatus.SetWindowText(doc->Status());
    m_btnRunStop.SetWindowText(doc->IsRunning() ? L"Stop" : L"Run");
}

void CControlPanelView::PushToDoc() {
    CScopeDoc* doc = GetDocument();
    if (!doc) return;
    auto& cfg = doc->config();

    int i = m_cmbSampleRate.GetCurSel();
    if (i >= 0) cfg.sampleRateId = kRates[i].id;

    i = m_cmbSamplesPerRec.GetCurSel();
    if (i >= 0) cfg.postTriggerSamples = kSampleCounts[i];

    i = m_cmbRangeA.GetCurSel();   if (i >= 0) cfg.inputRangeA = kRanges[i].id;
    i = m_cmbRangeB.GetCurSel();   if (i >= 0) cfg.inputRangeB = kRanges[i].id;

    i = m_cmbCouplingA.GetCurSel();
    cfg.couplingA = (i == 1) ? ats::Coupling::AC : ats::Coupling::DC;
    i = m_cmbCouplingB.GetCurSel();
    cfg.couplingB = (i == 1) ? ats::Coupling::AC : ats::Coupling::DC;

    i = m_cmbTrigSource.GetCurSel();
    if (i >= 0) cfg.triggerChannel = kTrigSources[i].id;
    i = m_cmbTrigSlope.GetCurSel();
    cfg.triggerSlope = (i == 1) ? ats::TriggerSlope::Negative
                                : ats::TriggerSlope::Positive;
    cfg.triggerLevelCode = static_cast<uint8_t>(m_sldTrigLevel.GetPos());

    // Channel mask
    unsigned mask = 0;
    if (m_chkEnableA.GetCheck() == BST_CHECKED) mask |= 0x1;
    if (m_chkEnableB.GetCheck() == BST_CHECKED) mask |= 0x2;
    if (mask == 0) mask = 0x1;   // always keep at least one
    cfg.channels = static_cast<ats::Channel>(mask);

    // Display state ------------------------------------------------------
    doc->displayA().enabled = (m_chkEnableA.GetCheck() == BST_CHECKED);
    doc->displayB().enabled = (m_chkEnableB.GetCheck() == BST_CHECKED);
    doc->displayA().zoom    = static_cast<double>(m_sldZoomA.GetPos());
    doc->displayB().zoom    = static_cast<double>(m_sldZoomB.GetPos());
    doc->displayA().offsetDiv = m_sldOffsetA.GetPos() / 10.0;
}

// ---- Handlers ---------------------------------------------------------

void CControlPanelView::OnRunStop() {
    CScopeDoc* doc = GetDocument();
    if (!doc) return;
    PushToDoc();
    if (doc->IsRunning()) doc->Stop();
    else                  doc->Start();
    m_btnRunStop.SetWindowText(doc->IsRunning() ? L"Stop" : L"Run");
    m_lblStatus.SetWindowText(doc->Status());
}

static void RestartIfRunning(CScopeDoc* doc) {
    if (doc->IsRunning()) { doc->Stop(); doc->Start(); }
}

void CControlPanelView::OnSampleRateChanged()    { PushToDoc(); RestartIfRunning(GetDocument()); }
void CControlPanelView::OnSamplesPerRecChanged() { PushToDoc(); RestartIfRunning(GetDocument()); }
void CControlPanelView::OnRangeAChanged()        { PushToDoc(); RestartIfRunning(GetDocument()); }
void CControlPanelView::OnRangeBChanged()        { PushToDoc(); RestartIfRunning(GetDocument()); }
void CControlPanelView::OnCouplingAChanged()     { PushToDoc(); RestartIfRunning(GetDocument()); }
void CControlPanelView::OnCouplingBChanged()     { PushToDoc(); RestartIfRunning(GetDocument()); }
void CControlPanelView::OnTriggerSourceChanged() { PushToDoc(); RestartIfRunning(GetDocument()); }
void CControlPanelView::OnTriggerSlopeChanged()  { PushToDoc(); RestartIfRunning(GetDocument()); }

void CControlPanelView::OnChannelAChanged() {
    PushToDoc();
    RestartIfRunning(GetDocument());
}
void CControlPanelView::OnChannelBChanged() {
    PushToDoc();
    RestartIfRunning(GetDocument());
}

void CControlPanelView::OnHScroll(UINT code, UINT pos, CScrollBar* pSB) {
    CFormView::OnHScroll(code, pos, pSB);
    if (!pSB) return;
    HWND hw = pSB->GetSafeHwnd();

    if (hw == m_sldZoomA.GetSafeHwnd()) {
        int p = m_sldZoomA.GetPos();
        WCHAR b[16]; swprintf_s(b, L"%d.0x", p);
        m_lblZoomA.SetWindowText(b);
        if (auto* doc = GetDocument()) doc->displayA().zoom = p;
    } else if (hw == m_sldZoomB.GetSafeHwnd()) {
        int p = m_sldZoomB.GetPos();
        WCHAR b[16]; swprintf_s(b, L"%d.0x", p);
        m_lblZoomB.SetWindowText(b);
        if (auto* doc = GetDocument()) doc->displayB().zoom = p;
    } else if (hw == m_sldOffsetA.GetSafeHwnd()) {
        int p = m_sldOffsetA.GetPos();
        if (auto* doc = GetDocument()) doc->displayA().offsetDiv = p / 10.0;
    } else if (hw == m_sldTrigLevel.GetSafeHwnd()) {
        int p = m_sldTrigLevel.GetPos();
        WCHAR b[16]; swprintf_s(b, L"%d", p);
        m_lblTrigLevel.SetWindowText(b);
        if (auto* doc = GetDocument()) {
            doc->config().triggerLevelCode = static_cast<uint8_t>(p);
            if (code == SB_ENDSCROLL) RestartIfRunning(doc);
        }
    }
}
