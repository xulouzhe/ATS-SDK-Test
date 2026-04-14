// Precompiled header for the MFC scope app.
#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN
#endif

#ifndef _AFX_ALL_WARNINGS
#define _AFX_ALL_WARNINGS
#endif

#include <afxwin.h>     // MFC core + standard components
#include <afxext.h>     // MFC extensions
#include <afxcmn.h>     // common controls (CSliderCtrl, CSpinButtonCtrl, ...)
#include <afxdisp.h>    // OLE automation (some MFC headers pull it in)

// GDI+ -----------------------------------------------------------------
#include <algorithm>
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")
