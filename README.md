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

## Phase 1: Image Import & OpenGL Viewport

Phase 1 adds native image import without blocking the UI thread:
- **Supported Formats**: JPG, PNG, WebP, BMP (decoded via native Windows Imaging Component / WIC with JUCE fallback).
- **OpenGL Canvas**: Uploads image as an OpenGL texture rendered to a viewport quad.
- **Aspect Ratio & Fit Mode**: Original aspect ratio preserved by default; toggleable between **Fit** (letterbox) and **Cover** (full bleed).
- **Auto-Downscale**: Images exceeding 4096×4096 are automatically downscaled proportionally to the max texture limit (4096px) with an indicator in the status line.
- **Status Bar**: Bottom bar displays image filename, dimensions, downscale notes, and errors.

### Manual Test Instructions (3 Import Paths)

1. **Path 1: File Dialog Open**
   - Click the **"Open Image..."** button in the header bar or press `Ctrl+O` (or click directly on the canvas placeholder).
   - Select any `.jpg`, `.png`, `.webp`, or `.bmp` image from disk.
   - Verify: The image is loaded asynchronously, uploaded as an OpenGL texture, rendered maintaining aspect ratio, and the file name + dimensions appear in the bottom status line.

2. **Path 2: Drag & Drop**
   - Drag an image file (`.png`, `.jpg`, `.webp`, `.bmp`) directly from Windows File Explorer onto the canvas.
   - Verify: Hovering displays a cyan highlight drop border. Dropping the file immediately loads and replaces the canvas texture.

3. **Path 3: Clipboard Paste (`Ctrl+V`)**
   - **Screenshot Workflow**: Press `Win+Shift+S` or `PrintScreen` to capture a screen region, then focus the Drink Floyd window and press `Ctrl+V` (or click "Paste (Ctrl+V)").
   - Verify: The screenshot is decoded and loaded into the canvas with status line showing `[Clipboard Screenshot] (WxH)`.
   - **Empty / Text Clipboard Test**: Copy plain text from an editor, then press `Ctrl+V`. Verify: The app **never crashes**; instead, it displays an amber warning in the status line: `"Clipboard does not contain image data (copied text or empty)."`.

---

## Phase 2: Native WASAPI Audio Loopback & FFT Analysis Engine

Phase 2 implements native Windows system-audio capture and real-time frequency analysis:
- **WASAPI Loopback Capture**: Captures whatever audio is playing through the default Windows playback endpoint (`AUDCLNT_STREAMFLAGS_LOOPBACK`).
- **Dynamic Device Change Handling**: Uses `IMMNotificationClient` to seamlessly migrate capture streams when default playback device changes (e.g. unplugging headphones) without crashing or requiring app restart.
- **Sample Rate / Format Conversion**: Transparently handles 44.1k, 48k, 96k, 192k at 16/24/32-bit integer or 32-bit float, downmixing to normalized mono.
- **FFT & Frequency Bands**: 2048-point Hann-windowed FFT partitioned into 4 musical bands:
  - **Bass**: ~20 Hz – 250 Hz
  - **Low-Mid**: ~250 Hz – 500 Hz
  - **Mid**: ~500 Hz – 2000 Hz
  - **High**: ~2000 Hz – 16000 Hz
- **Envelope Followers**: Attack (fast ~12ms) and release (smooth ~160ms) smoothing on each band.
- **Beat & Onset Energy**: Real-time kick/transient flux detection triggering a beat onset pulse.
- **Live Parameter Controls**:
  - `Input Gain`: Adjusts pre-FFT audio sensitivity.
  - `React`: Multiplies visual modulation response (0.0x produces zero reaction, 2.0x produces high sensitivity).
- **Audio Monitor UI**:
  - Top header badge displaying live capture status (`Listening` / `No audio detected` / `Device changed`) and active device name.
  - Dedicated animated debug monitor at the top of the parameter sidebar showing real-time `BASS`, `LOW-MID`, `MID`, `HIGH` level meters and `BEAT` indicator pill.
  - Canvas test value: Texture quad scale dynamically pulsates with bass energy (`1.0 + 0.08 * bass * react`).

---

## Phase 3: GPU Visual Engine, Scene System & Parameter Shader Pipeline

Phase 3 introduces the complete dual-pass OpenGL/GLSL visual engine:
- **Dual-Pass GLSL Pipeline**:
  - **Pass 1 (Warp & Feedback FBO)**: Evaluates per-pixel deformations, optical lenses, kaleidoscopic symmetries, and ping-pong framebuffer persistence (trails, decay, heat).
  - **Pass 2 (Presentation Viewport)**: Color grading, palette remapping (5 presets + user custom swatch), continuous hue drift, cinematic vignette, and brightness scaling.
- **4 Original Scene Systems**:
  - **Smear**: Gravitational melting and directional chromatic pixel sliding driven by bass and beat transients.
  - **Ripple**: Concentric shockwave liquid ripples undulating outwards from center, triggered by kick onsets.
  - **Flow**: Organic liquid advection / curl noise fluid swirl, driving harmonic wave currents.
  - **Tiles**: Kaleidoscopic mosaic grid vibrating, shifting, and rotating quadrant tiles to audio frequencies.
  - *Seamless switching*: Switching scenes never reloads the loaded image or interrupts audio capture.
- **14 Active Shader Uniforms**:
  `react`, `chaos`, `trails`, `heat`, `decay`, `brightness`, `inputGain`, `symmetry`, `tiles`, `twist`, `slice`, `lens`, `hueDrift`, `vignette`.
- **Palette Presets & Custom Colour**:
  - `Prism Core`: Spectral dispersion
  - `Neon Ultraviolet`: Deep violet $\to$ hot magenta $\to$ neon cyan
  - `Solar Flare`: Obsidian black $\to$ deep ruby $\to$ burning amber $\to$ white hot
  - `Monochrome Ice`: Dark navy $\to$ frosted cyan $\to$ silver white
  - `Dark Floyd`: Pitch black $\to$ emerald forest $\to$ prism rainbow highlights
  - `Custom`: Interactive color swatch cycling vibrant designer hues.
- **Lens Style Presets**: `Plain` (spherical convex), `Ribs` (fresnel lines), `Rings` (concentric rings).
- **Randomize Look Button**: Safely randomizes scene-compatible parameters within stable ranges (never black, never broken).
- **Preview Resolution Profiles**: Real-time switching between `1080x1080` (1:1), `1080x1920` (9:16), and `1920x1080` (16:9).
- **True Rendered FPS**: Header readout measures actual GPU VSync frame swap times (~60.0 FPS).

---

## Next Steps (Subsequent Phases)
- **Phase 1**: WASAPI loopback audio capture integration.
- **Phase 2**: Real-time GL/DirectX shader pipeline & parameter modulation.
- **Phase 3**: Offline video export engine (1080x1080, 1080x1920, 1920x1080 @ 30/60 fps).
