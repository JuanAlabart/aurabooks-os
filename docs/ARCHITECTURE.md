# AuraBooks Native C/C++ Embedded OS (Kindle Touch D01200)
## Architectural Specification & Engineering Blueprint

---

### 1. Hardware Target Specification
* **Device:** Amazon Kindle Touch (Model: D01200, 4th Generation Touch, K5)
* **SoC:** Freescale / NXP i.MX508 (ARMv7-A, Cortex-A8 core, 800 MHz)
* **RAM:** 256 MB DDR2 (Target budget for AuraBooks: **≤ 16 MB**)
* **Operating System:** Amazon Linux 2.6.31-rt11-lab126
* **C Library:** GNU C Library (`glibc 2.12.2`, ABI: `arm-linux-gnueabi` softfp)
* **Display Controller:** Freescale EPDC (E-Ink Pearl Controller) mapped to `/dev/fb0`
  * Resolution: **600 × 800** pixels, 16 grayscale levels (4-bit / 8-bit framebuffer modes)
  * Hardware ioctls: `MXCFB_SEND_UPDATE`, `MXCFB_WAIT_FOR_UPDATE_COMPLETE`
  * Waveform Modes:
    * `WAVEFORM_MODE_GC16`: Full flashing refresh (high contrast, clears all ghosting)
    * `WAVEFORM_MODE_GL16`: High-speed partial refresh for text reading
    * `WAVEFORM_MODE_DU`: Direct Update (black-and-white 1-bit, ~80ms response for typing/UI)
* **Touch Controller:** Neonode zForce Optical Infrasensor
  * Linux device: `/dev/input/event0` (or `/dev/input/event1`)
  * Protocol: Linux Input Subsystem `evdev` multi-touch (`EV_ABS`, `ABS_MT_POSITION_X`, `ABS_MT_POSITION_Y`, `EV_KEY`, `BTN_TOUCH`)

---

### 2. Software Architecture Layers

```
┌────────────────────────────────────────────────────────────────────────┐
│                   AuraBooks Native Application Layer                   │
│         [ Inicio Dashboard ]    [ Biblioteca ]    [ Citas ]            │
├────────────────────────────────────────────────────────────────────────┤
│                   AuraBooks High-Level UI Framework                    │
│     Cards  •  Buttons  •  Text  •  Modals  •  Images  •  Animations     │
├───────────────────────────────────┬────────────────────────────────────┤
│         aura_reader               │             aura_db                │
│    (CREngine / libmobi / MuPDF)   │    (SQLite3 + Reading Progress)    │
├───────────────────────────────────┴────────────────────────────────────┤
│                     AuraBooks Hardware HAL (Low Level)                 │
│  aura_fb (EPDC /dev/fb0)   •   aura_touch (/dev/input/event0)          │
│  aura_font (FreeType2)     •   aura_img (libjpeg-turbo / libpng)       │
├────────────────────────────────────────────────────────────────────────┤
│                     Linux Kernel 2.6.31 / Drivers                      │
└────────────────────────────────────────────────────────────────────────┘
```

---

### 3. Core Modules & Responsibilities

#### A. Hardware Abstraction Layer (HAL)
1. **`aura_fb.c / .h` (Framebuffer & EPDC Engine)**
   * Maps `/dev/fb0` via `mmap()`.
   * Manages double-buffering: back buffer (drawing in RAM) and front buffer (hardware screen).
   * Provides non-blocking hardware e-ink waveform update triggers:
     * `aura_fb_refresh_rect(x, y, w, h, REFRESH_PARTIAL)`
     * `aura_fb_refresh_full()`
2. **`aura_touch.c / .h` (Touch Input Engine)**
   * Opens `/dev/input/event0` with `O_NONBLOCK` or epoll/select.
   * Parses raw `struct input_event` into clean, debounced gestures:
     * `EVENT_TAP`, `EVENT_DOUBLE_TAP`, `EVENT_LONG_PRESS`, `EVENT_SWIPE_LEFT`, `EVENT_SWIPE_RIGHT`.
3. **`aura_font.c / .h` (Vector Typography Engine)**
   * Links against `libfreetype2`.
   * Loads high-contrast serif font (`Literata-Regular.ttf`, `Literata-Bold.ttf`, `Literata-Italic.ttf`).
   * Renders subpixel-antialiased glyphs directly into the 8-bit grayscale pixel buffer.

#### B. UI Framework
* **`aura_ui.c / .h`**
  * Retained/immediate-mode hybrid UI toolkit designed specifically for E-Ink:
  * Eliminates redundant paints: only modified card regions are sent to the hardware EPDC.
  * Standard components:
    * `aura_card`: rounded rectangle with configurable borders, radius, padding.
    * `aura_button`: tactile feedback on press with inverted colors during tap.
    * `aura_navbar`: floating bottom pill with active tab indicator.
    * `aura_progress_bar`: progress bar with percentage.

#### C. Reader & Storage Engine
* **`aura_reader.c / .h`**
  * E-Book parsing (EPUB, MOBI, AZW3).
  * Chapter pagination, font scaling, margin control, word wrapping, hyphenation.
* **`aura_db.c / .h`**
  * Embedded SQLite3 database in `/mnt/us/aurabooks/aurabooks.db`.
  * Tables: `books` (metadata, covers, ratings), `reading_progress` (current page, timestamps), `quotes` (user highlights + curated classics).

---

### 4. Build System & Compilation Pipeline
* **Target Triplet:** `arm-kindle5-linux-gnueabi`
* **Compilers:** GCC 4.9.4 / 7.5.0 cross-toolchain matching Kindle glibc 2.12.
* **Continuous Integration:** GitHub Actions workflow compiles the binary automatically on push and creates release artifacts (`aurabooks_bin`).
