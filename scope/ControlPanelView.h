#pragma once

#include "pch.h"
#include "Resource.h"

class CScopeDoc;

// Left-pane form view: holds all the interactive controls.
// Reads/writes into the document's config + display state, then calls
// doc->Reconfigure() or doc->Start()/Stop() as appropriate.
class CControlPanelView : public CFormView {
    DECLARE_DYNCREATE(CControlPanelView)

protected:
    CControlPanelView();
    ~CControlPanelView() override = default;

public:
    enum { IDD = IDD_CONTROL_PANEL };

    CScopeDoc* GetDocument() const;

    void OnInitialUpdate() override;
    void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) override;
    void DoDataExchange(CDataExchange* pDX) override;

    afx_msg void OnRunStop();
    afx_msg void OnSampleRateChanged();
    afx_msg void OnSamplesPerRecChanged();
    afx_msg void OnChannelAChanged();
    afx_msg void OnRangeAChanged();
    afx_msg void OnCouplingAChanged();
    afx_msg void OnChannelBChanged();
    afx_msg void OnRangeBChanged();
    afx_msg void OnCouplingBChanged();
    afx_msg void OnTriggerSourceChanged();
    afx_msg void OnTriggerSlopeChanged();
    afx_msg void OnHScroll(UINT code, UINT pos, CScrollBar* pSB);

    DECLARE_MESSAGE_MAP()

private:
    // Controls -----------------------------------------------------------
    CButton       m_btnRunStop;
    CStatic       m_lblStatus;

    CComboBox     m_cmbSampleRate;
    CComboBox     m_cmbSamplesPerRec;

    CButton       m_chkEnableA;
    CComboBox     m_cmbRangeA;
    CComboBox     m_cmbCouplingA;
    CSliderCtrl   m_sldZoomA;
    CStatic       m_lblZoomA;
    CSliderCtrl   m_sldOffsetA;

    CButton       m_chkEnableB;
    CComboBox     m_cmbRangeB;
    CComboBox     m_cmbCouplingB;
    CSliderCtrl   m_sldZoomB;
    CStatic       m_lblZoomB;

    CComboBox     m_cmbTrigSource;
    CComboBox     m_cmbTrigSlope;
    CSliderCtrl   m_sldTrigLevel;
    CStatic       m_lblTrigLevel;

    // Population helpers -------------------------------------------------
    void PopulateCombos();
    void PullFromDoc();       // doc -> widgets
    void PushToDoc();         // widgets -> doc.config (no restart)

    bool m_initialised = false;
};
