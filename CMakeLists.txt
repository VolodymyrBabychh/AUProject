cmake_minimum_required(VERSION 3.15)
project(FXPlugin VERSION 1.0.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add JUCE as a submodule (you'll need to add this)
add_subdirectory(JUCE)

# Set plugin information
juce_add_plugin(FXPlugin
    PLUGIN_MANUFACTURER_CODE Vbab
    PLUGIN_CODE Fxpl
    FORMATS AU
    PRODUCT_NAME "FX Plugin")

# Source files
target_sources(FXPlugin PRIVATE
    src/PluginProcessor.cpp
    src/PluginEditor.cpp)

# Include directories
target_include_directories(FXPlugin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Use necessary JUCE modules
target_link_libraries(FXPlugin PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_audio_plugin_client
    juce::juce_dsp)

# Set binary output directories
set_target_properties(FXPlugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/VST3"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/VST3") 