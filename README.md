# ATS9351 SDK Test (Win32)

Two small C++ programs that exercise an AlazarTech **ATS9351** digitizer through
the official ATS-SDK, configured to build as **Win32 (32-bit x86)** so the code
can eventually be merged into an existing Win32-only Visual Studio project.

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

---

## 1. Prerequisites on the workstation

Tick each item before moving on:

1. **ATS9351 driver** installed.
   Open *Device Manager* → expand **AlazarTech Boards** → you should see
   "ATS9351" with no yellow triangle. If not, reinstall the driver from
   https://www.alazartech.com/ and reboot.

2. **ATS-SDK** installed. Open a PowerShell and run:
   ```powershell
   dir C:\AlazarTech\ATS-SDK
   ```
   You should see one or more version folders (e.g. `26.1.0`). Then verify the
   header and the **Win32** library exist — the exact layout depends on the
   SDK version. SDK 26.x puts everything under a `Samples_C\` subfolder:

   ```powershell
   # SDK 26.x layout (new)
   dir C:\AlazarTech\ATS-SDK\*\Samples_C\Include\AlazarApi.h
   dir C:\AlazarTech\ATS-SDK\*\Samples_C\Library\Win32\ATSApi.lib
   ```

   Older SDK layouts (<= 7.x) put them one level up:

   ```powershell
   dir C:\AlazarTech\ATS-SDK\*\Include\AlazarApi.h
   dir C:\AlazarTech\ATS-SDK\*\Library\Win32\ATSApi.lib
   ```

   At least one of the two layouts must yield results. The `CMakeLists.txt`
   auto-detects both.

   **About `ATSApi.dll`:** the driver installer puts this into the Windows
   system directories, not the SDK tree. For a Win32 (32-bit) exe on a 64-bit
   Windows 10, the DLL is here:

   ```powershell
   dir C:\Windows\SysWOW64\ATSApi.dll
   ```

   (On 32-bit Windows it would be `C:\Windows\System32\ATSApi.dll`. The
   "SysWOW64" name is historical — on 64-bit Windows it's the folder for
   32-bit DLLs.) If that file exists, you're set; the exes will find the DLL
   through the standard Windows DLL search path at runtime and CMake will
   also copy it next to the build output as a convenience.

3. **Visual Studio 2022** with the **"Desktop development with C++"** workload.
   This installs:
   - MSVC v143 toolset (both x86 and x64 targets),
   - Windows 10/11 SDK,
   - CMake + Ninja (bundled — you do not need a separate CMake install).

4. **Git** (either the standalone Git for Windows, or the one bundled with VS).

---

## 2. Get the code onto the workstation

On **this laptop** (Mac), from the project directory, initialise a git repo and
push it to a remote you can reach from the workstation:

```bash
cd /Users/xulouzhe/Documents/Projects/ATS-SDK-Test
git init
git add .
git commit -m "Initial ATS9351 SDK test (Win32 build)"
git remote add origin <your-remote-url>
git push -u origin main
```

On the **workstation** (PowerShell):

```powershell
mkdir C:\work -Force
cd C:\work
git clone <your-remote-url> ATS-SDK-Test
cd ATS-SDK-Test
```

---

## 3. Build — Win32 (32-bit)

The repo ships with a CMake preset named **`vs2022-Win32`** that generates a
32-bit build. Pick one of the two paths below.

### Path A — "Open Folder" in VS 2022 (recommended)

1. Launch **Visual Studio 2022**.
2. *File → Open → Folder…* and pick `C:\work\ATS-SDK-Test`.
3. VS detects `CMakeLists.txt` + `CMakePresets.json`. In the top toolbar you'll
   see three dropdowns:
   - **Configuration**: set to `Visual Studio 2022 — Win32 (x86, default)`.
   - **Build preset**: set to `vs2022-Win32-release` (or `-debug`).
   - **Startup item**: set to `board_info.exe` or `npt_acquire.exe`.
4. Wait until the *Output* pane shows `CMake generation finished`. Expected
   lines for an SDK 26.x install look like:
   ```
   -- ATS-SDK root:        C:/AlazarTech/ATS-SDK/26.1.0
   -- ATS-SDK layout:      Samples_C
   -- ATS include folder:  C:/AlazarTech/ATS-SDK/26.1.0/Samples_C/Include
   -- Target architecture: Win32 (32-bit)
   -- ATS library folder:  C:/AlazarTech/ATS-SDK/26.1.0/Samples_C/Library/Win32
   -- ATSApi.lib resolved: C:/AlazarTech/ATS-SDK/26.1.0/Samples_C/Library/Win32/ATSApi.lib
   -- ATSApi.dll source:   C:/Windows/SysWOW64/ATSApi.dll (copied next to each exe)
   ```
   (For an older SDK the "layout" line will read `classic` and the paths won't
   have the `Samples_C/` segment.) If you don't see these, the SDK wasn't
   auto-detected — skip to
   "[If auto-detection fails](#if-cmake-cannot-find-the-ats-sdk)" below.
5. Build: **Build → Build All** (F7).
6. Run: **Debug → Start Without Debugging** (Ctrl+F5).

### Path B — generate a `.sln` and use VS the classic way

From a PowerShell in the repo root:

```powershell
cd C:\work\ATS-SDK-Test
cmake --preset vs2022-Win32
```

This creates `build\ATS9351Test.sln`. Double-click it to open VS 2022, set
**Release | Win32** in the toolbar, then **Build → Build Solution**
(Ctrl+Shift+B). Executables land in `build\Release\`.

To build purely from the command line (no GUI clicks):

```powershell
cmake --preset vs2022-Win32
cmake --build --preset vs2022-Win32-release
```

### If CMake cannot find the ATS-SDK

Either edit `CMakePresets.json` and switch to the
`vs2022-Win32-sdk-override` preset (pointing `ATS_SDK_ROOT` at your actual
version), **or** pass it on the command line:

```powershell
cmake --preset vs2022-Win32 -DATS_SDK_ROOT="C:/AlazarTech/ATS-SDK/26.1.0"
```

Use forward slashes. Replace `26.1.0` with whatever you see under
`C:\AlazarTech\ATS-SDK\`. Point at the version folder itself — CMake figures
out whether the layout uses `Samples_C\` or not.

### Runtime DLL

`ATSApi.dll` is installed into `C:\Windows\SysWOW64\` (32-bit DLL) and
`C:\Windows\System32\` (64-bit DLL) by the ATS driver installer, so the exe
can find it via the standard Windows DLL search path regardless of where you
run it from. For convenience CMake also copies the system DLL next to each
build output — so `build\Release\board_info.exe` works standalone too.

---

## 4. Run the tests

Open a PowerShell in `C:\work\ATS-SDK-Test` after the build succeeds.

### 4.1  First test — `board_info`

```powershell
.\build\Release\board_info.exe
```

**Expected output** (values will vary):

```
SDK version:    7.4.0
Driver version: 7.4.0
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

If this prints an error, **stop** and send me the exact output before running
`npt_acquire`. Common failure modes are listed in the Troubleshooting section.

### 4.2  Second test — `npt_acquire`

**Before running:** connect a signal generator to **Channel A**. The default
trigger is set to channel A rising through roughly +15% of full-scale with an
infinite timeout, so without a signal the program will block forever. A small
sine (e.g. 1 MHz, 200 mVpp) is perfect.

If you just want a quick liveness check with no external signal, edit
`src/npt_acquire.cpp` line:

```cpp
constexpr U32 triggerTimeoutMs = 0;   // 0 = wait forever
```

…change `0` to `1000` (1 second timeout per buffer). The acquisition will
record whatever noise is on the input and still exercise the DMA path.

Then run:

```powershell
.\build\Release\npt_acquire.exe
```

**Expected output:**

```
Board ready: bits/sample=12, bytes/sample=2, max samples/ch=...
Buffers: count=4, size=40960 bytes each
Starting capture... (target = 10 buffers)
  10 / 10 buffers

Captured 10 buffers (0.39 MB) in 0.00X s -> ... MB/s
Raw samples written to: ats9351_capture.bin
Format: interleaved by record. For each record,
  1024 samples of CH A, then 1024 samples of CH B,
  each sample is a uint16 (12-bit MSB-aligned).
```

`ats9351_capture.bin` will be written in whichever directory you ran the exe
from (usually `C:\work\ATS-SDK-Test`).

### 4.3  Interpreting the binary output

For each of the 100 records (10 buffers × 10 records/buffer):

```
[ 1024 × uint16 CH A ][ 1024 × uint16 CH B ]
```

Each sample is a 12-bit code **left-justified** into a 16-bit word. Convert to
volts in a quick Python snippet on the workstation (or back on this laptop):

```python
import numpy as np
raw = np.fromfile("ats9351_capture.bin", dtype="<u2")  # little-endian uint16
raw = raw >> 4                                          # 0..4095
fs_volts = 0.4                                          # ±400 mV range
volts = (raw.astype("float32") - 2048.0) / 2048.0 * fs_volts

# Reshape into (records, 2 channels, samples)
n = volts.size // (1024 * 2)
x = volts.reshape(n, 2, 1024)
chA = x[:, 0, :]
chB = x[:, 1, :]
```

---

## 5. Merging into your existing Win32 VS project

Once `npt_acquire` works standalone, pulling the capture code into your legacy
project is mechanical:

1. **Copy the sources** into the legacy project tree:
   - `include/ATSHelpers.h`
   - whichever of `src/board_info.cpp` / `src/npt_acquire.cpp` you want to
     reuse — or extract the acquisition code into a function with a clean
     signature (e.g. `bool CaptureNptToFile(const Config& cfg);`) and call
     that from your existing code.
2. **Right-click the project → Properties**. Set **Configuration: All
   Configurations**, **Platform: Win32**. Then (paths shown for SDK 26.x —
   drop the `Samples_C\` segment if you're on an older SDK):
   - *C/C++ → General → Additional Include Directories*: add
     `C:\AlazarTech\ATS-SDK\26.1.0\Samples_C\Include`
   - *C/C++ → Language → C++ Language Standard*: **ISO C++17** (or newer). The
     helper code uses `<chrono>`, `std::vector`, and `constexpr`.
   - *Linker → General → Additional Library Directories*: add
     `C:\AlazarTech\ATS-SDK\26.1.0\Samples_C\Library\Win32`
   - *Linker → Input → Additional Dependencies*: add `ATSApi.lib`
3. **No DLL deployment needed.** `ATSApi.dll` is already in
   `C:\Windows\SysWOW64\` from the driver install, so a 32-bit exe just finds
   it. (If you want to be explicit, add a post-build event:
   `copy /Y C:\Windows\SysWOW64\ATSApi.dll "$(OutDir)"` — but it's optional.)
4. **Build.** If the link fails with "unresolved external symbol Alazar…",
   re-check that the Platform dropdown is **Win32** (not x64) — the Win32
   `.lib` only exports the 32-bit symbols.
5. If your legacy project still uses an older C++ standard, ship a thin C++17
   compilation unit (just the ATS code) and expose a plain C-style function
   signature that the rest of the legacy code can call through `extern "C"`.

Tip: once the standalone test passes, copy the *exact* compile/link flags from
`build\ATS9351Test.vcxproj` into the legacy project to avoid missing a setting.

---

## 6. Troubleshooting

| Symptom | Likely cause / fix |
| ------- | ------------------ |
| `board_info` prints `Systems: 0`. | Driver not loaded. Reinstall the ATS driver, reboot, verify in Device Manager. |
| CMake error: `Could not locate the ATS-SDK`. | Pass `-DATS_SDK_ROOT=...` as shown above, or use the `vs2022-Win32-sdk-override` preset. |
| Link error `unresolved external symbol Alazar...`. | Platform mismatch — you're linking the x64 `.lib` into a Win32 exe or vice versa. Check the build actually used `Library\Win32\ATSApi.lib` in the CMake Output pane. |
| `ATSApi.dll` not found at launch. | The driver install usually places it in `C:\Windows\SysWOW64\ATSApi.dll` (Win32 exe) or `C:\Windows\System32\ATSApi.dll` (x64 exe). If missing, reinstall the ATS driver. As a quick workaround you can `copy` it from either system dir next to the `.exe`. |
| `AlazarWaitAsyncBufferComplete` times out or returns a non-zero code. | No trigger arrived. Feed a signal into CH A, set `triggerTimeoutMs` to a nonzero value, or lower `triggerLevelCode`. |
| Captured samples look like full-scale noise. | Remember the 12-bit samples are MSB-aligned in uint16 — shift right by 4 before plotting. |
| VS 2022 shows "CMake presets not found". | You opened a subfolder. Reopen `C:\work\ATS-SDK-Test` itself so the preset file is visible. |

---

## 7. Feedback loop

Run `board_info` first. If that succeeds, run `npt_acquire`. Please send:

1. The full console output of whichever program fails (ATS error codes are
   numeric — the code also prints the decoded name).
2. Your installed SDK version (the folder name under `C:\AlazarTech\ATS-SDK\`).
3. The `-- ATSApi.lib resolved:` line from the VS CMake Output pane so I can
   confirm the Win32 lib was actually picked.

I'll iterate on the code here and you re-pull.
