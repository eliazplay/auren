# AUREN

AUREN is a drum extraction, sampling and performance instrument for Windows.
The interface is drawn entirely in-house: JUCE provides the audio, MIDI,
windowing and plug-in host integration, while every visible control uses the
custom AUREN rack design.

## Build requirements

- CMake 3.24 or newer
- A C++20 compiler
- Git (CMake downloads the pinned JUCE source during configuration)
- Windows 10/11 with Visual Studio 2022 Build Tools for the production VST3

JUCE is fetched at the exact commit used by its 8.0.15 release. Review the
[JUCE licence](https://juce.com/legal/) before distributing a commercial build.

## Build on Windows

From PowerShell with Visual Studio 2022 installed:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --target AUREN_VST3 AUREN_Standalone --parallel 2
```

The products are written beneath `build/AUREN_artefacts/Release/`:

- `VST3/AUREN.vst3`
- `Standalone/AUREN.exe`

Set `-DAUREN_BUILD_STANDALONE=OFF` when only the VST3 target is required.

## GitHub build

The `Build Windows VST3` workflow runs for pushes to `main`, pull requests,
version tags and manual dispatches. It configures a clean Windows 2022 runner,
builds both formats and publishes `AUREN.vst3` and `AUREN.exe` as a workflow
artifact. No SDK or JUCE checkout needs to be committed to this repository.

