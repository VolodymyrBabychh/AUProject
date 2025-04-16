# Setting up JUCE for FX Plugin

This document explains how to set up JUCE for building the FX Plugin.

## Option 1: Using Git Submodule (Recommended)

1. Initialize a Git repository if you haven't already:
```bash
git init
```

2. Add JUCE as a submodule:
```bash
git submodule add https://github.com/juce-framework/JUCE.git
```

3. Update the submodule:
```bash
git submodule update --init --recursive
```

4. Uncomment the following line in CMakeLists.txt:
```cmake
add_subdirectory(JUCE)
```

## Option 2: Manual JUCE Installation

1. Download JUCE from the official website: https://juce.com/get-juce/download

2. Extract JUCE to a location on your drive

3. Edit CMakeLists.txt to point to your JUCE installation:
```cmake
# Replace the line
# add_subdirectory(JUCE)
# with:
set(JUCE_DIR "/path/to/your/JUCE")
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${JUCE_DIR})
find_package(JUCE CONFIG REQUIRED)
```

## Building with Xcode

After setting up JUCE, you can build the project with Xcode:

1. Create a build directory and run CMake:
```bash
mkdir build
cd build
cmake -G Xcode ..
```

2. Open the generated Xcode project:
```bash
open FXPlugin.xcodeproj
```

3. Build the project in Xcode

4. The built AU component will be available in the build folder

## Installing the Plugin

Copy the built FXPlugin.component file to:
```
~/Library/Audio/Plug-Ins/Components/
```

or

```
/Library/Audio/Plug-Ins/Components/
```

Then restart Logic Pro to see your plugin in the Audio Units list. 