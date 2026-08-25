# Sleeping Knights 265 (`sk265`)

[English](README.md) | [中文说明](README-CN.md)

**Sleeping Knights 265** 是为 `x265` HEVC 编码器打造的现代化、高吞吐、解耦型独立命令行前端（Standalone CLI Frontend）。

它将用户交互、参数解析、FrameServer 脚本输入、媒体容器封装与流水线调度从 `libx265` 内部实现中完全剥离，严格通过公开 C API 进行通信。

---

## 压制者使用指南 (Encoder User Guide)

### 1. 快速上手

`sk265` 保持对官方 `x265` 编码参数的全面向后兼容，同时提供开箱即用的多容器直出与 FrameServer 支持：

```bash
# 1. 视频容器直接转码（自动调用 FFmpeg 解码并输出 10-bit MP4）
sk265 -i source.mp4 -o encoded.mp4 --preset slow --crf 18

# 2. VapourSynth 脚本压制并输出 MKV（使用 VCB-S 动画专属调优）
sk265 -i script.vpy -o output.mkv --tune vcb-s --preset slow --crf 17.5 --stylish

# 3. AviSynth+ 脚本压制并应用个人预设 Profile
sk265 --config anime -i script.avs -o output.mp4

# 4. Y4M 管道流式输入与原始码流输出
vspipe -y script.vpy - | sk265 -i - -o stream.hevc --crf 19
```

---

### 2. 核心压制特性

#### 🎯 默认 10-bit 高质量编码
- **默认位深**：未指定 `-D` 时默认按 **10-bit (`Main 10`)** 进行编码，有效抑制色彩断层（Banding）并提升压缩效率；
- 如需输出 8-bit 或 12-bit，可显式指定 `-D 8` 或 `-D 12`。

#### 🎨 专属画质调优预设 (`--tune`)
- **`--tune vcb-s` / `vcbs`**（及高精度版 `--tune vcb-s++`）：专为二次元动画 BD 与高质量电影定制的画质参数矩阵（`ctu 32`, `qg-size 8`, `rd 4`, `rdoq 2`, `psy-rd 1.8`, `psy-rdoq 1.0`, `cb/cr -2:-2`, `deblock -1:-1`, `no-sao`, `no-strong-intra-smoothing`, `pbratio 1.2`, `weightb 1`）；
- **`--tune lp` / `littlepox`**（及 `--tune lp++`）：针对中低码率动画的画质优化矩阵。

#### 📦 多容器直出与混流器选择 (`--muxer`)
- **智能自动路由（默认 `--muxer auto`）**：
  - 输出 `.mp4` $\to$ 默认使用 **L-SMASH** 引擎（严格符合 ISO `hvc1` 规范，自动去重带内参数集，注入 NCLX/SAR 原子与 64-bit 精准时间线）；
  - 输出 `.mkv` $\to$ 默认使用 **LAVF (FFmpeg)** 引擎；
  - 输出 `.hevc` / `.h265` / `.265` / `.bin` / `.raw` / `-` $\to$ 输出 Annex-B 原始裸流。
- **手动选择**：支持通过 `--muxer lsmash`、`--muxer lavf`（别名 `ffmpeg`）、`--muxer raw` 手动指定封装后端；对不合法的文件组合进行前置强校验拦截。

#### 📥 丰富的输入源支持
- **AviSynth+**：原生支持 UTF-8 中文与空格路径（`Import(..., utf8=true)`）；支持 `--avs-lib <path>` 自定义 DLL 路径；支持 YUV 4:0:0 / 4:2:0 / 4:2:2 / 4:4:4 及 8~16 位深。
- **VapourSynth (API 4)**：支持直接解析 `.vpy` 脚本；支持 `--vpy-lib <path>` 自定义核心库。
- **LAVF 原盘直读**：支持无需手写脚本直接读取 `.mp4`、`.mkv`、`.ts`、`.m2ts`、`.avi` 等主流视频文件。
- **Y4M**：支持文件与标准输入管道（`-i -`）。

#### ⚙️ 级联配置引擎 (`--config`)
- **注释支持**：支持带有 `#` 和 `//` 注释的纯文本 CLI 配置文件（`.txt` / `.conf`）；
- **四级优先级级联覆盖**：
  $$\text{Preset 默认值} \;\to\; \text{全局默认 (\texttt{default.txt})} \;\to\; \text{Profile 配置文件} \;\to\; \text{CLI 命令行实参}$$
- **专属命名寻址**：`--config anime` 严格只搜寻 `~/.config/sk265/anime.txt`（Windows 下为 `%APPDATA%/sk265/anime.txt`），绝不误触本地普通文件；显式指定路径（如 `--config ./my.txt`）直接读取指定文件。

#### 📊 终端监控与 IPC 进度流
- **`--stylish` 经典多列对齐进度条**：
  ```text
           frames        fps    kb/s      elapsed    remain       size   est.size
  [ 50.0%]  500/1000   45.21  2500.30    00:01:23   00:01:23   12.45 MB  24.90 MB
  ```
- **`--jsonl` 紧凑原始数据流**：向 `stderr` 逐行输出未截断的纯粹原始 JSON 格式，专供 GUI、自动化脚本与云端集群解析：
  ```json
  {"frame":500,"total_frames":1000,"bytes":13054812,"elapsed_ms":11050,"fps":45.21,"bitrate":2500.30}
  ```
- **`--progress-file <file>` 与 `--log-file <file>`**：支持定期将进度写入外部文件供无管道轮询监控，并将控制台日志镜像记录到日志文件。
- **`--opts <0|1|2|3>` / `--level-of-options`**：精细控制注入 SEI User Data 中的参数详细程度。

---

## 开发者与编译构建指南 (Developer & Build Guide)

### 1. 架构设计原理

`sk265` 在架构层面追求高度解耦、稳健性与吞吐效率：

1. **零内核耦合（Zero Core Coupling）**：
   前端与 `libx265` 严格仅通过公开 C API（`x265.h`）进行通信，严禁包含 `libx265` 的私有内部头文件；
2. **对称多深度路由（Symmetric Multi-Depth Routing）**：
   消除官方 upstream 的 8-bit master 桥接机制，8-bit、10-bit、12-bit 作为平等的独立核心通过 `CoreRouter::getApi(depth)` 对称分发；
3. **C++20 异步双缓冲流水线（`BoundedQueue`）**：
   基于条件变量与移动语义的有界预取队列，生产（输入解码）与消费（编码输出）并行重叠，根据分辨率智能自适应队列容量（4K UHD 默认 4 帧、1080p 默认 16 帧、720p 默认 32 帧），具备安全背压与优雅取消机制；
4. **全工程 Modern C++20 纯 RAII**：
   全局杜绝裸指针与手动内存管理，所有 C 结构体均通过智能指针及自定义 Deleter 管理生命周期。

---

### 2. 编译构建

#### 编译依赖
- 支持 C++20 的编译器（GCC $\ge$ 11、Clang $\ge$ 14 或 MSVC $\ge$ 2019）
- CMake $\ge$ 3.20
- Ninja 构建工具
- NASM $\ge$ 2.15（用于汇编加速）

#### 在 Windows 上构建 (MinGW / UCRT64)
```powershell
# 1. 克隆代码库及子模块
git clone --recurse-submodules https://github.com/msg7086/sleeping-knights-265.git
cd sleeping-knights-265

# 2. 配置并编译
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build

# 3. 运行自动化测试套件
ctest --test-dir build --output-on-failure
```

#### 在 Linux 上构建
```bash
# 1. 安装基础开发库 (以 Debian/Ubuntu 为例)
sudo apt-get install -y cmake ninja-build build-essential nasm libavformat-dev libavcodec-dev libavutil-dev libswscale-dev

# 2. 配置并编译
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build

# 3. 运行测试
ctest --test-dir build --output-on-failure
```

---

## 许可证 (License)

The **Sleeping Knights 265** project is licensed under the **GNU General Public License v2.0 or later** (GPL-2.0-or-later). See [`LICENSE`](LICENSE) for details.
