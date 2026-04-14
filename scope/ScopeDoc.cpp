#include "pch.h"
#include "ScopeDoc.h"

IMPLEMENT_DYNCREATE(CScopeDoc, CDocument)

CScopeDoc::CScopeDoc() {
    m_config = ats::AcquisitionConfig::defaultForAts9351();
}

CScopeDoc::~CScopeDoc() {
    Stop();
}

BOOL CScopeDoc::OnNewDocument() {
    if (!CDocument::OnNewDocument()) return FALSE;

    // Open the board once per document. If it fails we still return TRUE so
    // the app window comes up; the status will show the error.
    if (!m_engine.open()) {
        m_status.Format(L"Board open failed: %S", m_engine.lastError().c_str());
    } else {
        m_status = L"Stopped (board ready)";
    }
    return TRUE;
}

bool CScopeDoc::Reconfigure() {
    bool wasRunning = m_engine.isRunning();
    if (wasRunning) m_engine.stop();

    if (!m_engine.configure(m_config)) {
        m_status.Format(L"Configure failed: %S", m_engine.lastError().c_str());
        UpdateAllViews(nullptr);
        return false;
    }
    m_status = L"Configured (press Run)";
    UpdateAllViews(nullptr);
    return true;
}

bool CScopeDoc::Start() {
    if (!Reconfigure()) return false;
    if (!m_engine.start()) {
        m_status.Format(L"Start failed: %S", m_engine.lastError().c_str());
        UpdateAllViews(nullptr);
        return false;
    }
    m_status = L"Running";
    UpdateAllViews(nullptr);
    return true;
}

void CScopeDoc::Stop() {
    m_engine.stop();
    m_status = L"Stopped";
    UpdateAllViews(nullptr);
}
