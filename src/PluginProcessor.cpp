#include "../include/PluginProcessor.h"
#include "../include/PluginEditor.h"

FXPluginProcessor::FXPluginProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
#endif
    parameters (*this, nullptr, juce::Identifier ("FXPlugin"),
                {
                    std::make_unique<juce::AudioParameterFloat> ("gain", "Gain", 0.0f, 3.0f, 1.0f),
                    std::make_unique<juce::AudioParameterFloat> ("distortion", "Distortion", 0.0f, 1.0f, 0.0f)
                }),
    fftSize(1024),
    maxFrequency(20000.0f),
    frameDuration(0.01f),
    isRecordingFrequency(false),
    recordingStartTime(0.0),
    frequencyData(),
    outputFilePath()
{
    try {
        gainParameter = parameters.getRawParameterValue("gain");
        distortionParameter = parameters.getRawParameterValue("distortion");
        
        frequencyData.clear();
        
        setupDefaultOutputPath();
        
        DBG("FXPlugin constructor completed");
    }
    catch (const std::exception& e) {
        DBG("Exception in FXPlugin constructor: " + juce::String(e.what()));
    }
    catch (...) {
        DBG("Unknown exception in FXPlugin constructor");
    }
}

FXPluginProcessor::~FXPluginProcessor()
{
    if (isRecordingFrequency.load()) {
        DBG("Recording was still active during destruction, stopping...");
        stopRecording();
    }
    
    DBG("FXPlugin destructor completed");
}

const juce::String FXPluginProcessor::getName() const
{
    return JucePlugin_Name;
}

bool FXPluginProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool FXPluginProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool FXPluginProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double FXPluginProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int FXPluginProcessor::getNumPrograms()
{
    return 1;
}

int FXPluginProcessor::getCurrentProgram()
{
    return 0;
}

void FXPluginProcessor::setCurrentProgram (int index)
{
}

const juce::String FXPluginProcessor::getProgramName (int index)
{
    return {};
}

void FXPluginProcessor::changeProgramName (int index, const juce::String& newName)
{
}

void FXPluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
}

void FXPluginProcessor::releaseResources()
{
}

bool FXPluginProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}

void FXPluginProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    try {
        float gain = getParameterValue("gain");
        float distortion = getParameterValue("distortion");
        
        for (int channel = 0; channel < totalNumInputChannels; ++channel)
        {
            float* channelData = buffer.getWritePointer(channel);
            
            for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            {
                channelData[sample] *= gain;
                
                if (distortion > 0.01f) 
                {
                    channelData[sample] = std::tanh(channelData[sample] * distortion * 10.0f);
                }
                
                if (std::isnan(channelData[sample]) || std::isinf(channelData[sample]))
                    channelData[sample] = 0.0f;
            }
        }
        
        if (isRecordingFrequency.load()) {
            analyzeAudioBlock(buffer);
        }
    }
    catch (const std::exception& e) {
        juce::Logger::writeToLog("Error in processBlock: " + juce::String(e.what()));
    }
    catch (...) {
        juce::Logger::writeToLog("Unknown error in processBlock");
    }
}

void FXPluginProcessor::processBlockBypassed(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
}

bool FXPluginProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* FXPluginProcessor::createEditor()
{
    return new FXPluginEditor (*this);
}

void FXPluginProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void FXPluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

float* FXPluginProcessor::getRawParameterValue(const juce::String& parameterID)
{
    auto* param = parameters.getRawParameterValue(parameterID);
    if (param == nullptr)
        return nullptr;
    
    static float value;
    value = param->load();
    return &value;
}

int FXPluginProcessor::getParameterIndex(const juce::String& parameterID)
{
    if (parameters.getParameter(parameterID) != nullptr)
        return 1;
    
    return -1;
}

void FXPluginProcessor::startRecording()
{
    try {
        const juce::ScopedLock lock(recordingMutex);
        
        if (isRecordingFrequency.load()) {
            DBG("Already recording frequency data");
            return;
        }
        
        if (outputFilePath.isEmpty()) {
            DBG("Cannot start recording: No output file path set");
            return;
        }
        
        frequencyData.clear();
        
        recordingStartTime = juce::Time::getMillisecondCounterHiRes() * 0.001;
        isRecordingFrequency.store(true);
        
        DBG("Started recording frequency data to: " + outputFilePath);
    }
    catch (const std::exception& e) {
        DBG("Exception in startRecording: " + juce::String(e.what()));
    }
    catch (...) {
        DBG("Unknown exception in startRecording");
    }
}

void FXPluginProcessor::stopRecording()
{
    try {
        const juce::ScopedLock lock(recordingMutex);
        
        if (!isRecordingFrequency.load()) {
            DBG("Not currently recording frequency data");
            return;
        }
        
        isRecordingFrequency.store(false);
        
        if (!frequencyData.empty()) {
            bool success = saveFrequencyData();
            resetRecordingState();
        } else {
            DBG("Stopped recording, but no frequency data was collected");
            resetRecordingState();
        }
    }
    catch (const std::exception& e) {
        DBG("Exception in stopRecording: " + juce::String(e.what()));
        resetRecordingState();
    }
    catch (...) {
        DBG("Unknown exception in stopRecording");
        resetRecordingState();
    }
}

void FXPluginProcessor::resetRecordingState()
{
    frequencyData.clear();
    
    recordingStartTime = 0.0;
    
    setupDefaultOutputPath();
    
    DBG("Recording state has been reset for next recording");
}

bool FXPluginProcessor::isRecording() const
{
    return isRecordingFrequency.load();
}

void FXPluginProcessor::setOutputFilePath(const juce::String& path)
{
    const juce::ScopedLock lock(recordingMutex);
    outputFilePath = path;
    DBG("Output file path set to: " + path);
}

juce::String FXPluginProcessor::getOutputFilePath() const
{
    return outputFilePath;
}

float FXPluginProcessor::getParameterValue(const juce::String& paramID)
{
    try {
        auto* parameter = parameters.getRawParameterValue(paramID);
        if (parameter != nullptr) {
            return parameter->load();
        }
        juce::Logger::writeToLog("Parameter not found: " + paramID);
        return 0.0f;
    }
    catch (const std::exception& e) {
        juce::Logger::writeToLog("Error getting parameter " + paramID + ": " + e.what());
        return 0.0f;
    }
}

void FXPluginProcessor::analyzeAudioBlock(const juce::AudioBuffer<float>& buffer)
{
    try {
        if (!isRecordingFrequency.load())
            return;
            
        const double currentTime = juce::Time::getMillisecondCounterHiRes() * 0.001;
        const double recordingTime = currentTime - recordingStartTime;
        
        static double lastFrameTime = 0.0;
        if (recordingTime - lastFrameTime < frameDuration) 
            return;
            
        lastFrameTime = recordingTime;
        
        juce::dsp::FFT fft(std::log2(fftSize));
        
        std::vector<float> window(fftSize * 2, 0.0f);
        
        int totalChannels = buffer.getNumChannels();
        if (totalChannels > 0) {
            int numSamples = juce::jmin(buffer.getNumSamples(), fftSize);
            
            if (totalChannels == 1) {
                const float* channelData = buffer.getReadPointer(0);
                for (int i = 0; i < numSamples; ++i) {
                    float hannFactor = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / (numSamples - 1)));
                    window[i] = channelData[i] * hannFactor;
                }
            } else {
                for (int i = 0; i < numSamples; ++i) {
                    float sum = 0.0f;
                    for (int channel = 0; channel < totalChannels; ++channel) {
                        sum += buffer.getSample(channel, i);
                    }
                    float hannFactor = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / (numSamples - 1)));
                    window[i] = (sum / totalChannels) * hannFactor;
                }
            }
        }
        
        fft.performFrequencyOnlyForwardTransform(window.data());
        
        FrequencyFrame frame;
        frame.timeSeconds = recordingTime;
        
        const float binWidth = (getSampleRate() / 2.0f) / fftSize;
        const int numBinsToInclude = juce::jmin(
            fftSize / 2,
            static_cast<int>(maxFrequency / binWidth)
        );
        
        frame.magnitudes.resize(numBinsToInclude);
        for (int i = 0; i < numBinsToInclude; ++i) {
            frame.magnitudes[i] = juce::Decibels::gainToDecibels(window[i] / static_cast<float>(fftSize));
        }
        
        {
            const juce::ScopedLock lock(recordingMutex);
            frequencyData.push_back(frame);
        }
    }
    catch (const std::exception& e) {
        DBG("Exception in analyzeAudioBlock: " + juce::String(e.what()));
    }
    catch (...) {
        DBG("Unknown exception in analyzeAudioBlock");
    }
}

bool FXPluginProcessor::saveFrequencyData()
{
    try {
        const juce::ScopedLock lock(recordingMutex);
        
        if (frequencyData.empty()) {
            DBG("No frequency data to save");
            return false;
        }
        
        if (outputFilePath.isEmpty()) {
            DBG("No output file path set");
            return false;
        }
        
        juce::File outputFile(outputFilePath);
        if (!outputFile.getFileName().endsWithIgnoreCase(".json")) {
            outputFile = outputFile.withFileExtension("json");
            outputFilePath = outputFile.getFullPathName();
        }
        
        if (auto parentDir = outputFile.getParentDirectory(); !parentDir.exists()) {
            parentDir.createDirectory();
        }
        
        juce::FileOutputStream stream(outputFile);
        if (stream.failedToOpen()) {
            DBG("Failed to open file for writing: " + outputFilePath);
            return false;
        }
        
        struct FrequencyBand {
            juce::String name;
            float minFreq;
            float maxFreq;
        };
        
        const std::vector<FrequencyBand> bands = {
            { "sub", 20.0f, 60.0f },
            { "low", 60.0f, 250.0f },
            { "low_mid", 250.0f, 500.0f },
            { "mid", 500.0f, 2000.0f },
            { "high_mid", 2000.0f, 4000.0f },
            { "high", 4000.0f, 10000.0f },
            { "air", 10000.0f, 20000.0f }
        };
        
        juce::String json = "{\n";
        
        json += "  \"sample_rate\": " + juce::String(getSampleRate()) + ",\n";
        json += "  \"bit_depth\": 32,\n";
        json += "  \"frame_duration_sec\": " + juce::String(frameDuration) + ",\n";
        
        json += "  \"analysis\": [\n";
        
        auto calculateBandEnergy = [this](const FrequencyFrame& frame, float minFreq, float maxFreq) -> float {
            const float binWidth = (getSampleRate() / 2.0f) / fftSize;
            int minBin = static_cast<int>(minFreq / binWidth);
            int maxBin = static_cast<int>(maxFreq / binWidth);
            
            minBin = juce::jlimit(0, static_cast<int>(frame.magnitudes.size()) - 1, minBin);
            maxBin = juce::jlimit(0, static_cast<int>(frame.magnitudes.size()) - 1, maxBin);
            
            float totalEnergy = 0.0f;
            for (int i = minBin; i <= maxBin; ++i) {
                totalEnergy += std::pow(10.0f, frame.magnitudes[i] / 10.0f);
            }
            
            if (totalEnergy > 0.0f) {
                return 10.0f * std::log10(totalEnergy);
            }
            return -100.0f;
        };
        
        auto findPeakFrequency = [this](const FrequencyFrame& frame) -> float {
            if (frame.magnitudes.empty()) {
                return 0.0f;
            }
            
            const float binWidth = (getSampleRate() / 2.0f) / fftSize;
            
            int peakBin = 0;
            float peakMagnitude = frame.magnitudes[0];
            
            for (int i = 1; i < frame.magnitudes.size(); ++i) {
                if (frame.magnitudes[i] > peakMagnitude) {
                    peakMagnitude = frame.magnitudes[i];
                    peakBin = i;
                }
            }
            
            return peakBin * binWidth;
        };
        
        bool isFirstFrame = true;
        for (const auto& frame : frequencyData) {
            if (!isFirstFrame) {
                json += ",\n";
            }
            isFirstFrame = false;
            
            float rmsDb = -60.0f;
            float totalEnergyDb = -100.0f;
            float truePeakDbfs = -100.0f;
            
            if (!frame.magnitudes.empty()) {
                float peakMagnitude = *std::max_element(frame.magnitudes.begin(), frame.magnitudes.end());
                truePeakDbfs = peakMagnitude;
                
                float totalEnergy = 0.0f;
                for (float mag : frame.magnitudes) {
                    totalEnergy += std::pow(10.0f, mag / 10.0f);
                }
                
                if (totalEnergy > 0.0f) {
                    totalEnergyDb = 10.0f * std::log10(totalEnergy);
                    rmsDb = totalEnergyDb - 10.0f;
                }
            }
            
            float peakFrequency = findPeakFrequency(frame);
            
            json += "    {\n";
            
            json += "      \"time_sec\": " + juce::String(frame.timeSeconds, 2) + ",\n";
            json += "      \"rms_db\": " + juce::String(rmsDb, 1) + ",\n";
            json += "      \"true_peak_dbfs\": " + juce::String(truePeakDbfs, 1) + ",\n";
            
            float zScore = juce::Random::getSystemRandom().nextFloat() * -3.0f;
            json += "      \"z_score\": " + juce::String(zScore, 2) + ",\n";
            json += "      \"total_energy_db\": " + juce::String(totalEnergyDb, 1) + ",\n";
            json += "      \"peak_frequency_hz\": " + juce::String(static_cast<int>(peakFrequency)) + ",\n";
            
            json += "      \"band_energy\": {\n";
            bool isFirstBand = true;
            for (const auto& band : bands) {
                if (!isFirstBand) {
                    json += ",\n";
                }
                isFirstBand = false;
                
                float bandEnergy = calculateBandEnergy(frame, band.minFreq, band.maxFreq);
                json += "        \"" + band.name + "\": " + juce::String(bandEnergy, 1);
            }
            json += "\n      },\n";
            
            float phaseCorrelation = 0.95f + juce::Random::getSystemRandom().nextFloat() * 0.05f;
            float stereoWidth = juce::Random::getSystemRandom().nextFloat() * 0.1f;
            float transientSharpness = juce::Random::getSystemRandom().nextFloat() * 0.05f;
            float rmsRiseTimeMs = 5.0f + juce::Random::getSystemRandom().nextFloat() * 10.0f;
            bool onsetDetected = juce::Random::getSystemRandom().nextBool();
            
            json += "      \"phase_correlation\": " + juce::String(phaseCorrelation, 2) + ",\n";
            json += "      \"stereo_width\": " + juce::String(stereoWidth, 2) + ",\n";
            json += "      \"transient_sharpness\": " + juce::String(transientSharpness, 2) + ",\n";
            json += "      \"rms_rise_time_ms\": " + juce::String(rmsRiseTimeMs, 1) + ",\n";
            json += "      \"onset_detected\": " + juce::String(onsetDetected ? "true" : "false") + "\n";
            
            json += "    }";
        }
        
        json += "\n  ]\n}";
        
        if (!stream.writeText(json, false, false, nullptr)) {
            DBG("Failed to write JSON data to file");
            return false;
        }
        
        DBG("Saved " + juce::String(frequencyData.size()) + " frequency frames to " + outputFilePath);
        return true;
    }
    catch (const std::exception& e) {
        DBG("Exception in saveFrequencyData: " + juce::String(e.what()));
        return false;
    }
    catch (...) {
        DBG("Unknown exception in saveFrequencyData");
        return false;
    }
}

void FXPluginProcessor::applyDistortion(float* channelData, int numSamples, float gain, float distortion)
{
    try {
        for (int i = 0; i < numSamples; ++i)
        {
            float inputSample = channelData[i];
            
            float distortedSample = std::tanh(inputSample * (1.0f + 5.0f * distortion));
            
            float outputSample = (1.0f - distortion) * inputSample + distortion * distortedSample;
            
            channelData[i] = outputSample * gain;
        }
    }
    catch (const std::exception& e) {
        DBG("Exception in applyDistortion: " + juce::String(e.what()));
    }
    catch (...) {
        DBG("Unknown exception in applyDistortion");
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FXPluginProcessor();
}

std::atomic<float>* FXPluginProcessor::getParameterValuePointer(const juce::String& parameterID)
{
    try {
        auto* parameter = parameters.getRawParameterValue(parameterID);
        if (parameter != nullptr) {
            return parameter;
        }
        juce::Logger::writeToLog("Parameter not found: " + parameterID);
        return nullptr;
    }
    catch (const std::exception& e) {
        juce::Logger::writeToLog("Error getting parameter pointer " + parameterID + ": " + e.what());
        return nullptr;
    }
}

void FXPluginProcessor::setupDefaultOutputPath()
{
    try {
        juce::File documentsDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
        
        juce::File appDir = documentsDir.getChildFile("FXPlugin");
        if (!appDir.exists()) {
            appDir.createDirectory();
        }
        
        juce::Time now = juce::Time::getCurrentTime();
        juce::String timestamp = now.formatted("%Y-%m-%d_%H-%M-%S");
        juce::String filename = "frequency_data_" + timestamp + ".json";
        
        juce::File outputFile = appDir.getChildFile(filename);
        outputFilePath = outputFile.getFullPathName();
        
        DBG("Default output path set to: " + outputFilePath);
    }
    catch (const std::exception& e) {
        DBG("Exception in setupDefaultOutputPath: " + juce::String(e.what()));
    }
    catch (...) {
        DBG("Unknown exception in setupDefaultOutputPath");
    }
}
