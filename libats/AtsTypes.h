// AtsTypes.h — shared configuration + frame types for libats.
#pragma once

#include <cstdint>
#include <vector>

namespace ats {

enum class Channel : uint32_t {
    A  = 0x1,
    B  = 0x2,
    AB = 0x3,
};

enum class TriggerSlope { Positive, Negative };
enum class Coupling     { AC, DC };

// Configuration for one acquisition run.
// Defaults are ATS9351-friendly: 500 MS/s, ±400 mV DC on both channels,
// CH A rising trigger at ~+17% FS, 2048 samples per record.
struct AcquisitionConfig {
    // Clock --------------------------------------------------------------
    // Use SAMPLE_RATE_* constants from AlazarCmd.h. See AtsAcquisition.cpp
    // for the default (SAMPLE_RATE_500MSPS).
    unsigned sampleRateId   = 0;        // filled by AcquisitionConfig::defaultForAts9351()

    // Input --------------------------------------------------------------
    // Use INPUT_RANGE_PM_* constants. ATS9351 only supports PM_400_MV.
    unsigned inputRangeA    = 0;        // defaulted in factory
    unsigned inputRangeB    = 0;
    Coupling couplingA      = Coupling::DC;
    Coupling couplingB      = Coupling::DC;

    // Channel mask -------------------------------------------------------
    Channel  channels       = Channel::AB;

    // Record shape -------------------------------------------------------
    unsigned preTriggerSamples  = 0;
    unsigned postTriggerSamples = 2048; // must be a multiple of 32
    unsigned recordsPerBuffer   = 10;
    unsigned dmaBufferCount     = 4;

    // Trigger ------------------------------------------------------------
    // triggerChannel: TRIG_CHAN_A / TRIG_CHAN_B / TRIG_EXTERNAL from AlazarCmd.h
    unsigned     triggerChannel   = 0;  // defaulted in factory
    TriggerSlope triggerSlope     = TriggerSlope::Positive;
    uint8_t      triggerLevelCode = 150; // 0..255, 128 = 0V ≈ +17% FS
    unsigned     triggerTimeoutMs = 0;   // 0 = wait forever

    // Factory producing the defaults used by the reference AlazarTech NPT sample.
    static AcquisitionConfig defaultForAts9351();
};

// One captured frame: `recordsPerFrame` records per channel, each of
// `samplesPerRecord` samples. Raw 12-bit samples stored in the upper bits
// of each uint16 (0x0000 = -FS, 0x8000 ≈ 0 V, 0xFFFF = +FS).
struct Frame {
    std::vector<uint16_t> channelA;   // empty if CH A not captured
    std::vector<uint16_t> channelB;   // empty if CH B not captured

    unsigned samplesPerRecord = 0;
    unsigned recordsPerFrame  = 0;
    double   sampleRateHz     = 0.0;
    double   fullScaleVoltsA  = 0.4;  // signed full-scale, i.e. ±400 mV → 0.4
    double   fullScaleVoltsB  = 0.4;
    uint64_t sequenceNumber   = 0;    // monotonic frame counter

    // Helpers --------------------------------------------------------------
    double sampleIntervalSec() const {
        return sampleRateHz > 0.0 ? 1.0 / sampleRateHz : 0.0;
    }

    // Convert a raw uint16 code to volts using the unsigned full-scale mapping.
    // code = 0x0000 → -FS,  0x8000 → 0 V,  0xFFFF → +FS.
    static inline double codeToVolts(uint16_t code, double fullScaleVolts) {
        constexpr double kMid = 32768.0;
        return (static_cast<double>(code) - kMid) / kMid * fullScaleVolts;
    }
};

// SAMPLE_RATE_* → Hz.
double sampleRateIdToHz(unsigned sampleRateId);

// INPUT_RANGE_PM_* → full-scale volts (e.g. PM_400_MV → 0.4).
double inputRangeIdToVolts(unsigned inputRangeId);

} // namespace ats
