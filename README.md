# Sleeping Knights 265 (`sk265`)

[English](README.md) | [中文说明](README-CN.md)

**Sleeping Knights 265** is a modern, high-performance, modular standalone CLI frontend for the `x265` HEVC encoder, built with Modern C++20.

It decouples user-facing CLI handling, FrameServer inputs, container multiplexing, and pipeline orchestration from `libx265`'s internal implementation details, communicating strictly through public C APIs.

---

## Encoder & User Guide

### 1. Quick Start

`sk265` maintains 100% backward compatibility with standard `x265` CLI parameters while offering out-of-the-box container multiplexing and FrameServer support:

```bash
# 1. Direct video transcode (auto FFmpeg decoding -> 10-bit MP4 output via L-SMASH)
sk265 -i source.mp4 -o encoded.mp4 --preset slow --crf 18

# 2. VapourSynth script encode to MKV with VCB-S anime tuning
sk265 -i script.vpy -o output.mkv --tune vcb-s --preset slow --crf 17.5 --stylish

# 3. AviSynth+ script with user preset profile
sk265 --config anime -i script.avs -o output.mp4

# 4. Y4M standard input streaming to raw HEVC bitstream
vspipe -y script.vpy - | sk265 -i - -o stream.hevc --crf 19
```

---

### 2. Key Features for Encoders

#### 🎯 Default 10-bit High Quality Encoding
- **Default Depth**: Encodes in **10-bit (`Main 10`)** by default when `-D` is omitted, eliminating color banding and maximizing compression efficiency.
- Pass `-D 8` or `-D 12` explicitly if 8-bit or 12-bit output is required.

#### 🎨 Dedicated Tuning Presets (`--tune`)
- **`--tune vcb-s` / `vcbs`** (and high-precision `--tune vcb-s++`): Tailored tuning matrix for high-bitrate anime BDs and film (`ctu 32`, `qg-size 8`, `rd 4`, `rdoq 2`, `psy-rd 1.8`, `psy-rdoq 1.0`, `cb/cr -2:-2`, `deblock -1:-1`, `no-sao`, `no-strong-intra-smoothing`, `pbratio 1.2`, `weightb 1`).
- **`--tune lp` / `littlepox`** (and `--tune lp++`): Optimized tuning matrix for mid/low-bitrate anime.

#### 📦 Container Multiplexing & Engine Selection (`--muxer`)
- **Smart Automatic Routing (Default `--muxer auto`)**:
  - `.mp4` $\to$ **L-SMASH** engine (strictly ISO `hvc1` compliant, automatic in-band parameter set deduplication, NCLX/SAR atoms, 64-bit precise timeline with Edit List).
  - `.mkv` $\to$ **LAVF (FFmpeg)** Matroska engine.
  - `.hevc` / `.h265` / `.265` / `.bin` / `.raw` / `-` $\to$ Raw Annex-B bitstream.
- **Manual Engine Selection**:
  - `--muxer lsmash`: Enforce L-SMASH for MP4/MOV.
  - `--muxer lavf` (or `--muxer ffmpeg`): Enforce FFmpeg multiplexer for MP4 or MKV.
  - `--muxer raw`: Enforce raw Annex-B stream output.
  - Incompatible container and muxer combinations are caught and rejected with clear error guidance.

#### 📥 Comprehensive Input Ecosystem
- **AviSynth+**: Native UTF-8 Unicode script path support via `Import(..., utf8=true)`; custom library loading via `--avs-lib <path>`; supports YUV 4:0:0 / 4:2:0 / 4:2:2 / 4:4:4 across 8~16 bit depths.
- **VapourSynth (API 4)**: Direct `.vpy` evaluation; custom library loading via `--vpy-lib <path>`.
- **LAVF (FFmpeg) Direct Decoding**: Decodes `.mp4`, `.mkv`, `.ts`, `.m2ts`, `.avi`, `.flv`, `.mov`, `.webm`, etc., without needing external frame server scripts.
- **Y4M**: Supports `.y4m` files and stdin streaming (`-i -`).

#### ⚙️ Cascading Configuration Engine (`--config`)
- **Comment Support**: Full support for `#` and `//` line and inline comments in plain CLI text configuration files (`.txt` / `.conf`).
- **Four-Tier Priority Cascade**:
  $$\text{Preset Defaults} \;\to\; \text{Global Default (\texttt{default.txt})} \;\to\; \text{Profile Config} \;\to\; \text{CLI Arguments}$$
- **Strict Profile Name Lookup**: `--config anime` strictly searches in `~/.config/sk265/anime.txt` (Windows: `%APPDATA%/sk265/anime.txt`), never accidentally triggering local non-config files; explicit file paths (e.g. `--config ./anime.txt`) read the specified file directly.
- **Trace Logging**: Console logs report which configuration files were loaded and what parameters were injected.

#### 📊 UI, Logging & IPC Monitoring
- **`--stylish` x264-r2204 Style Progress Bar**:
  ```text
           frames        fps    kb/s      elapsed    remain       size   est.size
  [ 50.0%]  500/1000   45.21  2500.30    00:01:23   00:01:23   12.45 MB  24.90 MB
  ```
- **`--jsonl` Raw Data Stream**: Emits compact, unrounded JSON progress lines to `stderr` for GUIs, automation scripts, and cluster schedulers:
  ```json
  {"frame":500,"total_frames":1000,"bytes":13054812,"elapsed_ms":11050,"fps":45.21,"bitrate":2500.30}
  ```
- **`--progress-file <file>` & `--log-file <file>`**: Periodically saves progress JSON to a file for polling, and mirrors console logs to a log file.
- **`--opts <0|1|2|3>` / `--level-of-options`**: Controls SEI User Data configuration options detail level.
- **Graceful Ctrl+C Interrupts**: Clean pipeline shutdown with exit code `2` upon abort, eliminating hang risks.

---

## Architecture & Developer Guide

### 1. Architectural Principles

`sk265` adheres strictly to modern software engineering principles:

1. **Zero Core Coupling**:
   The frontend communicates with `libx265` strictly through its public C API (`x265.h`). Never includes internal headers (`common.h`, `encoder.h`, `param.h`).
2. **Symmetric Multi-Depth Routing**:
   Eliminates upstream's 8-bit master architecture. 8-bit, 10-bit, and 12-bit cores are symmetric, isolated, and dispatched dynamically via `CoreRouter::getApi(depth)`.
3. **C++20 Asynchronous Bounded Pipeline (`BoundedQueue`)**:
   Condition-variable based bounded prefetch queue overlapping input decoding with encoder consumption. Automatically adapts queue depth by resolution (4 frames for 4K UHD, 16 frames for 1080p, 32 frames for 720p/SD) with backpressure and graceful cancellation support.
4. **Clean Modern C++20 RAII**:
   Zero owning raw pointers. All C handles and allocations are encapsulated with `std::unique_ptr`, `std::span`, and custom RAII deleters.

---

### 2. Build Instructions

#### Prerequisites
- C++20 compliant compiler (GCC $\ge$ 11, Clang $\ge$ 14, or MSVC $\ge$ 2019)
- CMake $\ge$ 3.20
- Ninja build system
- NASM $\ge$ 2.15 (for assembly optimization)

#### Building on Windows (MinGW / UCRT64)
```powershell
# 1. Clone repository with submodules
git clone --recurse-submodules https://github.com/msg7086/sleeping-knights-265.git
cd sleeping-knights-265

# 2. Configure and build
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build

# 3. Run automated test suite
ctest --test-dir build --output-on-failure
```

#### Building on Linux
```bash
# 1. Install dependencies (Debian/Ubuntu example)
sudo apt-get install -y cmake ninja-build build-essential nasm libavformat-dev libavcodec-dev libavutil-dev libswscale-dev

# 2. Configure and build
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build

# 3. Run test suite
ctest --test-dir build --output-on-failure
```

---

## License

The **Sleeping Knights 265** project is licensed under the **GNU General Public License v2.0 or later** (GPL-2.0-or-later). See [`LICENSE`](LICENSE) for full license details.
