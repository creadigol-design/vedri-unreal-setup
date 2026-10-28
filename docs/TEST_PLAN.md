# Test plan

Step-by-step checks that someone who didn't write the code can follow. Each milestone adds a section.
The PC sections cover which menu to use, what to click, what should happen, and what to check on the
SDI monitor and the StarTracker.

## Core layer: standalone tests (Mac or any machine with a C++ compiler)

Needs the Xcode command line tools on the Mac (`xcode-select --install`), or clang/gcc on Linux, plus `python3`.

1. Open a terminal in the repo root.
2. Run `make -C Tests/Standalone`.
   - **Expect:** a list of `[ ok ]` lines, then `56 tests, 0 failed checks`, then
     `validation codes: 43 emitted, all documented`. The exit status is 0.
3. Optional, where available: `make -C Tests/Standalone all-compilers` (clang and gcc) and
   `make -C Tests/Standalone sanitize`.
   - **Expect:** the same result from each.
   - On some Linux images clang has no sanitizer runtime. Use `make -C Tests/Standalone sanitize CXX=g++` there.
4. Bundled presets: `make -C Tests/Standalone presets`, then `git status`.
   - **Expect:** no changes under `VedriVPToolkit/Resources/Presets/`. The shipped files must equal what the serialiser writes.

If anything fails, the failing line is printed as `File.cpp:LINE: FAILED [TestName] ...`.

## PC (M7)

The PC sections are added as M1–M6 land.
