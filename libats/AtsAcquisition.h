// AtsAcquisition.h — free-running NPT AutoDMA acquisition engine for ATS9351.
//
// Usage:
//     ats::AcquisitionEngine eng;
//     if (!eng.open()) { /* eng.lastError() */ }
//     eng.configure(ats::AcquisitionConfig::defaultForAts9351());
//     eng.start();
//     while (running) {
//         auto frame = eng.latestFrame();  // may be null if nothing new yet
//         if (frame) { ... render ... }
//     }
//     eng.stop();
//
// The engine runs its own worker thread. Consumers poll latestFrame() on
// whatever cadence suits them (GUI timer at 20-60 Hz, CLI tight loop, etc).
// Intermediate frames are overwritten between polls — the engine only keeps
// the newest and increments a dropped-frame counter for the rest.
#pragma once

#include "AtsTypes.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace ats {

class AcquisitionEngine {
public:
    AcquisitionEngine();
    ~AcquisitionEngine();

    AcquisitionEngine(const AcquisitionEngine&)            = delete;
    AcquisitionEngine& operator=(const AcquisitionEngine&) = delete;

    // Open the board. Returns false on failure; inspect lastError().
    bool open(unsigned systemId = 1, unsigned boardId = 1);

    // Apply configuration to the board. Must be called while stopped.
    bool configure(const AcquisitionConfig& cfg);

    // Launch the worker thread. The board must have been configure()d first.
    bool start();

    // Stop the worker, abort DMA, and free DMA buffers.
    void stop();

    bool isRunning() const { return m_running.load(); }

    // Non-blocking: returns the newest frame, or nullptr if no new frame has
    // arrived since the last call. Thread-safe.
    std::shared_ptr<const Frame> latestFrame();

    uint64_t framesProduced() const { return m_framesProduced.load(); }
    uint64_t framesDropped()  const { return m_framesDropped.load(); }

    const std::string& lastError() const { return m_lastError; }

    unsigned bitsPerSample()  const { return m_bitsPerSample; }

private:
    void workerLoop();
    bool armBoardLocked();        // called from start() with buffers allocated
    bool allocateBuffers();
    void releaseBuffers();
    void setError(const char* where, int atsCode);

    // Board handle (HANDLE from the SDK). Void* so this header doesn't pull
    // in AlazarApi.h.
    void* m_board = nullptr;

    AcquisitionConfig m_cfg;
    unsigned          m_bitsPerSample = 0;

    // DMA buffers allocated via AlazarAllocBufferU16.
    std::vector<uint16_t*> m_buffers;
    size_t                 m_bytesPerBuffer   = 0;
    unsigned               m_samplesPerRecord = 0;

    std::thread        m_worker;
    std::atomic<bool>  m_running{false};
    std::atomic<bool>  m_shouldStop{false};

    std::mutex                          m_frameMutex;
    std::shared_ptr<const Frame>        m_latestFrame;
    bool                                m_frameIsFresh = false;

    std::atomic<uint64_t> m_framesProduced{0};
    std::atomic<uint64_t> m_framesDropped{0};
    std::atomic<uint64_t> m_sequence{0};

    std::string m_lastError;
};

} // namespace ats
