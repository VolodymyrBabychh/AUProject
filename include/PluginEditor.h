#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"
#include <atomic>
#include <memory>

//==============================================================================
/**
*/
class FXPluginEditor  : public juce::AudioProcessorEditor,
                        private juce::Slider::Listener,
                        private juce::Button::Listener,
                        private juce::Timer
{
public:
    FXPluginEditor (FXPluginProcessor&);
    ~FXPluginEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // Timer callback for UI updates
    void timerCallback() override;
    
    // Slider callback
    void sliderValueChanged(juce::Slider* slider) override;
    
    // Button callback
    void buttonClicked(juce::Button* button) override;
    
    // Method to safely set parameters
    void safeSetParameter(const juce::String& paramID, float value);
    
    // Method to choose output file path
    bool chooseOutputFilePath();
    
    // Method to toggle recording
    void toggleRecording();

    // Reference to the processor
    FXPluginProcessor& audioProcessor;
    
    // Flag to prevent UI updates during initialization
    std::atomic<bool> uiInitialized{false};
    
    // Time tracking for UI updates
    double lastUIRefreshTime;

    // UI Components
    juce::Label gainLabel;
    juce::Label distortionLabel;
    juce::Slider gainSlider;
    juce::Slider distortionSlider;
    
    // UI Components for frequency analysis
    juce::TextButton recordButton;
    juce::TextButton chooseFileButton;
    juce::TextButton resetButton;
    juce::Label filePathLabel;
    juce::Label statusLabel;
    
    // File chooser (needs to be kept alive during async operation)
    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FXPluginEditor)
}; 