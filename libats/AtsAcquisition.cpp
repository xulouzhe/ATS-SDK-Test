// AtsAcquisition.cpp
#include "AtsAcquisition.h"

#include <AlazarApi.h>
#include <AlazarCmd.h>
#include <AlazarError.h>

#include <cstdio>
#include <cstring>
#include <sstream>

namespace ats {

// ---------------------------------------------------------------------------
// Defaults + enum mappings.
// ---------------------------------------------------------------------------

AcquisitionConfig AcquisitionConfig::defaultForAts9351() {
    AcquisitionConfig c;
    c.sampleRateId     = SAMPLE_RATE_500MSPS;
    c.inputRangeA      = INPUT_RANGE_PM_400_MV;
    c.inputRangeB      = INPUT_RANGE_PM_400_MV;
    c.triggerChannel   = TRIG_CHAN_A;
    return c;
}

double sampleRateIdToHz(unsigned rateId) {
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

double inputRangeIdToVolts(unsigned rangeId) {
    switch (rangeId) {
        case INPUT_RANGE_PM_20_MV:  return 0.020;
        case INPUT_RANGE_PM_40_MV:  return 0.040;
        case INPUT_RANGE_PM_50_MV:  return 0.050;
        case INPUT_RANGE_PM_80_MV:  return 0.080;
        case INPUT_RANGE_PM_100_MV: return 0.100;
        case INPUT_RANGE_PM_200_MV: return 0.200;
        case INPUT_RANGE_PM_400_MV: return 0.400;
        case INPUT_RANGE_PM_500_MV: return 0.500;
        case INPUT_RANGE_PM_800_MV: return 0.800;
        case INPUT_RANGE_PM_1_V:    return 1.000;
        case INPUT_RANGE_PM_2_V:    return 2.000;
        case INPUT_RANGE_PM_4_V:    return 4.000;
        case INPUT_RANGE_PM_5_V:    return 5.000;
        case INPUT_RANGE_PM_8_V:    return 8.000;
        case INPUT_RANGE_PM_10_V:   return 10.000;
        default:                    return 0.0;
    }
}

// Small helpers ------------------------------------------------------------

static unsigned countChannels(Channel ch) {
    unsigned mask = static_cast<unsigned>(ch);
    unsigned n = 0;
    while (mask) { n += (mask & 1u); mask >>= 1; }
    return n;
}

static unsigned atsCouplingOf(Coupling c) {
    return (c == Coupling::DC) ? DC_COUPLING : AC_COUPLING;
}

static unsigned atsSlopeOf(TriggerSlope s) {
    return (s == TriggerSlope::Positive) ? TRIGGER_SLOPE_POSITIVE
                                         : TRIGGER_SLOPE_NEGATIVE;
}

// ---------------------------------------------------------------------------
// AcquisitionEngine
// ---------------------------------------------------------------------------

AcquisitionEngine::AcquisitionEngine() = default;

AcquisitionEngine::~AcquisitionEngine() {
    stop();
    releaseBuffers();
}

void AcquisitionEngine::setError(const char* where, int atsCode) {
    std::ostringstream os;
    os << where << " -> " << atsCode << " ("
       << AlazarErrorToText(static_cast<RETURN_CODE>(atsCode)) << ")";
    m_lastError = os.str();
}

bool AcquisitionEngine::open(unsigned systemId, unsigned boardId) {
    HANDLE h = AlazarGetBoardBySystemID(systemId, boardId);
    if (!h) {
        m_lastError = "AlazarGetBoardBySystemID returned NULL";
        return false;
    }
    m_board = h;

    U8  bits = 0;
    U32 maxSamples = 0;
    RETURN_CODE rc = AlazarGetChannelInfo(h, &maxSamples, &bits);
    if (rc != ApiSuccess) {
        setError("AlazarGetChannelInfo", rc);
        return false;
    }
    m_bitsPerSample = bits;
    return true;
}

bool AcquisitionEngine::configure(const AcquisitionConfig& cfg) {
    if (m_running.load()) {
        m_lastError = "configure() called while running; call stop() first";
        return false;
    }
    if (!m_board) {
        m_lastError = "configure() called before open()";
        return false;
    }

    m_cfg = cfg;
    HANDLE h = static_cast<HANDLE>(m_board);
    RETURN_CODE rc;

    rc = AlazarSetCaptureClock(h, INTERNAL_CLOCK, cfg.sampleRateId,
                               CLOCK_EDGE_RISING, 0);
    if (rc != ApiSuccess) { setError("AlazarSetCaptureClock", rc); return false; }

    rc = AlazarInputControlEx(h, CHANNEL_A, atsCouplingOf(cfg.couplingA),
                              cfg.inputRangeA, IMPEDANCE_50_OHM);
    if (rc != ApiSuccess) { setError("AlazarInputControlEx A", rc); return false; }

    rc = AlazarInputControlEx(h, CHANNEL_B, atsCouplingOf(cfg.couplingB),
                              cfg.inputRangeB, IMPEDANCE_50_OHM);
    if (rc != ApiSuccess) { setError("AlazarInputControlEx B", rc); return false; }

    AlazarSetBWLimit(h, CHANNEL_A, 0);
    AlazarSetBWLimit(h, CHANNEL_B, 0);

    rc = AlazarSetTriggerOperation(h,
             TRIG_ENGINE_OP_J,
             TRIG_ENGINE_J, cfg.triggerChannel, atsSlopeOf(cfg.triggerSlope),
             cfg.triggerLevelCode,
             TRIG_ENGINE_K, TRIG_DISABLE, TRIGGER_SLOPE_POSITIVE, 128);
    if (rc != ApiSuccess) { setError("AlazarSetTriggerOperation", rc); return false; }

    rc = AlazarSetExternalTrigger(h, DC_COUPLING, ETR_5V);
    if (rc != ApiSuccess) { setError("AlazarSetExternalTrigger", rc); return false; }

    rc = AlazarSetTriggerDelay(h, 0);
    if (rc != ApiSuccess) { setError("AlazarSetTriggerDelay", rc); return false; }

    rc = AlazarSetTriggerTimeOut(h, cfg.triggerTimeoutMs);
    if (rc != ApiSuccess) { setError("AlazarSetTriggerTimeOut", rc); return false; }

    rc = AlazarConfigureAuxIO(h, AUX_OUT_TRIGGER, 0);
    if (rc != ApiSuccess) { setError("AlazarConfigureAuxIO", rc); return false; }

    // Explicit record size — matches AlazarTech's reference sample.
    rc = AlazarSetRecordSize(h, cfg.preTriggerSamples, cfg.postTriggerSamples);
    if (rc != ApiSuccess) { setError("AlazarSetRecordSize", rc); return false; }

    m_samplesPerRecord = cfg.preTriggerSamples + cfg.postTriggerSamples;
    const unsigned channelCount = countChannels(cfg.channels);
    const unsigned bytesPerSample = (m_bitsPerSample + 7) / 8;
    m_bytesPerBuffer = static_cast<size_t>(m_samplesPerRecord) *
                       bytesPerSample *
                       cfg.recordsPerBuffer *
                       channelCount;

    return true;
}

bool AcquisitionEngine::allocateBuffers() {
    releaseBuffers();
    HANDLE h = static_cast<HANDLE>(m_board);
    m_buffers.assign(m_cfg.dmaBufferCount, nullptr);
    for (unsigned i = 0; i < m_cfg.dmaBufferCount; ++i) {
        U16* p = AlazarAllocBufferU16(h, static_cast<U32>(m_bytesPerBuffer));
        if (!p) {
            m_lastError = "AlazarAllocBufferU16 failed";
            releaseBuffers();
            return false;
        }
        m_buffers[i] = p;
    }
    return true;
}

void AcquisitionEngine::releaseBuffers() {
    if (!m_board) return;
    HANDLE h = static_cast<HANDLE>(m_board);
    for (uint16_t* b : m_buffers) {
        if (b) AlazarFreeBufferU16(h, b);
    }
    m_buffers.clear();
}

bool AcquisitionEngine::armBoardLocked() {
    HANDLE h = static_cast<HANDLE>(m_board);

    // Infinite acquisition for free-running mode: we never stop on a record
    // count, only when stop() aborts DMA.
    constexpr U32 infiniteRecords = 0x7FFFFFFFu;

    RETURN_CODE rc = AlazarBeforeAsyncRead(h,
        static_cast<U32>(m_cfg.channels),
        -static_cast<long>(m_cfg.preTriggerSamples),
        m_samplesPerRecord,
        m_cfg.recordsPerBuffer,
        infiniteRecords,
        ADMA_EXTERNAL_STARTCAPTURE | ADMA_NPT);
    if (rc != ApiSuccess) { setError("AlazarBeforeAsyncRead", rc); return false; }

    for (uint16_t* b : m_buffers) {
        rc = AlazarPostAsyncBuffer(h, b, static_cast<U32>(m_bytesPerBuffer));
        if (rc != ApiSuccess) { setError("AlazarPostAsyncBuffer", rc); return false; }
    }

    rc = AlazarStartCapture(h);
    if (rc != ApiSuccess) { setError("AlazarStartCapture", rc); return false; }
    return true;
}

bool AcquisitionEngine::start() {
    if (m_running.load()) return true;
    if (!m_board) { m_lastError = "start() before open()"; return false; }
    if (m_bytesPerBuffer == 0) { m_lastError = "start() before configure()"; return false; }

    if (!allocateBuffers())   return false;
    if (!armBoardLocked())    { releaseBuffers(); return false; }

    m_shouldStop.store(false);
    m_running.store(true);
    m_framesProduced.store(0);
    m_framesDropped.store(0);
    m_sequence.store(0);

    m_worker = std::thread(&AcquisitionEngine::workerLoop, this);
    return true;
}

void AcquisitionEngine::stop() {
    if (!m_running.load() && !m_worker.joinable()) return;
    m_shouldStop.store(true);
    // Abort DMA first so the worker's blocking AlazarWaitAsyncBufferComplete
    // returns immediately instead of waiting for the 5 s timeout.
    if (m_board) {
        AlazarAbortAsyncRead(static_cast<HANDLE>(m_board));
    }
    if (m_worker.joinable()) m_worker.join();
    releaseBuffers();
    m_running.store(false);
}

void AcquisitionEngine::workerLoop() {
    HANDLE h = static_cast<HANDLE>(m_board);
    const unsigned channelCount   = countChannels(m_cfg.channels);
    const bool     captureA       = (static_cast<unsigned>(m_cfg.channels) & 0x1) != 0;
    const bool     captureB       = (static_cast<unsigned>(m_cfg.channels) & 0x2) != 0;
    const unsigned samplesPerRec  = m_samplesPerRecord;
    const unsigned recordsPerBuf  = m_cfg.recordsPerBuffer;
    const size_t   totalSamples   = static_cast<size_t>(samplesPerRec) * recordsPerBuf;
    const double   sampleRateHz   = sampleRateIdToHz(m_cfg.sampleRateId);
    const double   fsA            = inputRangeIdToVolts(m_cfg.inputRangeA);
    const double   fsB            = inputRangeIdToVolts(m_cfg.inputRangeB);

    unsigned bufIdx = 0;
    while (!m_shouldStop.load()) {
        uint16_t* buf = m_buffers[bufIdx % m_cfg.dmaBufferCount];

        // 5 s timeout — caller can always raise this if long-period triggers.
        RETURN_CODE rc = AlazarWaitAsyncBufferComplete(h, buf, 5000);
        if (rc != ApiSuccess) {
            if (m_shouldStop.load()) break;
            setError("AlazarWaitAsyncBufferComplete", rc);
            break;
        }

        // Layout inside the DMA buffer (per AlazarTech docs):
        //   [R0A, R1A, ... R(n-1)A, R0B, R1B, ... R(n-1)B]
        // Each sample is a uint16 in little-endian, unsigned full-scale.
        auto frame = std::make_shared<Frame>();
        frame->samplesPerRecord = samplesPerRec;
        frame->recordsPerFrame  = recordsPerBuf;
        frame->sampleRateHz     = sampleRateHz;
        frame->fullScaleVoltsA  = fsA;
        frame->fullScaleVoltsB  = fsB;
        frame->sequenceNumber   = m_sequence.fetch_add(1);

        const uint16_t* src = buf;
        if (captureA) {
            frame->channelA.assign(src, src + totalSamples);
            src += totalSamples;
        }
        if (captureB) {
            frame->channelB.assign(src, src + totalSamples);
        }

        {
            std::lock_guard<std::mutex> lk(m_frameMutex);
            if (m_latestFrame && m_frameIsFresh) {
                m_framesDropped.fetch_add(1);
            }
            m_latestFrame  = frame;
            m_frameIsFresh = true;
        }
        m_framesProduced.fetch_add(1);

        // Re-post this buffer to the board.
        rc = AlazarPostAsyncBuffer(h, buf, static_cast<U32>(m_bytesPerBuffer));
        if (rc != ApiSuccess) {
            setError("AlazarPostAsyncBuffer", rc);
            break;
        }
        bufIdx++;
    }
}

std::shared_ptr<const Frame> AcquisitionEngine::latestFrame() {
    std::lock_guard<std::mutex> lk(m_frameMutex);
    if (!m_frameIsFresh) return nullptr;
    m_frameIsFresh = false;
    return m_latestFrame;
}

} // namespace ats
