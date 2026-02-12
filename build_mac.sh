#!/bin/bash

# --- 1. SETUP VCPKG ---
# Fonction pour trouver ou installer vcpkg
setup_vcpkg() {
    # 1. Vérifier la variable d'environnement
    if [ -n "$VCPKG_ROOT" ] && [ -d "$VCPKG_ROOT" ]; then
        echo "Using system VCPKG_ROOT: $VCPKG_ROOT"
        return
    fi
    
    # 2. Vérifier dans le dossier home standard
    if [ -d "$HOME/vcpkg" ]; then
        export VCPKG_ROOT="$HOME/vcpkg"
        echo "Found vcpkg at $HOME/vcpkg"
        return
    fi

    # 3. Vérifier localement dans le projet
    if [ -d "./vcpkg" ]; then
        export VCPKG_ROOT="$(pwd)/vcpkg"
        echo "Found local vcpkg at $VCPKG_ROOT"
        return
    fi

    # 4. Si non trouvé, on propose de l'installer
    echo "vcpkg not found. Cloning vcpkg locally..."
    git clone https://github.com/microsoft/vcpkg.git
    ./vcpkg/bootstrap-vcpkg.sh
    export VCPKG_ROOT="$(pwd)/vcpkg"
}

setup_vcpkg

if [ ! -f "$VCPKG_ROOT/vcpkg" ]; then
    echo "Error: vcpkg setup failed."
    exit 1
fi


echo "Building Galaxy CPU project (macOS/Linux)..."

# 2. Build Directory
mkdir -p build

# 3. Configure CMake
# Triplet pour Mac (arm64 ou x64 selon votre machine, ici x64 par défaut ou automatique)
# Pour Apple Silicon, utilisez arm64-osx, pour Intel x64-osx
# On laisse vcpkg deviner ou on force si besoin.
echo "Configuring with CMake..."
cmake -B build -S . \
    -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
    -DCMAKE_BUILD_TYPE=Release

if [ $? -ne 0 ]; then
    echo "CMake Configuration Failed!"
    exit 1
fi

# 4. Build
echo "Building..."
cmake --build build

if [ $? -ne 0 ]; then
    echo "Build Failed!"
    exit 1
fi

echo "Build Success! Executable in build/bin/GalaxyApp"
