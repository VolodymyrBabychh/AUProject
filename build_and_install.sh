#!/bin/bash

# Script to build and install the FX Plugin

# Create build directory if it doesn't exist
if [ -f build/CMakeCache.txt ]; then
    OLD_SRC=$(grep "^CMAKE_HOME_DIRECTORY" build/CMakeCache.txt | cut -d= -f2)
    if [ "$OLD_SRC" != "$(pwd)" ]; then
        echo "CMake source dir changed ($OLD_SRC → $(pwd)), resetting cache"
        rm -rf build
    fi
fi
mkdir -p build
cd build

# Configure the project with CMake
echo "Configuring project..."
cmake -G Xcode ..

if [ $? -ne 0 ]; then
    echo "Error: CMake configuration failed."
    exit 1
fi

# Build the project
echo "Building project..."
cmake --build . --config Release

if [ $? -ne 0 ]; then
    echo "Error: Build failed."
    exit 1
fi

# Install the component to the user's audio plugins directory
echo "Installing plugin..."
USER_PLUGINS_DIR="$HOME/Library/Audio/Plug-Ins/Components"
mkdir -p "$USER_PLUGINS_DIR"

# The correct path to the component file based on build output
COMPONENT_PATH="./VST3/AU/FX Plugin.component"

if [ -d "$COMPONENT_PATH" ]; then
    echo "Installing $COMPONENT_PATH to $USER_PLUGINS_DIR..."
    cp -R "$COMPONENT_PATH" "$USER_PLUGINS_DIR"
    
    if [ $? -ne 0 ]; then
        echo "Error: Failed to copy plugin to $USER_PLUGINS_DIR"
        exit 1
    fi
else
    echo "Error: Could not find built component file at $COMPONENT_PATH"
    echo "Available files in VST3/AU directory:"
    ls -la ./VST3/AU/
    exit 1
fi

echo "Successfully built and installed FX Plugin."
echo "Please restart Logic Pro to use the plugin."
echo "The plugin should appear in Audio Units > YourCompany > FX Plugin" 