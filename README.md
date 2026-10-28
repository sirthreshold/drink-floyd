# Drink Floyd

> **Phase 0 — Windows Standalone Audio-Reactive Visualizer**
> Built with C++20, JUCE 8, and CMake.

**Drink Floyd** is an original standalone audio-reactive desktop music visualizer for Windows. Phase 0 establishes the native standalone application scaffold, core window layout, FPS readout header, dark parameter control panel, and full architectural specifications.

---

## Architecture Overview (Phase 0)
- **Target**: Windows Standalone Application ONLY (`DrinkFloyd.exe`).
- **No DAW Plugins**: Completely free of VST3, AU, AAX, plugin wrappers, or host automation code.
- **Framework**: JUCE 8 (`juce_gui_extra`, `juce_gui_basics`, `juce_core`, `juce_graphics`, `juce_data_structures`, `juce_events`).
- **Standard**: C++20.
- **Build System**: CMake 3.22+ targeting Visual Studio 2022 (MSVC) or Ninja/GCC.

---

## Layout
The main window consists of:
1. **Header Bar**:
   - Application branding: **Drink Floyd**.
   - Live **FPS readout** (real-time delta timer).
   - Mode indicator: **Windows Standalone | Phase 0**.
   - Output resolution profile selectors (`1080x1080`, `1080x1920`, `1920x1080`).
2. **Left Visual Canvas**:
   - Expansive dark viewport reserved for real-time graphics rendering.
   - Dynamic aspect-ratio letterboxing.
3. **Right Parameter Panel**:
   - Dark parameter sidebar containing controls for all 16 canonical visualizer parameters:
     - *react*, *chaos*, *input gain*
     - *trails*, *heat*, *decay*
     - *brightness*, *hue drift*, *palette*, *vignette*
     - *symmetry*, *tiles*, *twist*, *slice*, *lens*, *lens style*

---

## Windows Build Steps

### Option A: Building with Visual Studio 2022 (Recommended for VS users)

#### Prerequisites
- **Visual Studio 2022** (Community, Professional, or Enterprise)
  - Ensure the **"Desktop development with C++"** workload is installed.
  - Includes MSVC v143 (x64) and Windows 10/11 SDK.
- **CMake** 3.22 or higher.
- **Git**.

#### Build Instructions
Open **Developer PowerShell for VS 2022** or standard terminal:

1. **Clone the repository**:
   ```powershell
   git clone <repo-url> drink-floyd
   cd drink-floyd
   ```

2. **Generate Visual Studio 2022 Solution**:
   ```powershell
   cmake -B build -G "Visual Studio 17 2022" -A x64
   ```

3. **Build the Standalone Executable**:
   ```powershell
   # Build Release configuration
   cmake --build build --config Release
   ```
   *(Or open `build\DrinkFloyd.sln` inside Visual Studio 2022 and press `F5` / Build)*.

4. **Launch the Application**:
   ```powershell
   .\build\DrinkFloyd_artefacts\Release\Drink Floyd.exe
   ```

---

### Option B: Building via Command Line (Ninja / MinGW / Clang)

If compiling via command-line tools:

1. **Configure with Ninja**:
   ```powershell
   cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release
   ```

2. **Compile**:
   ```powershell
   cmake --build build
   ```

3. **Run**:
   ```powershell
   .\build\DrinkFloyd_artefacts\Release\Drink Floyd.exe
   ```

---

## Project Structure
```text
drink-floyd/
├── CMakeLists.txt         # Root JUCE CMake configuration (produces ONLY standalone app)
├── .gitignore             # Git ignore rules for C++/Visual Studio/CMake
├── SPEC.md                # Full technical specification & canonical parameter list
├── README.md              # Project documentation and build instructions
└── Source/
    ├── Main.cpp           # Native JUCEApplication entry point & DocumentWindow
    ├── MainComponent.h    # Main container component header
    ├── MainComponent.cpp  # Main container layout implementation
    ├── HeaderBar.h        # Header bar component with live FPS readout
    ├── HeaderBar.cpp      # Header bar paint & timer updates
    ├── VisualCanvas.h     # Large visual canvas viewport header
    ├── VisualCanvas.cpp   # Canvas paint & aspect ratio preservation
    ├── ParameterPanel.h   # Dark right sidebar header
    └── ParameterPanel.cpp # Parameter controls and styling
```

---

## Next Steps (Subsequent Phases)
- **Phase 1**: WASAPI loopback audio capture integration.
- **Phase 2**: Real-time GL/DirectX shader pipeline & parameter modulation.
- **Phase 3**: Offline video export engine (1080x1080, 1080x1920, 1920x1080 @ 30/60 fps).
