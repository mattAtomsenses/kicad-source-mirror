# Production Deployment: EasyEDA Pro Export for KiCad

## Overview

This document covers deploying KiCad with EasyEDA Pro export functionality to Ubuntu servers in production.

---

## Table of Contents

1. [Deployment Options](#deployment-options)
2. [Option A: Headless Export Server](#option-a-headless-export-server)
3. [Option B: Full GUI Server with Remote Desktop](#option-b-full-gui-server-with-remote-desktop)
4. [Option C: Native System Installation](#option-c-native-system-installation)
5. [Option D: Docker Container Deployment](#option-d-docker-container-deployment)
6. [Verification](#verification)
7. [Troubleshooting](#troubleshooting)

---

## Deployment Options

| Option | Use Case | Complexity | GUI Access |
|--------|----------|------------|------------|
| A. Headless Export Server | Automated exports, CI/CD | Low | CLI only |
| B. Remote Desktop Server | Interactive design work | Medium | VNC/RDP |
| C. Native Installation | Production workstation | Low | Native |
| D. Docker Container | Isolated environment | Medium | Via X11/VNC |

---

## Option A: Headless Export Server (CLI-only)

**Best for:** Automated batch exports, CI/CD pipelines, API integration

### Prerequisites

```bash
# Ubuntu 24.04 LTS
sudo apt update
sudo apt install -y build-essential cmake ninja-build git gcc g++
```

### Build Dependencies

```bash
# Install all KiCad build dependencies
sudo apt install -y \
    cmake ninja-build build-essential pkg-config git \
    libcairo2-dev libglu1-mesa-dev libgl1-mesa-dev libx11-dev mesa-common-dev \
    libgtk-3-dev libglm-dev libwxgtk3.2-dev libwxgtk-webview3.2-dev \
    python3-wxgtk4.0 libbz2-dev libssl-dev libzstd-dev zlib1g-dev \
    python3-dev swig libocct-modeling-algorithms-dev libocct-modeling-data-dev \
    libocct-data-exchange-dev libocct-visualization-dev libocct-foundation-dev \
    libocct-ocaf-dev libgit2-dev libsecret-1-dev libboost-all-dev \
    libcurl4-openssl-dev libngspice0-dev unixodbc-dev libspnav-dev \
    libnng-dev libprotobuf-dev protobuf-compiler libpoppler-cpp-dev \
    libpoppler-glib-dev shared-mime-info libzint-dev ccache
```

### Build KiCad

```bash
# Clone repository (if not already done)
cd /opt
sudo git clone https://github.com/KiCad/kicad-source-mirror.git kicad
cd kicad

# Copy your EasyEDA Pro export files to source tree
# (From your development build)

# Build
mkdir -p build && cd build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
ninja -j$(nproc)

# Install system-wide
sudo cmake --install . --prefix /usr/local
sudo ldconfig
```

### Use CLI Export

```bash
# Export PCB to EasyEDA Pro format
kicad-cli pcb export easyedapro --output /path/to/output.epro /path/to/input.kicad_pcb

# Export with custom options
kicad-cli pcb export easyedapro \
    --output /exports/design.epro \
    --include-footprints \
    --include-zones \
    /workspace/design.kicad_pcb
```

### Create Export Service

```bash
# /etc/systemd/system/kicad-export.service
[Unit]
Description=KiCad EasyEDA Pro Export Service
After=network.target

[Service]
Type=simple
User=kicad
WorkingDirectory=/exports
ExecStart=/usr/local/bin/kicad-cli server --port 8080
Restart=always

[Install]
WantedBy=multi-user.target
```

---

## Option B: Full GUI Server with Remote Desktop

**Best for:** Interactive design, remote teams, cloud workstations

### B1: Using VNC Server

```bash
# Install desktop environment and VNC
sudo apt install -y xfce4 xfce4-goodies tightvncserver

# Set up VNC password
vncserver :1
# Enter password when prompted

# Kill and configure
vncserver -kill :1
cat > ~/.vnc/xstartup << 'EOF'
#!/bin/bash
unset SESSION_MANAGER
unset DBUS_SESSION_BUS_ADDRESS
startxfce4 &
EOF
chmod +x ~/.vnc/xstartup

# Start VNC server
vncserver :1 -geometry 1920x1080 -depth 24

# Connect from client: <server-ip>:5901
```

### B2: Using xRDP (Remote Desktop Protocol)

```bash
# Install xRDP
sudo apt install -y xrdp
sudo systemctl enable xrdp
sudo systemctl start xrdp

# Allow through firewall
sudo ufw allow 3389/tcp

# Create user for KiCad
sudo adduser kicad
sudo usermod -aG sudo kicad

# Install KiCad (see Option C)
# Connect from Windows: mstsc.exe -> server-ip:3389
```

---

## Option C: Native System Installation

**Best for:** Production workstations, standard deployment

### Installation Script

```bash
#!/bin/bash
# install-kicad-easyedapro.sh

set -e

echo "Installing KiCad with EasyEDA Pro Export..."

# 1. Install runtime dependencies
sudo apt update
sudo apt install -y \
    libwxgtk3.2-1 \
    libwxgtk-webview3.2-1 \
    libwxgtk-media3.2-1 \
    libgtk-3-0t64 \
    libcairo2 \
    libgl1-mesa-dri \
    libglu1-mesa \
    libglm-dev \
    libboost-system1.83.0 \
    libboost-locale1.83.0 \
    libboost-thread1.83.0 \
    libboost-chrono1.83.0 \
    libocct-modeling-algorithms-dev \
    libocct-modeling-data-dev \
    libocct-data-exchange-dev \
    libocct-visualization-dev \
    libocct-foundation-dev \
    libocct-ocaf-dev \
    libngspice0 \
    libgit2-1.8 \
    libsecret-1-0 \
    libcurl4 \
    libnng1 \
    libprotobuf33 \
    libzstd1 \
    libssl3 \
    zlib1g

# 2. Copy build artifacts (assuming build exists)
BUILD_DIR="/opt/kicad-source-mirror/build"
INSTALL_PREFIX="/usr/local"

# 3. Install binaries
sudo cmake -DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX -P $BUILD_DIR
sudo cmake --install $BUILD_DIR --prefix $INSTALL_PREFIX

# 4. Update library cache
sudo ldconfig

# 5. Create desktop entry
cat << EOF | sudo tee /usr/share/applications/kicad-easyedapro.desktop
[Desktop Entry]
Name=KiCad (EasyEDA Pro Export)
Comment=Electronics Design Automation
Exec=/usr/local/bin/kicad %F
Icon=kicad
Type=Application
Categories=Electronics;Engineering;
MimeType=application/x-kicad-project;
EOF

# 6. Verify installation
echo "Installation complete!"
kicad --version
```

### Usage

```bash
# Launch KiCad
kicad

# Or open a project
kicad /path/to/project.kicad_pro

# Export via CLI
kicad-cli pcb export easyedapro --output output.epro input.kicad_pcb
```

---

## Option D: Docker Container Deployment

**Best for:** Isolated environments, multiple versions, cloud deployment

### D1: Headless Container

```dockerfile
# Dockerfile.headless
FROM ubuntu:24.04

RUN apt-get update && apt-get install -y \
    kicad kicad-libraries \
    libwxgtk3.2-1 libglm-dev libboost-system1.83.0 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace
VOLUME ["/workspace"]

ENTRYPOINT ["kicad-cli"]
CMD ["--help"]
```

```bash
# Build
docker build -f Dockerfile.headless -t kicad-headless .

# Run export
docker run --rm -v /path/to/projects:/workspace \
    kicad-headless pcb export easyedapro \
    --output /workspace/output.epro \
    /workspace/input.kicad_pcb
```

### D2: GUI Container with VNC

```dockerfile
# Dockerfile.vnc
FROM ubuntu:24.04

RUN apt-get update && apt-get install -y \
    xfce4 xfce4-goodies tightvncserver \
    kicad kicad-libraries \
    && rm -rf /var/lib/apt/lists/*

EXPOSE 5901

WORKDIR /workspace
VOLUME ["/workspace"]

CMD vncserver :1 -geometry 1920x1080 -depth 24 && \
    tail -f ~/.vnc/*.log
```

```bash
# Build and run
docker build -f Dockerfile.vnc -t kicad-vnc .
docker run -d -p 5901:5901 -v /path/to/projects:/workspace kicad-vnc
# Connect with VNC client to localhost:5901
```

### Docker Compose Production

```yaml
# docker-compose.prod.yml
version: '3.8'

services:
  kicad:
    build:
      context: .
      dockerfile: Dockerfile.headless
    volumes:
      - ./projects:/workspace
      - ./exports:/exports
    environment:
      - DISPLAY=:0
    command: pcb export easyedapro --output /exports/output.epro /workspace/input.kicad_pcb

  # Optional: Web interface
  web:
    image: nginx:alpine
    ports:
      - "80:80"
    volumes:
      - ./exports:/usr/share/nginx/html:ro
```

---

## Package Creation

### Create Debian Package

```bash
cd /opt/kicad-source-mirror/build
cpack -G DEB
# Generates: kicad-easyedapro_<version>_amd64.deb

# Install on any Ubuntu machine
sudo dpkg -i kicad-easyedapro_<version>_amd64.deb
```

### Create Tarball Package

```bash
cd /opt/kicad-source-mirror/build
mkdir -p package/{bin,lib,share}
cp -r kicad pcbnew eeschema package/bin/
cp -r *.so* package/lib/
cp -r ../resources package/share/
tar czf kicad-easyedapro-portable.tar.gz package/

# Extract and run anywhere
tar xzf kicad-easyedapro-portable.tar.gz
cd package/bin
./kicad
```

---

## Verification

### Check EasyEDA Pro Export Availability

```bash
# Check if plugin is loaded
kicad-cli pcb export --help | grep -i easyeda

# Or from Python
python3 << 'EOF'
import pcbnew
plugin_manager = pcbnew.IO_MGR.PluginFind()
print("Available plugins:", plugin_manager)
EOF
```

### Test Export

```bash
# Create test PCB and export
cat > test_export.kicad_pcb << 'EOF'
(kicad_pcb (version 20231001) (generator "pcbnew")
  (general
    (thickness 1.6)
    (depth 0)
  )
  (setup
    (stackup
      (layer "F.Cu" (signal)
        (thickness 0.035)
      )
      (layer "B.Cu" (signal)
        (thickness 0.035)
      )
    )
  )
)
EOF

kicad-cli pcb export easyedapro test_export.epro test_export.kicad_pcb
ls -la test_export.epro
```

---

## Troubleshooting

### Library Not Found Errors

```bash
# Check library paths
ldd /usr/local/bin/kicad | grep "not found"

# Add to library path if needed
echo "/usr/local/lib" | sudo tee /etc/ld.so.conf.d/kicad.conf
sudo ldconfig
```

### Permission Issues

```bash
# Add user to dialout group for USB devices
sudo usermod -aG dialout $USER

# Fix library permissions
sudo chmod +x /usr/local/lib/kicad/
```

### GUI Not Displaying

```bash
# Check DISPLAY variable
echo $DISPLAY

# For local X11
export DISPLAY=:0

# For remote X11
export DISPLAY=<remote-ip>:0

# For VNC
export DISPLAY=:1
```

### Export Fails

```bash
# Enable debug logging
kicad-cli pcb export easyedapro \
    --verbose \
    --log-file /tmp/export.log \
    --output output.epro \
    input.kicad_pcb

# Check logs
cat /tmp/export.log
```

---

## Maintenance

### Update KiCad

```bash
cd /opt/kicad-source-mirror
git pull
cd build
ninja
sudo cmake --install . --prefix /usr/local
sudo ldconfig
```

### Backup Configuration

```bash
# Backup user settings
tar czf kicad-backup-$(date +%Y%m%d).tar.gz \
    ~/.config/kicad \
    ~/.local/share/kicad
```

---

## Support

- **Issues:** Report to https://github.com/KiCad/kicad-source-mirror/issues
- **Documentation:** https://docs.kicad.org/
- **EasyEDA Format:** https://docs.easyeda.com/
