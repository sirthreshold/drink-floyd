# Drink Floyd — System Specification

## 1. Product Overview & Vision
**Drink Floyd** is an original standalone desktop audio-reactive music visualizer for Windows.
It captures live playback from the system audio stream and translates sound dynamics into rich real-time visual art, with offline rendering capabilities for video export.

This project is an **original** implementation. It uses no proprietary assets, branding, or code from existing visualizers.

---

## 2. Platform & Distribution Scope (v1)
- **Target OS**: Windows 10 / Windows 11 (64-bit x86_64).
- **Target Form Factor**: **Windows STANDALONE Application ONLY**.
- **Explicit Target Exclusions (v1)**:
  - **NO VST3 / DAW plugin target**.
  - **NO AU (AudioUnit) target**.
  - **NO AAX / CLAP target**.
  - **NO DAW host automation / DAW parameter reflection**.
  - **NO browser / WebAssembly version**.
  - **NO mobile (iOS / Android) targets**.

The application operates as a dedicated desktop tool with its own dedicated window, direct GPU rendering pipeline, and native audio loopback capture.

---

## 3. Display & Export Specifications

### 3.1 Supported Canvas & Export Aspect Ratios / Resolutions
1. **Square (1:1)**: `1080 x 1080` (optimized for Instagram posts, album artwork displays)
2. **Vertical / Portrait (9:16)**: `1080 x 1920` (optimized for TikTok, Instagram Reels, YouTube Shorts)
3. **Landscape / Widescreen (16:9)**: `1920 x 1080` (optimized for YouTube, desktop monitors, stage projection)

### 3.2 Real-time Display & Offline Export Framerates
- **Real-Time Display**: VSync-aligned preview (target 60 FPS, with dynamic FPS counter in header).
- **Offline Video Export**:
  - `30 FPS` (standard social/streaming delivery)
  - `60 FPS` (high-fidelity motion delivery)

---

## 4. Audio Input Pipeline
- **Required Audio Input Architecture**:
  - **Windows Audio Session API (WASAPI) System Loopback ONLY**.
  - Direct loopback recording of the default Windows audio output device (what you hear through headphones/speakers).
  - Eliminates the need to select or re-route virtual audio cables.
- **Audio File Import**:
  - **Explicitly NOT required in v1**. Visualizer is driven live by whichever media player, browser, Spotify, or DAW is currently playing audio through Windows.

---

## 5. Parameter Specification
The visual engine is governed by 16 canonical parameters:

| Parameter | Type / Range | Category | Description |
| :--- | :--- | :--- | :--- |
| **react** | Float [0.0 – 1.0] | Dynamics | Sensitivity of visual deformation to audio energy peaks. |
| **chaos** | Float [0.0 – 1.0] | Dynamics | Noise-driven turbulence and unpredictable spatial modulation. |
| **trails** | Float [0.0 – 1.0] | Persistence | Frame feedback accumulation strength (temporal echo). |
| **heat** | Float [0.0 – 1.0] | Dynamics | Overdrive saturation and luminous core blooming. |
| **decay** | Float [0.0 – 1.0] | Persistence | Dissipation rate of trails and audio excitation energy. |
| **brightness** | Float [0.0 – 2.0] | Color & Light | Master luminance multiplier. |
| **input gain** | Float [0.0 – 2.0] | Audio | Pre-amplifier gain applied to WASAPI loopback audio signal. |
| **symmetry** | Integer [1 – 12] | Geometry | Radial kaleidoscope symmetry folding order. |
| **tiles** | Integer [1 – 16] | Geometry | Grid tessellation repetition factor across UV coordinates. |
| **twist** | Float [-1.0 – +1.0] | Geometry | Spiral angular torque applied around the center coordinate. |
| **slice** | Float [0.0 – 1.0] | Geometry | Linear/angular plane shear displacement. |
| **lens** | Float [0.0 – 1.0] | Optics | Optical curvature / refractive focal distortion intensity. |
| **hue drift** | Float [0.0 – 1.0] | Color & Light | Continuous cycle speed of color spectrum rotation. |
| **vignette** | Float [0.0 – 1.0] | Optics | Radial perimeter darkening and border falloff. |
| **palette** | Enum / Dropdown | Color & Light | Color gradient lookup (e.g. Prism, Neon, Monochrome, Sunset). |
| **lens style** | Enum / Dropdown | Optics | Optical distortion algorithm (e.g. Fisheye, Barrel, Pincushion, Prismatic). |

---

## 6. Architecture & User Interface Layout (Phase 0)
- **Top Header Bar**:
  - Product title & status indicator.
  - Live FPS readout (updated via frame delta timer).
  - Aspect ratio toggle preview indicators (1080x1080, 1080x1920, 1920x1080).
- **Left Main Canvas**:
  - Prominent visual canvas area preserving selected aspect ratio.
  - Render target area for the upcoming graphics engine.
- **Right Parameter Sidebar**:
  - Fixed-width dark UI panel (`#14161b` / `#1b1e26`).
  - Structured parameter cards categorized by function:
    - *Dynamics & Energy*
    - *Geometry & Form*
    - *Color & Aesthetics*
    - *Optics & FX*
- **Design Language**:
  - Dark theme with high-contrast readouts, subdued slate paneling, and minimal distractions.

---

## 7. Non-Goals & Out-of-Scope (v1)
- No VST / VST3 / AU / AAX plugin architecture.
- No MIDI input or host transport sync.
- No DAW automation curves or DAW preset management.
- No web browser or WebGL compilation target.
- No macOS / Linux / iOS / Android builds in v1.
- No embedded audio player or playlist management (audio comes exclusively via WASAPI loopback).
