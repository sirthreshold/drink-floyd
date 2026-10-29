# Drink Floyd 2.0

> **Original Audio-Reactive Desktop Music Visualizer for Windows**  
> Built with C++20, OpenGL GLSL Shaders, JUCE 8, and WASAPI Loopback Capture.

Drink Floyd is a high-performance, real-time music visualizer designed like a sleek macOS Pro app. It captures whatever audio is playing on your Windows PC with near-zero latency, breaking loaded artwork, album covers, or live YouTube video frames into dynamic audio-reactive pixel melts, sand scatters, and kaleidoscopic fluid warps.

---

## Quick Download for Windows (No Compilation Required)

Your friends do not need to install CMake or Visual Studio to run Drink Floyd!

1. Go to the [Releases](https://github.com/) tab on this repository.
2. Download **`Drink-Floyd-2.0-Windows.zip`**.
3. Extract the ZIP anywhere on your PC.
4. Double-click **`drinkfloys2.0.exe`** (or `Drink Floyd 2.0.exe`) and start playing music!

---

## Included Custom Presets

The app comes with your custom looks already installed and ready in the **PRESETS & PROFILES** dropdown:
- **`landscsape`**: Smooth dynamic pixel melt & scatter with dual-axis symmetry, subtle lens refraction, and warm tones.
- **`purple haze`**: Kaleidoscopic multi-tile distortion with deep electric violet / magenta aesthetic tinting.
- **`randomizer`**: Wide-angle sand disperse with Blood Moon gradient and audio-reactive swirling curl field.
- **`sand`**: Fine-grain pixel separation with underlying image transparency and snappy beat response.
- **`My Custom Preset`**: High-reactivity custom color-tinted melt look.
*Plus 10 factory audio-reactive styles (808 Disperse, House Quads, Droste Spiral, Cyberpunk Datamosh, etc.).*

---

## Key Features

- **macOS Dark Theme UI/UX**: Authentic macOS design featuring traffic light window controls (close, minimize, fullscreen), capsule sliders, dark slate cards, and Logic Pro style audio DSP inspector.
- **Multi-Genre Music Reactivity**: Multi-band spectral flux tracking kicks, 808 sub-bass, sharp snares/claps, rolling trap hi-hats, and vocal transients across hip hop, rock, pop, EDM, and funk.
- **Automatic YouTube Video Sync**: Click `[Sync YouTube]` (or press `F9`) while playing any video on YouTube in Chrome, Edge, Brave, or Firefox. Drink Floyd automatically captures the video frame and displays it without manual copy-pasting.
- **22 Visual Scenes**: Pixel Melt & Scatter, Sand Canvas Disperse, Smear, Ripple, Flow, Tiles, Droste Spiral, Hyperspace Tunnel, Corridor, Cube, Planet, Windows Glitch, Melt, Shatter, Prism, Terrain, Breathe, Datamosh, Sort, Coral, Scanline, and Pure Image.
- **16 Gradient Color Swatches**: Neon Ultraviolet, Solar Flare, Monochrome Ice, Dark Floyd, Cyberpunk Sunset, Vaporwave Twilight, Emerald Aurora, Blood Moon, Deep Ocean Abyss, Acid Toxic Green, Hyperpop Bubblegum, Golden Hour, Infrared Thermal, Retro Synthwave, and Custom Tint.
- **True Borderless Fullscreen**: Press `F11` or `F` to enter borderless cover mode that fills the entire display with zero borders. Press `Esc` or double-click to return.

---

## Keyboard Shortcuts

| Shortcut | Action |
|---|---|
| `F11` or `F` | Toggle True Borderless Fullscreen |
| `Esc` | Exit Fullscreen |
| `F9` or `Ctrl + Y` | Automatically Sync YouTube Video Player |
| `Ctrl + V` | Paste Image from Clipboard |
| `Ctrl + O` | Open Image File Dialog |
| Double-Click Canvas | Toggle Fullscreen |

---

## Building from Source (Developers)

### Prerequisites
- Visual Studio 2022 (with *Desktop development with C++*)
- CMake 3.22 or higher
- Windows 10/11 SDK

```powershell
git clone <your-repo-url>
cd drink-floyd
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```
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
