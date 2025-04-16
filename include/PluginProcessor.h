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
#include "JucePlugin_Common.h"
#include <mutex>
#include <atomic>
#include <vector>
#include <map>

//==============================================================================
/**
*/
class FXPluginProcessor : public juce::AudioProcessor
{
public:
    // Frequency data storage
    struct FrequencyFrame {
        double timeSeconds;
        std::vector<float> magnitudes;
    };
    
    //==============================================================================
    FXPluginProcessor();
    ~FXPluginProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;
    
    //==============================================================================
    // Parameter access methods
    float* getRawParameterValue(const juce::String& parameterID);
    std::atomic<float>* getParameterValuePointer(const juce::String& parameterID);
    int getParameterIndex(const juce::String& parameterID);
    float getParameterValue(const juce::String& parameterID);
    juce::AudioProcessorValueTreeState& getParameterTree() { return parameters; }
    
    //==============================================================================
    // Frequency analysis recording methods
    void startRecording();
    void stopRecording();
    bool isRecording() const;
    void resetRecordingState();
    void setOutputFilePath(const juce::String& path);
    juce::String getOutputFilePath() const;
    bool saveFrequencyData();
    
    // File path setup
    void setupDefaultOutputPath();

private:
    // Core audio processing methods
    void applyDistortion(float* channelData, int numSamples, float gain, float distortion);
    
    // FFT and frequency analysis methods
    void analyzeAudioBlock(const juce::AudioBuffer<float>& buffer);
    
    // Parameter management
    juce::AudioProcessorValueTreeState parameters;
    std::atomic<float>* gainParameter = nullptr;
    std::atomic<float>* distortionParameter = nullptr;
    
    // Frequency analysis
    int fftSize;
    float maxFrequency;
    double frameDuration;
    std::atomic<bool> isRecordingFrequency;
    double recordingStartTime;
    juce::String outputFilePath;
    juce::CriticalSection recordingMutex;
    
    // FFT processing storage
    std::vector<FrequencyFrame> frequencyData;
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FXPluginProcessor)
}; 
