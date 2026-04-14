#pragma once

#include "pch.h"
#include "AtsAcquisition.h"
#include <memory>

// CScopeDoc owns the AcquisitionEngine and the current per-channel display
// settings (zoom, offset). It does NOT own rendering state — the view pulls
// frames from the engine directly and re-renders on its own timer.
class CScopeDoc : public CDocument {
    DECLARE_DYNCREATE(CScopeDoc)

protected:
    CScopeDoc();

public:
    // Acquisition --------------------------------------------------------
    ats::AcquisitionEngine& engine() { return m_engine; }

    // Apply current m_config to the board. Stops the engine if running,
    // reconfigures, does NOT auto-restart.
    bool Reconfigure();

    bool Start();
    void Stop();
    bool IsRunning() const { return m_engine.isRunning(); }

    // Config accessors ---------------------------------------------------
    ats::AcquisitionConfig& config() { return m_config; }

    // Per-channel display (software) knobs -------------------------------
    struct Display {
        bool    enabled   = true;
        double  zoom      = 1.0;    // 1.0 = native; 2.0 = zoom in 2x
        double  offsetDiv = 0.0;    // vertical offset in divisions
    };
    Display& displayA() { return m_displayA; }
    Display& displayB() { return m_displayB; }

    // Last status string (shown on the control panel).
    void SetStatus(const CString& s) { m_status = s; UpdateAllViews(nullptr); }
    const CString& Status() const    { return m_status; }

    ~CScopeDoc() override;

protected:
    BOOL OnNewDocument() override;

private:
    ats::AcquisitionEngine m_engine;
    ats::AcquisitionConfig m_config;

    Display m_displayA;
    Display m_displayB;

    CString m_status = L"Stopped";
};
