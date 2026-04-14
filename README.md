# ATS9351 SDK Test

Two small C++ programs that exercise an AlazarTech **ATS9351** digitizer through
the official ATS-SDK:

| Program        | What it does                                                |
| -------------- | ----------------------------------------------------------- |
| `board_info`   | Enumerates ATS systems, opens the board, prints model/serial/version info. Run this first. |
| `npt_acquire`  | AutoDMA "No-Pre-Trigger" capture, both channels at 500 MS/s. Writes raw 12-bit samples (MSB-aligned in uint16) to `ats9351_capture.bin`. |

The acquisition code follows the pattern from the `SampleProgram/NPT` example
that ships with the ATS-SDK, distilled into a single file you can edit from the
top.

## Layout

```
ATS-SDK-Test/
├── CMakeLists.txt
├── CMakePresets.json       # VS 2022 picks this up automatically
├── include/
│   └── ATSHelpers.h        # ATS_CHECK macro + sample-rate table
├── src/
│   ├── board_info.cpp      # Step 1: confirm the board is visible
│   └── npt_acquire.cpp     # Step 2: AutoDMA NPT capture
└── README.md
```

## Prerequisites on the Windows 10 workstation

1. **ATS9351 driver** installed — the board should appear under
   *Device Manager → AlazarTech Boards* without a yellow triangle.
2. **ATS-SDK** installed, typically at
   `C:\AlazarTech\ATS-SDK\<version>\` (contains `Include\` and `Library\`).
3. **Visual Studio 2022** with the "Desktop development with C++" workload
   (this pulls in MSVC, the Windows SDK, and the bundled CMake + Ninja — no
   separate CMake install needed).
4. **Git** to pull this repo.

## Getting the code onto the workstation

On this laptop, from the project directory:

```powershell
git init
git add .
git commit -m "Initial ATS9351 SDK test"
# push to a remote you can reach from the workstation, e.g. GitHub / GitLab /
# a USB drive / a file share.
```

On the workstation:

```powershell
git clone <your-remote> C:\work\ATS-SDK-Test
cd C:\work\ATS-SDK-Test
```

## Build with Visual Studio 2022

There's a `CMakePresets.json` in the repo, so VS 2022 picks up the project
automatically. Pick whichever of the two options below you prefer.

### Option A — "Open Folder" (native CMake in VS 2022, recommended)

1. Launch **Visual Studio 2022**.
2. *File → Open → Folder…* and select `C:\work\ATS-SDK-Test`.
3. VS will detect `CMakeLists.txt` + `CMakePresets.json`. In the top toolbar,
   set the configure preset to **"Visual Studio 2022 — x64"** and wait for the
   "CMake generation finished" message in the Output pane.
4. In the *Solution Explorer* switch to *CMake Targets View* (the dropdown at
   the top of the pane). You'll see `board_info` and `npt_acquire`.
5. Right-click a target → *Set as Startup Item*. Then **Build → Build All**
   (F7) and **Debug → Start Without Debugging** (Ctrl+F5).

If CMake can't auto-locate the SDK, choose the **"VS 2022 x64 — explicit SDK
path"** preset instead and edit the `ATS_SDK_ROOT` value in
`CMakePresets.json` to match your installed version (e.g.
`C:/AlazarTech/ATS-SDK/7.4.0`). Forward slashes — CMake doesn't like backslashes
in JSON.

### Option B — generate a `.sln` and open it

If you prefer a classic solution file:

```powershell
cd C:\work\ATS-SDK-Test
cmake --preset vs2022-x64
```

This creates `build\ATS9351Test.sln`. Double-click it to open in VS 2022,
pick **Release | x64** from the toolbar, and hit **Build → Build Solution**
(Ctrl+Shift+B). Executables land in `build\Release\`.

From the command line you can skip the GUI entirely:

```powershell
cmake --preset vs2022-x64
cmake --build --preset vs2022-x64-release
```

### If CMake can't find the ATS-SDK

The `CMakeLists.txt` globs `C:/AlazarTech/ATS-SDK/*` and picks the newest one
with a valid `Include/AlazarApi.h`. If yours lives somewhere else, either:

- edit the `ATS_SDK_ROOT` cache variable in the
  `vs2022-x64-sdk-override` preset, **or**
- pass it on the command line:
  `cmake --preset vs2022-x64 -DATS_SDK_ROOT="C:/path/to/ATS-SDK/7.4.0"`

### Runtime DLL

The build copies `ATSApi.dll` next to each `.exe` automatically, so you can run
from `build\Release\` (Option B) or `build\out\build\<preset>\` (Option A) with
no extra setup.

## Run

Output path depends on how you built:

- **Option A (Open Folder)**: `build\board_info.exe` and `build\npt_acquire.exe`
  (VS 2022 places them directly under the CMake binary dir it created).
- **Option B (.sln)**: `build\Release\board_info.exe` etc.

You can also press **Ctrl+F5** in VS to run the startup item directly.

### 1. Board info

```powershell
.\build\Release\board_info.exe     # Option B path
```

Expected output (values will vary):

```
SDK version:    x.y.z
Driver version: x.y.z
Systems:        1

System 1: 1 board(s)
  Board 1:
    Model:          ATS9351 (kind=...)
    Bits/sample:    12
    Max samples/ch: ...
    Serial number:  ...
    FPGA version:   x.y
    CPLD version:   x.y

board_info OK
```

If this fails, stop here — the code below won't work either. Typical causes:
driver not loaded, SDK headers from a different generation than the installed
driver, board not powered / not seated, or PCIe slot that doesn't supply enough
lanes.

### 2. NPT acquisition

```powershell
.\build\Release\npt_acquire.exe    # Option B path
```

By default this captures 10 buffers × 10 records × 1024 samples × 2 channels at
500 MS/s, triggering on channel A rising through roughly +15% of full-scale. The
trigger timeout is `0` (wait forever), so **feed a signal into CH A** or change
`triggerLevelCode`/`triggerTimeoutMs` at the top of `src/npt_acquire.cpp`.

A healthy run prints the throughput (MB/s) and writes `ats9351_capture.bin`.

### Data format of `ats9351_capture.bin`

For each record (there are `recordsPerBuffer × buffersPerAcquire` of them):

```
[ CH A sample 0 .. CH A sample N-1 ][ CH B sample 0 .. CH B sample N-1 ]
```

Each sample is a `uint16`, little-endian, 12 bits of data left-justified in
the upper bits (shift right by 4 to get a 0..4095 code). Convert to volts with

```
V = ((code - 2048) / 2048.0) * fullScaleVolts   // fullScaleVolts = 0.4 for ±400 mV
```

## Configuration knobs

Edit the top of `src/npt_acquire.cpp`:

| Constant               | Meaning                                             |
| ---------------------- | --------------------------------------------------- |
| `sampleRateId`         | `SAMPLE_RATE_*` from `AlazarApi.h`                  |
| `inputRange`           | `INPUT_RANGE_PM_*` — check ATS9351 manual for supported ranges |
| `postTriggerSamples`   | Samples per record (must be a multiple of 32 on ATS9351) |
| `recordsPerBuffer`     | Records per DMA buffer                              |
| `buffersPerAcquire`    | Total buffers to collect (0 = infinite, stop with Ctrl+C) |
| `dmaBufferCount`       | Number of DMA buffers kept in flight on the board   |
| `triggerLevelCode`     | 0..255; 128 ≈ 0 V                                   |
| `triggerTimeoutMs`     | 0 waits forever; set a value if you want to capture without a trigger source |

## Troubleshooting

- **`board_info` prints `Systems: 0`** — the driver is not loaded. Reinstall
  the ATS driver from the AlazarTech site and reboot.
- **CMake error: "Could not locate the ATS-SDK"** — pass `-DATS_SDK_ROOT=...`
  as shown above.
- **Link error `unresolved external symbol Alazar...`** — you're building the
  wrong architecture (32-bit exe against x64 lib or vice versa). Use
  `-A x64` on the CMake configure step.
- **`ATSApi.dll not found` at runtime** — either the post-build copy step
  didn't fire, or you're running from a different directory. Copy
  `C:\AlazarTech\ATS-SDK\<ver>\Library\x64\ATSApi.dll` next to the exe.
- **`AlazarWaitAsyncBufferComplete` times out or returns an error** — no
  trigger arrived. Feed a signal into CH A, or raise `triggerTimeoutMs`, or
  lower `triggerLevelCode`.
- **Garbage data** — remember the 12-bit samples are MSB-aligned in uint16.
  Shift right by 4 before plotting.

## Feedback loop

Run `board_info` first. If that succeeds, run `npt_acquire`. Send me the exact
console output (including any ATS error codes) of whichever one fails, plus
your installed SDK version (`C:\AlazarTech\ATS-SDK\<version>\`) and I'll iterate.
