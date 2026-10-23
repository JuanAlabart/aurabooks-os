# AuraBooks Native Embedded OS (Kindle Touch D01200)

**AuraBooks Native** is an independent, ultra-lightweight, high-performance C/C++ embedded operating environment engineered specifically for the Amazon Kindle Touch (Freescale/NXP i.MX508 ARMv7 Cortex-A8, 800 MHz, 256 MB RAM, E-Ink Pearl 600×800).

---

## Key Engineering Highlights
* **Zero Bloat:** Replaces Amazon's 100+ MB Java stack with a unified native C binary consuming **< 16 MB of RAM**.
* **Direct Hardware E-Ink HAL:** Native ioctl interface to `/dev/fb0` with Freescale EPDC hardware waveform control (`GC16`, `GL16`, `DU`).
* **Instant Touch Response:** Direct `/dev/input/event0` multi-touch parsing with hardware-level gesture recognition.
* **Vector Typography:** TrueType / OpenType font rendering with subpixel antialiasing and mathematical line wrapping.
* **Integrated Reader:** PalmDOC (`.azw`, `.azw3`, `.mobi`) and EPUB reader engine with book covers, star ratings, and persistent quotes notebook.
* **Continuous Integration:** GitHub Actions cross-compiles bit-perfect ARM binaries on every push.

---

## Directory Structure
```
├── .github/workflows/      # Automated ARM cross-compilation pipeline
├── docs/                   # Engineering architecture and memory maps
├── include/                # Public header definitions
├── src/
│   ├── core/               # Framebuffer, EPDC driver, main loop
│   ├── input/              # Evdev touch input and gesture engine
│   ├── gfx/                # Typography, font cache, image decoder
│   ├── ui/                 # Retained-mode E-Ink UI toolkit (Cards, Buttons, Modals)
│   ├── reader/             # PalmDOC/EPUB parser and pagination engine
│   ├── db/                 # Embedded SQLite3 metadata & quotes storage
│   └── main.c              # Application entry point
└── Makefile                # Cross-compiler build rules
```

---

## Building
### 1. Cross-compiling for Kindle Touch (ARM)
```bash
make CC=arm-linux-gnueabi-gcc
```

### 2. Building for Desktop / Headless Simulation
```bash
make CC=gcc CFLAGS="-O2 -std=gnu11 -Iinclude -DAURA_SIMULATION"
```
