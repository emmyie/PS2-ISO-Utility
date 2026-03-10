# PS2 ISO Utility — ISO Splitter

A Windows desktop application that splits PlayStation 2 ISO files into 1 GB chunks compatible with **OpenPS2Loader (OPL)**. The tool reads ISO9660 metadata directly from the image, computes OPL-compatible filenames and a CRC32 hash, splits the file in a background thread with a live progress bar, and writes the required `ul.cfg` configuration entry automatically.

---

## Table of Contents

- [Features](#features)
- [How It Works](#how-it-works)
  - [1. Metadata Extraction](#1-metadata-extraction)
  - [2. Pre-split Calculations](#2-pre-split-calculations)
  - [3. Streaming Split](#3-streaming-split)
  - [4. OPL Configuration](#4-opl-configuration)
- [Output Format](#output-format)
- [Project Structure](#project-structure)
- [Dependencies](#dependencies)
- [Building with MSVC](#building-with-msvc)
  - [Prerequisites](#prerequisites)
  - [Option A — Visual Studio IDE](#option-a--visual-studio-ide)
  - [Option B — Command Line (MSBuild)](#option-b--command-line-msbuild)
- [License](#license)

---

## Features

- Splits any PS2 ISO into sequential 1 GB part files ready for OPL's `ul.*` format.
- Parses the ISO9660 file system to extract the boot executable ID (e.g., `SLUS12345`) without any external tools.
- Automatically detects whether the disc is CD (~700 MB) or DVD based on file size, or lets you override it.
- Generates OPL-compatible CRC32 hashes used in part-file naming.
- Writes a 64-byte `ul.cfg` record with title, boot ID, part count, and media type.
- Verifies that the sum of all written part files equals the original ISO size.
- Runs the split in a detached background thread so the UI stays responsive with a real-time progress bar.
- Custom pastel-themed ImGui UI with an integrated file browser filtered to `.iso` files.
- Supports a **dry-run** mode that calculates everything without writing any files.

---

## How It Works

### 1. Metadata Extraction

When you click **Split ISO**, the tool opens the `.iso` as a binary stream and navigates the ISO9660 file system:

1. Seeks to the **Primary Volume Descriptor** at sector 16 (byte offset `0x8000`).
2. Validates the `CD001` signature and version byte.
3. Reads the root directory record from the PVD and scans directory entries for `SYSTEM.CNF`.
4. Reads `SYSTEM.CNF` and searches for the `CDROM0:\` prefix to extract the boot executable name (e.g., `SCUS12345.ELF` → `SCUS12345`).

### 2. Pre-split Calculations

- The user-supplied title (or the boot ID if left blank) is sanitised to at most 32 ASCII characters.
- A CRC32 hash of the sanitised title is computed using the OPL-compatible polynomial (`0x04C11DB7`) and formatted as an 8-character uppercase hex string (e.g., `A1B2C3D4`).
- Media type is decided: ISOs larger than 700 MB are treated as DVD (`0x14`), smaller ones as CD (`0x12`).
- The number of 1 GB parts is calculated; the tool throws an error if more than 255 would be needed.

### 3. Streaming Split

The split runs in a `std::thread` to keep the UI live:

- A 4 MB read buffer is allocated once and reused across all parts.
- For each part index `NN` (zero-based, two-digit):
  - A file named `ul.<CRC>.<BOOT_ID>.<NN>` is created in the output directory (defaults to `<ISO path>.split/`).
  - The input stream is consumed in 4 MB blocks until either 1 GB has been written to this part or the ISO is exhausted.
  - After every block a progress callback updates the UI progress bar (`processed / total`).
- After all parts are written, the total byte count of every part file is summed and compared against the original ISO size. A mismatch raises an error.

### 4. OPL Configuration

A 64-byte binary record is appended to `ul.cfg` in the output directory:

| Offset | Size | Content |
|--------|------|---------|
| `0x00` | 32 B | Game title, space-padded |
| `0x20` | 15 B | `ul.<BOOT_ID>`, space-padded |
| `0x2F` | 1 B  | Number of parts |
| `0x30` | 1 B  | Media type (`0x14` = DVD, `0x12` = CD) |
| `0x36` | 1 B  | Always `0x08` |

---

## Output Format

Given a game with boot ID `SLUS12345`, title `My Game`, and a CRC of `A1B2C3D4`, a 5 GB ISO produces:

```
<output dir>/
├── ul.A1B2C3D4.SLUS12345.00   (1 GB)
├── ul.A1B2C3D4.SLUS12345.01   (1 GB)
├── ul.A1B2C3D4.SLUS12345.02   (1 GB)
├── ul.A1B2C3D4.SLUS12345.03   (1 GB)
├── ul.A1B2C3D4.SLUS12345.04   (~1 GB remainder)
└── ul.cfg                     (64 bytes appended)
```

Copy the `ul.*` files and `ul.cfg` to the `ul` directory on your OPL storage device.

---

## Project Structure

```
PS2-ISO-Utility/
├── main.cpp                        # WinMain — initialise, event loop, shutdown
├── ISO Splitter.sln                # Visual Studio solution
├── ISO Splitter.vcxproj            # MSVC project (Win32 & x64, Debug & Release)
├── vcpkg.json                      # vcpkg dependency manifest
├── LICENSE                         # Unlicense (public domain)
│
├── ui/
│   ├── menu.hpp                    # Window setup, ImGui frame loop, split trigger
│   ├── file_browser.hpp            # Directory-browsing ISO picker
│   └── theme.hpp                   # Custom colour scheme, fonts, and widget wrappers
│
└── utility/
    ├── iso_splitter.hpp            # Core split() logic and split_options / split_result
    ├── iso_util.hpp                # ISO9660 parser — reads SYSTEM.CNF and boot ID
    ├── ul_cfg_mgr.hpp              # ul.cfg binary record builder and file appender
    ├── crc32.hpp                   # OPL-compatible CRC32 hash
    ├── string_util.hpp             # Title sanitisation and string helpers
    ├── file_util.hpp               # read_exact() and read_block() helpers
    ├── image_loader.hpp / .cpp     # OpenGL texture loader (background image)
    └── stb_image.h                 # Single-header PNG/JPG decoder
```

---

## Dependencies

All runtime dependencies are managed by **vcpkg** and declared in `vcpkg.json`.

| Library | Purpose |
|---------|---------|
| [Dear ImGui](https://github.com/ocornut/imgui) (`glfw-binding`, `opengl3-binding`) | Immediate-mode GUI |
| [GLFW 3](https://www.glfw.org/) | Window creation, OpenGL context, input |
| [GLEW](https://glew.sourceforge.net/) | OpenGL extension loading |
| [stb_image](https://github.com/nothings/stb) | PNG/JPG decoding for the background image (bundled) |

---

## Building with MSVC

### Prerequisites

| Tool | Minimum version | Notes |
|------|-----------------|-------|
| **Visual Studio** | 2022 (v17) | Requires the *Desktop development with C++* workload |
| **MSVC toolset** | v145 | Installed with Visual Studio 2022 |
| **vcpkg** | Latest | For automatic dependency resolution |
| **Windows SDK** | 10.0 | Installed with Visual Studio |

#### Install and integrate vcpkg

```bat
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
C:\vcpkg\bootstrap-vcpkg.bat
C:\vcpkg\vcpkg integrate install
```

> **Tip:** Setting the `VCPKG_ROOT` environment variable to `C:\vcpkg` lets MSBuild find it automatically.

---

### Option A — Visual Studio IDE

1. Open `ISO Splitter.sln` in Visual Studio 2022.
2. On first open, vcpkg will automatically restore the packages declared in `vcpkg.json` (manifest mode).
3. Select the desired configuration from the toolbar — **Release | x64** is recommended for a deployable build.
4. Press **Ctrl+Shift+B** (or **Build → Build Solution**).
5. The executable is written to:
   ```
   x64\Release\ISO Splitter.exe
   ```

---

### Option B — Command Line (MSBuild)

Open a **Developer Command Prompt for VS 2022** (or any prompt where `msbuild` is on `PATH`) and run:

```bat
REM Restore dependencies
vcpkg install --triplet x64-windows

REM Build Release x64
msbuild "ISO Splitter.sln" /p:Configuration=Release /p:Platform=x64 /m
```

Replace `x64` / `Release` with your desired platform and configuration as needed:

| `/p:Platform=` | `/p:Configuration=` | Output path |
|----------------|---------------------|-------------|
| `x64`  | `Release` | `x64\Release\ISO Splitter.exe` |
| `x64`  | `Debug`   | `x64\Debug\ISO Splitter.exe`   |
| `Win32`| `Release` | `Release\ISO Splitter.exe`     |
| `Win32`| `Debug`   | `Debug\ISO Splitter.exe`       |

> **Note:** The Release|x64 configuration links dependencies statically and strips the console window (subsystem = Windows). Debug builds keep the console window for diagnostic output.

---

## License

This project is released into the public domain under the [Unlicense](LICENSE).

---

## OPL Compatibility & Testing Notes

- The ISO parsing logic is implemented to match behavior used by OpenPS2Loader's `iso2opl`/`isofs` tools:
  - Primary Volume Descriptor is read from sector 16 and validated (`CD001` signature and PVD version 1).
  - Root directory entries are scanned for `SYSTEM.CNF`. Both ASCII and Joliet (UCS-2 BE) filename encodings are supported. Joliet names are detected by interpreting the high-byte/low-byte UCS-2 layout and extracting the low bytes, mirroring common libcdvd/OPL heuristics.
  - `SYSTEM.CNF` is parsed for `BOOT` / `BOOT2` lines and for occurrences of the `CDROM0:\` path; the final filename component is returned and sanitized for use in `ul.*` filenames.

- Tests:
  - A set of unit tests was added at `tests/test_iso_util.cpp` that build minimal synthetic ISO files (PVD, root directory sector, and SYSTEM.CNF file contents) and exercise:
    - Normal ASCII `SYSTEM.CNF` and `BOOT`/`BOOT2` parsing
    - Missing `SYSTEM.CNF` detection (throws)
    - Joliet (UCS-2 BE) filename handling
    - Truncated directory records (safely skipped)
    - Oversized `file identifier length` values being capped to avoid OOB reads

- Running tests:
  - From the repository root (with a C++17 compiler):

```bash
g++ -std=c++17 -I. tests/test_iso_util.cpp -o tests/test_iso_util
./tests/test_iso_util
```
