# KiCad EDA - Source Repository

## Overview

This is the KiCad EDA (Electronic Design Automation) source code repository. KiCad is a large-scale, open-source C++ desktop application for electronic schematic capture and PCB layout design.

## Important Note

KiCad is a native desktop C++ application that requires compilation with CMake, wxWidgets, OpenCASCADE, OpenGL, and many system libraries. It **cannot be compiled or run directly** in the Replit sandbox environment due to these heavy system dependencies and GUI requirements.

## What's Running

A lightweight Python web server (`web/app.py`) serves a project information page (`web/index.html`) that describes the KiCad project, its components, tech stack, repository structure, and build instructions.

## Tech Stack (KiCad)

- **Language**: C++17/20
- **Build System**: CMake + Ninja
- **GUI Framework**: wxWidgets 3.2+
- **Graphics**: OpenGL / Cairo (via KiCad's GAL abstraction layer)
- **Scripting**: Python (via SWIG bindings)
- **IPC**: Protocol Buffers (Protobuf) + nng
- **3D Models**: OpenCASCADE (STEP/IGES)
- **Package Management**: vcpkg

## Web Server (Replit)

- **File**: `web/app.py` (dev) / `web/wsgi.py` (production via gunicorn)
- **Port**: 5000
- **Host**: 0.0.0.0
- **Deployment**: Autoscale via gunicorn

## Project Structure

```
3d-viewer/       - 3D board visualization
api/             - Protobuf API definitions
common/          - KiCommon shared library
eeschema/        - Schematic editor
pcbnew/          - PCB layout editor
gerbview/        - Gerber file viewer
cvpcb/           - Component-footprint association
kicad/           - Project manager (main entry point)
libs/            - kimath, kiplatform libraries
plugins/         - 3D model loader plugins
qa/              - Unit/integration tests
thirdparty/      - Bundled external deps
resources/       - Icons, bitmaps, packaging
tools/           - Developer utilities
demos/           - Example projects
web/             - Replit web info page (added for Replit environment)
```

## Building KiCad (Outside Replit)

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt install -y cmake ninja-build build-essential \
    libwxgtk3.2-dev libboost-all-dev libcairo2-dev \
    libgl1-mesa-dev libocct-modeling-algorithms-dev \
    python3-dev swig libprotobuf-dev protobuf-compiler \
    libnng-dev libcurl4-openssl-dev libgit2-dev

# Build
mkdir build && cd build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
ninja -j$(nproc)
sudo cmake --install . --prefix /usr/local
```
