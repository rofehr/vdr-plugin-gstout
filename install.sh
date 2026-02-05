#!/bin/bash
#
# Installation script for VDR GStreamer output plugin
#

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${GREEN}VDR GStreamer Output Plugin Installation${NC}"
echo "=========================================="
echo

# Check root for installation
if [ "$EUID" -ne 0 ] && [ "$1" = "install" ]; then 
    echo -e "${RED}Error: Installation requires root privileges${NC}"
    echo "Please run: sudo $0 install"
    exit 1
fi

# Check dependencies
echo "Checking dependencies..."
MISSING_DEPS=""

if ! pkg-config --exists vdr; then
    MISSING_DEPS="${MISSING_DEPS} vdr-dev"
fi

if ! pkg-config --exists gstreamer-1.0; then
    MISSING_DEPS="${MISSING_DEPS} libgstreamer1.0-dev"
fi

if ! pkg-config --exists gstreamer-app-1.0; then
    MISSING_DEPS="${MISSING_DEPS} libgstreamer-plugins-base1.0-dev"
fi

if [ -n "$MISSING_DEPS" ]; then
    echo -e "${RED}Missing dependencies:${MISSING_DEPS}${NC}"
    echo
    echo "Install with:"
    echo -e "${YELLOW}sudo apt-get install${MISSING_DEPS} gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly${NC}"
    echo
    echo "Optional (for hardware acceleration):"
    echo -e "${YELLOW}sudo apt-get install gstreamer1.0-vaapi${NC}"
    exit 1
fi

echo -e "${GREEN}✓ All dependencies found${NC}"

# Check GStreamer version
GST_VERSION=$(pkg-config --modversion gstreamer-1.0)
echo "GStreamer version: $GST_VERSION"
echo

# Check for VAAPI support
if gst-inspect-1.0 vaapi &>/dev/null; then
    echo -e "${GREEN}✓ VAAPI hardware acceleration available${NC}"
else
    echo -e "${YELLOW}! VAAPI not available (software decoding only)${NC}"
fi
echo

# Compile
if [ "$1" != "install-only" ]; then
    echo "Compiling plugin..."
    if make clean && make; then
        echo -e "${GREEN}✓ Compilation successful${NC}"
    else
        echo -e "${RED}✗ Compilation failed${NC}"
        exit 1
    fi
    echo
fi

# Install
if [ "$1" = "install" ] || [ "$1" = "install-only" ]; then
    echo "Installing plugin..."
    if make install; then
        echo -e "${GREEN}✓ Plugin installed${NC}"
    else
        echo -e "${RED}✗ Installation failed${NC}"
        exit 1
    fi
    echo
    
    echo -e "${GREEN}Installation complete!${NC}"
    echo
    echo "To use the plugin, add to your VDR startup:"
    echo -e "${YELLOW}vdr -P gstout${NC}"
    echo
    echo "Or with custom sinks:"
    echo -e "${YELLOW}vdr -P 'gstout -a pulsesink -v vaapisink'${NC}"
    echo
    echo "Test GStreamer setup:"
    echo -e "${YELLOW}gst-launch-1.0 audiotestsrc ! autoaudiosink${NC}"
    echo -e "${YELLOW}gst-launch-1.0 videotestsrc ! autovideosink${NC}"
    echo
else
    echo "Compilation complete!"
    echo
    echo "To install, run:"
    echo -e "${YELLOW}sudo $0 install${NC}"
    echo
fi
