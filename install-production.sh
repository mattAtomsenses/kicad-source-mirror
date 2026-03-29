#!/bin/bash
# Production Installation Script for KiCad with EasyEDA Pro Export
# Run with: sudo ./install-production.sh

set -e

INSTALL_PREFIX="${INSTALL_PREFIX:-/usr/local}"
BUILD_DIR="${BUILD_DIR:-/opt/kicad-source-mirror/build}"

# Get actual user home directory (even when running with sudo)
REAL_HOME=$(getent passwd "$SUDO_USER" | cut -d: -f6)

echo "=========================================="
echo "KiCad EasyEDA Pro Export - Production Install"
echo "=========================================="
echo "Install Prefix: $INSTALL_PREFIX"
echo "Build Directory: $BUILD_DIR"
echo "User: $SUDO_USER"
echo "Home: $REAL_HOME"
echo ""

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo "Please run with sudo: sudo $0"
    exit 1
fi

# Check if build exists
if [ ! -d "$BUILD_DIR" ]; then
    echo "Error: Build directory not found: $BUILD_DIR"
    echo "Please build KiCad first or set BUILD_DIR environment variable."
    echo ""
    echo "Example:"
    echo "  sudo BUILD_DIR=/home/$SUDO_USER/projects/kicad-source-mirror/build $0"
    exit 1
fi

echo "[1/6] Installing runtime dependencies..."
apt-get update
apt-get install -y --no-install-recommends \
    libwxgtk3.2-1t64 \
    libwxgtk-webview3.2-1t64 \
    libwxgtk-media3.2-1t64 \
    libwxgtk-gl3.2-1t64 \
    libgtk-3-0t64 \
    libcairo2 \
    libgl1-mesa-dri \
    libglu1-mesa \
    libglm-dev \
    libboost-system1.83.0 \
    libboost-locale1.83.0 \
    libboost-thread1.83.0 \
    libboost-chrono1.83.0 \
    libboost-date-time1.83.0 \
    libboost-filesystem1.83.0 \
    libboost-program-options1.83.0 \
    libboost-regex1.83.0 \
    libboost-iostreams1.83.0 \
    libocct-modeling-algorithms-dev \
    libocct-modeling-data-dev \
    libocct-data-exchange-dev \
    libocct-visualization-dev \
    libocct-foundation-dev \
    libocct-ocaf-dev \
    libngspice0 \
    libgit2-1.7 \
    libsecret-1-0 \
    libcurl4t64 \
    libnng1 \
    libprotobuf32t64 \
    libzstd1 \
    zlib1g \
    xdg-utils

echo "[2/6] Installing KiCad using cmake install..."
cd "$BUILD_DIR"
if cmake --install . --prefix "$INSTALL_PREFIX" 2>&1; then
    echo "✓ CMake install succeeded"
else
    echo "⚠ CMake install failed, trying manual installation..."
    echo "[2/6] Installing KiCad binaries manually..."

    # Create directories
    mkdir -p "$INSTALL_PREFIX/bin"
    mkdir -p "$INSTALL_PREFIX/lib"
    mkdir -p "$INSTALL_PREFIX/share/kicad"
    mkdir -p "$INSTALL_PREFIX/share/applications"
    mkdir -p "$INSTALL_PREFIX/share/mime/packages"

    # Copy binaries
    cp -f kicad/kicad "$INSTALL_PREFIX/bin/"
    cp -f kicad/kicad-cli "$INSTALL_PREFIX/bin/"

    # Copy kiface libraries (shared libraries)
    find . -name "*.so*" -type f -exec cp {} "$INSTALL_PREFIX/lib/" \; 2>/dev/null || true
    find . -name "*.kiface" -type f -exec cp {} "$INSTALL_PREFIX/lib/" \; 2>/dev/null || true

    # Copy resources
    cp -rf ../resources/* "$INSTALL_PREFIX/share/kicad/" 2>/dev/null || true

    # Create symlink for pcbnew
    ln -sf "$INSTALL_PREFIX/lib/_pcbnew.kiface" "$INSTALL_PREFIX/lib/kicad/" 2>/dev/null || true
fi

echo "[3/6] Updating library cache..."
ldconfig

echo "[4/6] Creating desktop entry..."
cat > "$INSTALL_PREFIX/share/applications/kicad-easyedapro.desktop" << 'EOF'
[Desktop Entry]
Name=KiCad (EasyEDA Pro Export)
Comment=Electronic Design Automation with EasyEDA Pro Export
Exec=/usr/local/bin/kicad %F
Icon=kicad
Type=Application
Categories=Electronics;Engineering;Development;
MimeType=application/x-kicad-project;application/x-kicad-pcb;application/x-kicad-sch;
StartupNotify=true
StartupWMClass=kicad
EOF

# Also install to system desktop entries
cp -f "$INSTALL_PREFIX/share/applications/kicad-easyedapro.desktop" /usr/share/applications/

echo "[5/6] Setting up library path..."
echo "$INSTALL_PREFIX/lib" > /etc/ld.so.conf.d/kicad.conf
ldconfig

echo "[6/6] Verifying installation..."
if [ -x "$INSTALL_PREFIX/bin/kicad" ]; then
    echo ""
    echo "=========================================="
    echo "✓ Installation Complete!"
    echo "=========================================="
    echo ""
    echo "KiCad installed to: $INSTALL_PREFIX"
    echo ""
    echo "Launch KiCad:"
    echo "  $INSTALL_PREFIX/bin/kicad"
    echo "  or just: kicad"
    echo ""
    echo "Export to EasyEDA Pro:"
    echo "  kicad-cli pcb export easyedapro --output output.epro input.kicad_pcb"
    echo ""
    echo "Desktop entry: Applications → Electronics → KiCad (EasyEDA Pro Export)"
    echo ""
    echo "Installation files:"
    echo "  Binaries: $INSTALL_PREFIX/bin/"
    echo "  Libraries: $INSTALL_PREFIX/lib/"
    echo "  Resources: $INSTALL_PREFIX/share/kicad/"
else
    echo "ERROR: Installation verification failed!"
    echo "kicad binary not found at: $INSTALL_PREFIX/bin/kicad"
    exit 1
fi
