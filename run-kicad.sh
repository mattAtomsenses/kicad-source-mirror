#!/bin/bash
# Script to run KiCad in Docker with X11 forwarding

echo "Setting up X11 forwarding..."
# Allow X11 connections
xhost +local:docker 2>/dev/null

# Run KiCad in the existing container with X11 support
docker exec -it -e DISPLAY=$DISPLAY \
    -e LD_LIBRARY_PATH=/workspace/build/pcbnew:/workspace/build/common:/workspace/build/3d-viewer/3d_cache/sg:/workspace/build/eeschema \
    -v /tmp/.X11-unix:/tmp/.X11-unix:rw \
    kicad-build \
    bash -c "cd /workspace/build && ./kicad/kicad"
