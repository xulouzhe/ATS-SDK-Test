// npt_acquire.cpp
// -----------------------------------------------------------------------------
// CLI smoke test for libats::AcquisitionEngine. Captures N frames of NPT
// AutoDMA data from the ATS9351 at 500 MS/s and writes them to disk in the
// same interleaved format AlazarTech's sample uses.
//
// This program is deliberately thin now — all the acquisition logic lives
// in libats. See scope/ for the MFC GUI version.
// -----------------------------------------------------------------------------

#include "AtsAcquisition.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <thread>

int main() {
    // ---- configuration ------------------------------------------------------
    auto cfg = ats::AcquisitionConfig::defaultForAts9351();
    cfg.postTriggerSamples = 1024;
    cfg.recordsPerBuffer   = 10;
    cfg.dmaBufferCount     = 4;
    cfg.triggerLevelCode   = 150;   // ~+17% FS, CH A rising
    cfg.triggerTimeoutMs   = 0;     // wait forever for a trigger

    constexpr unsigned framesToCapture = 10;
    const char* outputPath = "ats9351_capture.bin";

    // ---- open + configure ---------------------------------------------------
    ats::AcquisitionEngine eng;
    if (!eng.open()) {
        std::fprintf(stderr, "open failed: %s\n", eng.lastError().c_str());
        return EXIT_FAILURE;
    }
    std::printf("Board open: %u-bit samples\n", eng.bitsPerSample());

    if (!eng.configure(cfg)) {
        std::fprintf(stderr, "configure failed: %s\n", eng.lastError().c_str());
        return EXIT_FAILURE;
    }

    if (!eng.start()) {
        std::fprintf(stderr, "start failed: %s\n", eng.lastError().c_str());
        return EXIT_FAILURE;
    }

    std::FILE* fp = std::fopen(outputPath, "wb");
    if (!fp) {
        std::fprintf(stderr, "fopen %s failed\n", outputPath);
        eng.stop();
        return EXIT_FAILURE;
    }

    auto t0 = std::chrono::steady_clock::now();
    unsigned saved = 0;
    uint64_t lastSeq = UINT64_MAX;
    while (saved < framesToCapture) {
        auto frame = eng.latestFrame();
        if (!frame) {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            continue;
        }
        if (frame->sequenceNumber == lastSeq) continue;
        lastSeq = frame->sequenceNumber;

        // Interleaved output: all of CH A for all records, then all of CH B.
        if (!frame->channelA.empty()) {
            std::fwrite(frame->channelA.data(), sizeof(uint16_t),
                        frame->channelA.size(), fp);
        }
        if (!frame->channelB.empty()) {
            std::fwrite(frame->channelB.data(), sizeof(uint16_t),
                        frame->channelB.size(), fp);
        }
        ++saved;
        std::printf("  frame %u / %u  (seq %llu)\n",
                    saved, framesToCapture,
                    static_cast<unsigned long long>(frame->sequenceNumber));
    }

    auto t1 = std::chrono::steady_clock::now();
    eng.stop();
    std::fclose(fp);

    const double elapsed = std::chrono::duration<double>(t1 - t0).count();
    std::printf("\nCaptured %u frames in %.3f s (produced=%llu, dropped=%llu)\n",
                saved, elapsed,
                static_cast<unsigned long long>(eng.framesProduced()),
                static_cast<unsigned long long>(eng.framesDropped()));
    std::printf("Raw samples in: %s\n", outputPath);
    return EXIT_SUCCESS;
}
