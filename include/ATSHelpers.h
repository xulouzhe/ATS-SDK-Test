// ATSHelpers.h — small helpers shared by the test programs.
#pragma once

#include <AlazarApi.h>
#include <AlazarError.h>
#include <AlazarCmd.h>

#include <cstdio>
#include <cstdlib>
#include <string>

// Abort on any non-ApiSuccess return from an ATS call. Prints file/line/code/name.
#define ATS_CHECK(expr)                                                        \
    do {                                                                       \
        RETURN_CODE _rc = (expr);                                              \
        if (_rc != ApiSuccess) {                                               \
            std::fprintf(stderr,                                               \
                "[ATS ERROR] %s:%d  %s  ->  %d (%s)\n",                        \
                __FILE__, __LINE__, #expr,                                     \
                static_cast<int>(_rc),                                         \
                AlazarErrorToText(_rc));                                       \
            std::exit(EXIT_FAILURE);                                           \
        }                                                                      \
    } while (0)

// Non-fatal variant: logs and returns false on failure.
inline bool atsOk(RETURN_CODE rc, const char* what) {
    if (rc == ApiSuccess) return true;
    std::fprintf(stderr, "[ATS WARN] %s -> %d (%s)\n",
                 what, static_cast<int>(rc), AlazarErrorToText(rc));
    return false;
}

// Map a subset of the SAMPLE_RATE_* constants to samples-per-second.
// Extend as needed; values here cover the typical ATS9351 range.
inline double sampleRateHzFromId(U32 rateId) {
    switch (rateId) {
        case SAMPLE_RATE_1KSPS:    return 1e3;
        case SAMPLE_RATE_2KSPS:    return 2e3;
        case SAMPLE_RATE_5KSPS:    return 5e3;
        case SAMPLE_RATE_10KSPS:   return 10e3;
        case SAMPLE_RATE_20KSPS:   return 20e3;
        case SAMPLE_RATE_50KSPS:   return 50e3;
        case SAMPLE_RATE_100KSPS:  return 100e3;
        case SAMPLE_RATE_200KSPS:  return 200e3;
        case SAMPLE_RATE_500KSPS:  return 500e3;
        case SAMPLE_RATE_1MSPS:    return 1e6;
        case SAMPLE_RATE_2MSPS:    return 2e6;
        case SAMPLE_RATE_5MSPS:    return 5e6;
        case SAMPLE_RATE_10MSPS:   return 10e6;
        case SAMPLE_RATE_20MSPS:   return 20e6;
        case SAMPLE_RATE_50MSPS:   return 50e6;
        case SAMPLE_RATE_100MSPS:  return 100e6;
        case SAMPLE_RATE_125MSPS:  return 125e6;
        case SAMPLE_RATE_200MSPS:  return 200e6;
        case SAMPLE_RATE_250MSPS:  return 250e6;
        case SAMPLE_RATE_500MSPS:  return 500e6;
        default:                   return 0.0;
    }
}
