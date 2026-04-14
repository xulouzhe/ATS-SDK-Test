#pragma once

#include "pch.h"

class CScopeApp : public CWinApp {
public:
    CScopeApp() = default;

    BOOL InitInstance() override;
    int  ExitInstance() override;

private:
    ULONG_PTR m_gdiplusToken = 0;
};

extern CScopeApp theApp;
