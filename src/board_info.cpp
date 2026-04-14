// board_info.cpp
// -----------------------------------------------------------------------------
// First-contact test: enumerate ATS boards, print model, memory size, bits per
// sample, FPGA/driver/CPLD versions, and serial number. Run this BEFORE
// npt_acquire to confirm the driver + SDK + board are all wired up.
//
// Expected output on a healthy ATS9351:
//   Systems:              1
//   Boards in system 1:   1
//   Board 1:
//     Model:              ATS9351
//     Bits / sample:      12
//     Max samples/ch:     ...   (depends on on-board memory option)
//     Serial number:      ...
//     Driver version:     x.y.z
//     SDK version:        x.y.z
//     FPGA version:       x.y
//     CPLD version:       x.y
// -----------------------------------------------------------------------------

#include "ATSHelpers.h"

#include <cstdio>

static const char* boardKindToName(U32 kind) {
    // Only the families most likely to appear on this workstation are listed;
    // AlazarApi.h has the full enum if you need more.
    switch (kind) {
        case ATS850:  return "ATS850";
        case ATS860:  return "ATS860";
        case ATS310:  return "ATS310";
        case ATS330:  return "ATS330";
        case ATS460:  return "ATS460";
        case ATS660:  return "ATS660";
        case ATS9350: return "ATS9350";
        case ATS9351: return "ATS9351";
        case ATS9360: return "ATS9360";
        case ATS9370: return "ATS9370";
        case ATS9373: return "ATS9373";
        case ATS9440: return "ATS9440";
        case ATS9462: return "ATS9462";
        case ATS9625: return "ATS9625";
        case ATS9626: return "ATS9626";
        case ATS9870: return "ATS9870";
        default:      return "UNKNOWN";
    }
}

int main() {
    U32 sdkMajor = 0, sdkMinor = 0, sdkRev = 0;
    ATS_CHECK(AlazarGetSDKVersion(&sdkMajor, &sdkMinor, &sdkRev));

    U32 drvMajor = 0, drvMinor = 0, drvRev = 0;
    ATS_CHECK(AlazarGetDriverVersion(&drvMajor, &drvMinor, &drvRev));

    std::printf("SDK version:    %u.%u.%u\n", sdkMajor, sdkMinor, sdkRev);
    std::printf("Driver version: %u.%u.%u\n", drvMajor, drvMinor, drvRev);

    U32 systemCount = AlazarNumOfSystems();
    std::printf("Systems:        %u\n", systemCount);
    if (systemCount == 0) {
        std::fprintf(stderr,
            "No ATS systems detected. Check that the ATS driver is installed "
            "and the board is visible in Device Manager.\n");
        return EXIT_FAILURE;
    }

    for (U32 sysId = 1; sysId <= systemCount; ++sysId) {
        U32 boardCount = AlazarBoardsInSystemBySystemID(sysId);
        std::printf("\nSystem %u: %u board(s)\n", sysId, boardCount);

        for (U32 boardId = 1; boardId <= boardCount; ++boardId) {
            HANDLE board = AlazarGetBoardBySystemID(sysId, boardId);
            if (board == nullptr) {
                std::fprintf(stderr, "  Failed to open board %u/%u\n",
                             sysId, boardId);
                continue;
            }

            U32 kind = AlazarGetBoardKind(board);

            U8  bitsPerSample = 0;
            U32 maxSamplesPerChannel = 0;
            ATS_CHECK(AlazarGetChannelInfo(board, &maxSamplesPerChannel,
                                           &bitsPerSample));

            U32 serial = 0;
            atsOk(AlazarQueryCapability(board, GET_SERIAL_NUMBER, 0, &serial),
                  "GET_SERIAL_NUMBER");

            U8 fpgaMajor = 0, fpgaMinor = 0;
            atsOk(AlazarGetFPGAVersion(board, &fpgaMajor, &fpgaMinor),
                  "AlazarGetFPGAVersion");

            U8 cpldMajor = 0, cpldMinor = 0;
            atsOk(AlazarGetCPLDVersion(board, &cpldMajor, &cpldMinor),
                  "AlazarGetCPLDVersion");

            std::printf("  Board %u:\n", boardId);
            std::printf("    Model:          %s (kind=%u)\n",
                        boardKindToName(kind), kind);
            std::printf("    Bits/sample:    %u\n", bitsPerSample);
            std::printf("    Max samples/ch: %u\n", maxSamplesPerChannel);
            std::printf("    Serial number:  %u\n", serial);
            std::printf("    FPGA version:   %u.%u\n", fpgaMajor, fpgaMinor);
            std::printf("    CPLD version:   %u.%u\n", cpldMajor, cpldMinor);
        }
    }

    std::printf("\nboard_info OK\n");
    return EXIT_SUCCESS;
}
