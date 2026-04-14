# CLAUDE.md — context for future sessions

Quick-load context so a fresh Claude session can work on this repo without
re-deriving everything.

## Project purpose

Exercise an **AlazarTech ATS9351** PCIe digitizer through the official ATS-SDK,
as a standalone test, with the goal of **eventually merging the acquisition
code into an existing Win32-only Visual Studio project**. That legacy-project
merge is the whole reason this is a Win32 build, not x64.

Two programs:
- `src/board_info.cpp` — enumerates ATS systems, prints board/SDK/driver/FPGA
  info. First thing to run to prove the driver + SDK + board are wired up.
- `src/npt_acquire.cpp` — AutoDMA "No-Pre-Trigger" capture, both channels at
  500 MS/s, writes raw samples to `ats9351_capture.bin`.

`include/ATSHelpers.h` has `ATS_CHECK` (abort on non-`ApiSuccess`) and a
sample-rate-constant → Hz lookup.

## Environment constraints

- **Developer laptop** (this machine, macOS): edits code, commits, pushes.
  Cannot build or run — no ATS hardware, no Windows.
- **Workstation** (user's Windows 10 box, not this laptop): has the card, the
  driver, the SDK, Visual Studio 2022. User pulls, builds, runs, pastes
  output back. Workflow is always: edit here → commit → push → user pulls on
  workstation → user reports results → iterate.
- **Target**: Win32 (x86) only. Do NOT default to x64 — it would compile fine
  but breaks the merge plan for the legacy project. The `vs2022-Win32`
  preset is the default; the `vs2022-x64` preset is kept only for reference.

## ATS-SDK install on the workstation (SDK 26.1.0)

```
C:\AlazarTech\ATS-SDK\26.1.0\Samples_C\Include\AlazarApi.h   (+ siblings)
C:\AlazarTech\ATS-SDK\26.1.0\Samples_C\Library\Win32\ATSApi.lib
C:\AlazarTech\ATS-SDK\26.1.0\Samples_C\Library\x64\ATSApi.lib
```

Note the **`Samples_C\`** segment — SDK 26.x moved everything one level
deeper than older SDKs. `CMakeLists.txt` auto-detects both layouts via the
`_ats_find_inner_dir` function.

**`ATSApi.dll` is NOT in the SDK tree.** The driver installer puts it in the
Windows system folder:

- `C:\Windows\SysWOW64\ATSApi.dll`  (32-bit DLL — used by Win32 exes)
- `C:\Windows\System32\ATSApi.dll`  (64-bit DLL — used by x64 exes)

On 64-bit Windows the names are counter-intuitive but correct: `SysWOW64`
holds 32-bit DLLs, `System32` holds 64-bit DLLs. CMake's post-build copy
falls through to the appropriate system path when the SDK tree has no DLL
(SDK 26.x case).

## Reference samples (gitignored)

The developer drops a copy of `C:\AlazarTech\ATS-SDK\26.1.0\Samples_C\` into
the repo root as `Samples_C/` for Claude to read. **Never commit it** —
it's ~64 MB of vendor code, already in `.gitignore`.

If you need to see how AlazarTech writes something, the most relevant
reference is:

```
Samples_C/ATS9351/DualPort/NPT/          # NPT AutoDMA (matches our npt_acquire.cpp)
Samples_C/ATS9351/DualPort/CS/           # continuous streaming
Samples_C/ATS9351/DualPort/TR/           # traditional record (single capture)
Samples_C/Include/AlazarApi.h            # exact function signatures
Samples_C/Include/AlazarCmd.h            # constant enums (SAMPLE_RATE_*, INPUT_RANGE_*, ...)
Samples_C/Include/AlazarError.h          # RETURN_CODE values
```

Check the exact signature in `AlazarApi.h` before writing a new call — types
occasionally differ between SDK generations (see "Known gotchas" below).

## Build & run

Primary path (VS 2022 Open Folder → pick `vs2022-Win32` preset → F7). From
command line on the workstation:

```powershell
cd C:\work\ATS-SDK-Test
cmake --preset vs2022-Win32
cmake --build --preset vs2022-Win32-release
.\build\Release\board_info.exe
.\build\Release\npt_acquire.exe        # needs signal on CH A
```

Expected CMake Output pane lines (paste these back if build breaks):

```
-- ATS-SDK root:        C:/AlazarTech/ATS-SDK/26.1.0
-- ATS-SDK layout:      Samples_C
-- Target architecture: Win32 (32-bit)
-- ATS library folder:  C:/AlazarTech/ATS-SDK/26.1.0/Samples_C/Library/Win32
-- ATSApi.lib resolved: .../Library/Win32/ATSApi.lib
-- ATSApi.dll source:   C:/Windows/SysWOW64/ATSApi.dll (copied next to each exe)
```

## Known gotchas

- **`AlazarGetSDKVersion` / `AlazarGetDriverVersion` take `U8*`, not `U32*`.**
  Writing `U32` locals triggers MSVC C2664. Fixed in commit `90fc0ff`. If a
  similar "cannot convert U32* to U8*" error shows up for any other call,
  check `Samples_C/Include/AlazarApi.h` for the real signature and switch
  the local to `U8`.
- **Sample encoding is 12-bit MSB-aligned in `uint16`.** To get a 0..4095
  code, right-shift by 4 before converting to volts. Forgetting this is the
  top cause of "my data looks like random full-scale noise".
- **`postTriggerSamples` must be a multiple of 32 on the ATS9351.** Default
  in `npt_acquire.cpp` is 1024 (safe). If changing, keep it aligned.
- **Default trigger is CH A rising + infinite timeout.** Without a signal
  on CH A the acquire loop blocks forever. For a quick no-signal smoke test,
  bump `triggerTimeoutMs` from 0 to `1000` at the top of `npt_acquire.cpp`.
- **Win32 build uses `VirtualAlloc` for DMA buffers**, so `npt_acquire.cpp`
  explicitly `#include <windows.h>` (wrapped in `#ifdef _WIN32`).

## Collaboration rules (from past corrections)

- **Never push without explicit permission.** User approved the first two
  pushes individually. Assume each push needs a fresh OK unless the user
  says otherwise for a given stretch of work.
- The three PDFs at repo root (`ATS9351 Datasheet.pdf`,
  `ATS9351 User Manual_V1_8.pdf`, `ATSSDKGuide2610.pdf`) are committed
  intentionally. Don't add them to `.gitignore`. Reading them from this
  laptop fails (no `pdftotext` / `poppler` installed) — use the
  `Samples_C/` code and `Include/` headers as the authoritative reference
  instead.

## What to do when the user reports a build/run failure

1. Ask for (or look for) the exact MSVC error text or ATS runtime error
   code — both include file/line and are copy-pasteable.
2. Cross-check the relevant signature / constant in
   `Samples_C/Include/AlazarApi.h` or `AlazarCmd.h` rather than guessing.
3. If unsure what the AlazarTech-idiomatic way is, grep
   `Samples_C/ATS9351/DualPort/NPT/` for the same call — that sample is the
   closest to our code.
4. Make a minimal fix, commit, push only with explicit "granted" / "go
   ahead" from the user.
