#!/bin/bash

if [ -f "build/bin/GalaxyApp" ]; then
    echo "Starting GalaxyApp..."
    # cd to the directory to ensure relative paths work if needed
    cd build/bin
    ./GalaxyApp
else
    echo "Error: Application not found at build/bin/GalaxyApp"
    echo "Please run ./build_mac.sh first."
    exit 1
fi
