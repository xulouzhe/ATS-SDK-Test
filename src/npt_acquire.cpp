// npt_acquire.cpp
// -----------------------------------------------------------------------------
// AutoDMA "NPT" (No-Pre-Trigger) acquisition on an ATS9351.
//
// This is the pattern AlazarTech recommends for streaming high-rate data:
//   - Allocate N DMA buffers and post them to the board.
//   - Arm the board (AlazarStartCapture).
//   - Loop: wait for the next buffer -> process/save -> re-post it.
//   - When the acquisition ends (time or buffer count), AlazarAbortAsyncRead.
//
// Config knobs are grouped at the top of main(). Defaults:
//   - Internal 500 MS/s clock
//   - Both channels, DC-coupled, 50 ohm, ±400 mV
//   - Trigger on CH A rising through mid-scale (works with a signal generator;
//     if no signal is present the capture will idle until the timeout).
//   - 1024 samples/record, 10 records/buffer, 4 buffers, 10 total buffers.
//
// Raw samples (12-bit, MSB-aligned in 16-bit words) are appended to
// `ats9351_capture.bin`. One channel A record followed by one channel B record,
// repeated for each record, for each buffer.
// -----------------------------------------------------------------------------

#include "ATSHelpers.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

int main() {
    // -------------------------------------------------------------------------
    // Configuration — edit these before running.
    // -------------------------------------------------------------------------
    constexpr U32 systemId            = 1;
    constexpr U32 boardId             = 1;

    constexpr U32 sampleRateId        = SAMPLE_RATE_500MSPS;   // full speed
    constexpr U32 inputRange          = INPUT_RANGE_PM_400_MV; // ATS9351 default
    constexpr U32 couplingA           = DC_COUPLING;
    constexpr U32 couplingB           = DC_COUPLING;
    constexpr U32 impedance           = IMPEDANCE_50_OHM;      // fixed on ATS9351

    constexpr U32 preTriggerSamples   = 0;                     // NPT = 0
    constexpr U32 postTriggerSamples  = 1024;                  // per record
    constexpr U32 recordsPerBuffer    = 10;
    constexpr U32 buffersPerAcquire   = 10;                    // 0 = infinite
    constexpr U32 dmaBufferCount      = 4;                     // posted in flight

    // Trigger: fire when CH A crosses midscale going positive.
    // For a 12-bit board the trigger level is an 8-bit value (0..255),
    // 128 == 0 V. Shift up for a positive threshold.
    constexpr U32 triggerLevelCode    = 128 + 20;              // ~+15% of range
    constexpr U32 triggerTimeoutMs    = 0;                     // 0 = wait forever

    const char*   outputPath          = "ats9351_capture.bin";

    // -------------------------------------------------------------------------
    // Open board + sanity check model.
    // -------------------------------------------------------------------------
    HANDLE board = AlazarGetBoardBySystemID(systemId, boardId);
    if (board == nullptr) {
        std::fprintf(stderr, "AlazarGetBoardBySystemID(%u,%u) failed.\n",
                     systemId, boardId);
        return EXIT_FAILURE;
    }

    U32 kind = AlazarGetBoardKind(board);
    if (kind != ATS9351) {
        std::fprintf(stderr,
            "WARNING: board kind = %u, expected ATS9351 (%u). Continuing.\n",
            kind, static_cast<unsigned>(ATS9351));
    }

    U8  bitsPerSample = 0;
    U32 maxSamplesPerChannel = 0;
    ATS_CHECK(AlazarGetChannelInfo(board, &maxSamplesPerChannel, &bitsPerSample));
    const U32 bytesPerSample = (bitsPerSample + 7) / 8;   // -> 2 on ATS9351
    std::printf("Board ready: bits/sample=%u, bytes/sample=%u, max samples/ch=%u\n",
                bitsPerSample, bytesPerSample, maxSamplesPerChannel);

    // -------------------------------------------------------------------------
    // Clock / input / trigger configuration.
    // -------------------------------------------------------------------------
    ATS_CHECK(AlazarSetCaptureClock(board, INTERNAL_CLOCK, sampleRateId,
                                    CLOCK_EDGE_RISING, 0));

    ATS_CHECK(AlazarInputControlEx(board, CHANNEL_A, couplingA,
                                   inputRange, impedance));
    ATS_CHECK(AlazarInputControlEx(board, CHANNEL_B, couplingB,
                                   inputRange, impedance));

    // Full bandwidth (no low-pass filter).
    atsOk(AlazarSetBWLimit(board, CHANNEL_A, 0), "SetBWLimit A");
    atsOk(AlazarSetBWLimit(board, CHANNEL_B, 0), "SetBWLimit B");

    // Trigger engine J on CH A positive slope; engine K disabled.
    ATS_CHECK(AlazarSetTriggerOperation(board,
        TRIG_ENGINE_OP_J,
        TRIG_ENGINE_J, TRIG_CHAN_A, TRIGGER_SLOPE_POSITIVE, triggerLevelCode,
        TRIG_ENGINE_K, TRIG_DISABLE,  TRIGGER_SLOPE_POSITIVE, 128));

    // External trigger input is not used here, but configure sane defaults.
    ATS_CHECK(AlazarSetExternalTrigger(board, DC_COUPLING, ETR_5V));
    ATS_CHECK(AlazarSetTriggerDelay(board, 0));
    ATS_CHECK(AlazarSetTriggerTimeOut(board, triggerTimeoutMs));
    ATS_CHECK(AlazarConfigureAuxIO(board, AUX_OUT_TRIGGER, 0));

    // -------------------------------------------------------------------------
    // Allocate DMA buffers (page-aligned for best performance).
    // -------------------------------------------------------------------------
    constexpr U32 channelCount = 2;
    const U32 samplesPerRecord = preTriggerSamples + postTriggerSamples;
    const size_t bytesPerBuffer =
        static_cast<size_t>(samplesPerRecord) * bytesPerSample *
        recordsPerBuffer * channelCount;

    std::printf("Buffers: count=%u, size=%zu bytes each\n",
                dmaBufferCount, bytesPerBuffer);

    std::vector<U16*> buffers(dmaBufferCount, nullptr);
    for (U32 i = 0; i < dmaBufferCount; ++i) {
#ifdef _WIN32
        buffers[i] = static_cast<U16*>(
            VirtualAlloc(nullptr, bytesPerBuffer,
                         MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
#else
        buffers[i] = static_cast<U16*>(std::aligned_alloc(4096, bytesPerBuffer));
#endif
        if (!buffers[i]) {
            std::fprintf(stderr, "Buffer %u allocation failed.\n", i);
            return EXIT_FAILURE;
        }
    }

    // -------------------------------------------------------------------------
    // Arm AutoDMA NPT acquisition.
    // -------------------------------------------------------------------------
    const U32 channelMask = CHANNEL_A | CHANNEL_B;
    const U32 recordsPerAcquire = (buffersPerAcquire == 0)
        ? 0x7FFFFFFF
        : buffersPerAcquire * recordsPerBuffer;

    ATS_CHECK(AlazarBeforeAsyncRead(board, channelMask,
        -static_cast<long>(preTriggerSamples),
        samplesPerRecord,
        recordsPerBuffer,
        recordsPerAcquire,
        ADMA_EXTERNAL_STARTCAPTURE | ADMA_NPT));

    for (U32 i = 0; i < dmaBufferCount; ++i) {
        ATS_CHECK(AlazarPostAsyncBuffer(board, buffers[i], bytesPerBuffer));
    }

    std::FILE* out = std::fopen(outputPath, "wb");
    if (!out) {
        std::fprintf(stderr, "Cannot open %s for writing.\n", outputPath);
        AlazarAbortAsyncRead(board);
        return EXIT_FAILURE;
    }

    std::printf("Starting capture... (target = %u buffers)\n", buffersPerAcquire);
    auto t0 = std::chrono::steady_clock::now();
    ATS_CHECK(AlazarStartCapture(board));

    // -------------------------------------------------------------------------
    // Buffer loop.
    // -------------------------------------------------------------------------
    U32 buffersDone = 0;
    bool success = true;
    while (buffersDone < buffersPerAcquire) {
        U16* buf = buffers[buffersDone % dmaBufferCount];
        RETURN_CODE rc = AlazarWaitAsyncBufferComplete(board, buf, 5000);
        if (rc != ApiSuccess) {
            std::fprintf(stderr,
                "AlazarWaitAsyncBufferComplete failed after %u buffers: %d (%s)\n",
                buffersDone, static_cast<int>(rc), AlazarErrorToText(rc));
            success = false;
            break;
        }

        if (std::fwrite(buf, 1, bytesPerBuffer, out) != bytesPerBuffer) {
            std::fprintf(stderr, "fwrite failed at buffer %u\n", buffersDone);
            success = false;
            break;
        }

        ATS_CHECK(AlazarPostAsyncBuffer(board, buf, bytesPerBuffer));
        ++buffersDone;
        if (buffersDone % 10 == 0 || buffersDone == buffersPerAcquire) {
            std::printf("  %u / %u buffers\n", buffersDone, buffersPerAcquire);
        }
    }

    auto t1 = std::chrono::steady_clock::now();
    double elapsedSec =
        std::chrono::duration<double>(t1 - t0).count();

    ATS_CHECK(AlazarAbortAsyncRead(board));
    std::fclose(out);

    // -------------------------------------------------------------------------
    // Cleanup + summary.
    // -------------------------------------------------------------------------
    for (U32 i = 0; i < dmaBufferCount; ++i) {
#ifdef _WIN32
        if (buffers[i]) VirtualFree(buffers[i], 0, MEM_RELEASE);
#else
        std::free(buffers[i]);
#endif
    }

    const double totalBytes =
        static_cast<double>(buffersDone) * bytesPerBuffer;
    std::printf("\nCaptured %u buffers (%.2f MB) in %.3f s -> %.2f MB/s\n",
                buffersDone, totalBytes / (1024.0 * 1024.0),
                elapsedSec,
                (totalBytes / (1024.0 * 1024.0)) / (elapsedSec > 0 ? elapsedSec : 1));
    std::printf("Raw samples written to: %s\n", outputPath);
    std::printf("Format: interleaved by record. For each record,\n"
                "  %u samples of CH A, then %u samples of CH B,\n"
                "  each sample is a uint16 (12-bit MSB-aligned).\n",
                samplesPerRecord, samplesPerRecord);

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
