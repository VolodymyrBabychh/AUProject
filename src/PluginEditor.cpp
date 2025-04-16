#include "../include/PluginProcessor.h"
#include "../include/PluginEditor.h"

//==============================================================================
FXPluginEditor::FXPluginEditor(FXPluginProcessor& p)
    : juce::AudioProcessorEditor(static_cast<juce::AudioProcessor*>(&p)), audioProcessor(p), uiInitialized(false),
      lastUIRefreshTime(juce::Time::getMillisecondCounterHiRes())
{
    try {
        // Set up the sliders
        gainSlider.setSliderStyle(juce::Slider::SliderStyle::LinearVertical);
        gainSlider.setRange(0.0, 1.0, 0.01);
        gainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 90, 20);
        gainSlider.setPopupDisplayEnabled(true, false, this);
        gainSlider.setTextValueSuffix(" Gain");
        gainSlider.addListener(this);
        addAndMakeVisible(gainSlider);
        
        distortionSlider.setSliderStyle(juce::Slider::SliderStyle::LinearVertical);
        distortionSlider.setRange(0.0, 1.0, 0.01);
        distortionSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 90, 20);
        distortionSlider.setPopupDisplayEnabled(true, false, this);
        distortionSlider.setTextValueSuffix(" Distortion");
        distortionSlider.addListener(this);
        addAndMakeVisible(distortionSlider);
        
        // Set up the record button
        recordButton.setButtonText("Start Recording");
        recordButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkred);
        recordButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        recordButton.addListener(this);
        addAndMakeVisible(recordButton);
        
        // Set up the file path button
        chooseFileButton.setButtonText("Choose Output Path");
        chooseFileButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkblue);
        chooseFileButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        chooseFileButton.addListener(this);
        addAndMakeVisible(chooseFileButton);
        
        // Set up the reset button
        resetButton.setButtonText("Reset");
        resetButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkgrey);
        resetButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        resetButton.addListener(this);
        addAndMakeVisible(resetButton);
        
        // Set up labels
        gainLabel.setText("Gain", juce::dontSendNotification);
        gainLabel.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(gainLabel);
        
        distortionLabel.setText("Distortion", juce::dontSendNotification);
        distortionLabel.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(distortionLabel);
        
        filePathLabel.setText("No output file selected", juce::dontSendNotification);
        auto options = juce::FontOptions().withHeight(12.0f);
        filePathLabel.setFont(juce::Font(options));
        addAndMakeVisible(filePathLabel);
        
        statusLabel.setText("Ready", juce::dontSendNotification);
        auto boldOptions = juce::FontOptions().withHeight(12.0f).withStyle("bold");
        statusLabel.setFont(juce::Font(boldOptions));
        statusLabel.setColour(juce::Label::textColourId, juce::Colours::red);
        addAndMakeVisible(statusLabel);
        
        // Initialize sliders with current parameter values (without notification)
        // Get parameter indexes
        float gainValue = 0.5f;
        float distortionValue = 0.0f;
        
        try {
            std::atomic<float>* gainParam = audioProcessor.getParameterValuePointer("gain");
            if (gainParam != nullptr) {
                gainValue = gainParam->load();
            }
            
            std::atomic<float>* distortionParam = audioProcessor.getParameterValuePointer("distortion");
            if (distortionParam != nullptr) {
                distortionValue = distortionParam->load();
            }
        } catch (const std::exception& e) {
            juce::Logger::writeToLog("Exception in parameter initialization: " + juce::String(e.what()));
        }
        
        gainSlider.setValue(gainValue, juce::dontSendNotification);
        distortionSlider.setValue(distortionValue, juce::dontSendNotification);
        
        // Initialize with current file path
        juce::String outputFilePath = audioProcessor.getOutputFilePath();
        if (!outputFilePath.isEmpty()) {
            // Show a shorter version of the path if it's too long
            if (outputFilePath.length() > 40) {
                juce::File file(outputFilePath);
                juce::String filename = file.getFileName();
                juce::String directory = file.getParentDirectory().getFileName();
                filePathLabel.setText("Output: ..." + directory + "/" + filename, juce::dontSendNotification);
            } else {
                filePathLabel.setText("Output: " + outputFilePath, juce::dontSendNotification);
            }
            statusLabel.setText("Default output file ready", juce::dontSendNotification);
        }
        
        // Set the plugin window size
        setSize (400, 300);
        
        // UI is now initialized, start timer for updates
        uiInitialized.store(true);
        startTimerHz(5); // Update UI at 5Hz instead of 30Hz to reduce CPU usage
    } catch (const std::exception& e) {
        juce::Logger::writeToLog("Exception in FXPluginEditor constructor: " + juce::String(e.what()));
    }
}

FXPluginEditor::~FXPluginEditor()
{
    try {
        // Prevent UI updates during destruction
        uiInitialized.store(false);
        
        // Stop timer
        stopTimer();
        
        // Stop recording if active
        if (audioProcessor.isRecording()) {
            audioProcessor.stopRecording();
        }
    } catch (const std::exception& e) {
        juce::Logger::writeToLog("Exception in FXPluginEditor destructor: " + juce::String(e.what()));
    }
}

//==============================================================================
void FXPluginEditor::paint(juce::Graphics& g)
{
    try {
        // Clear the background
        g.fillAll (juce::Colours::darkgrey);
        
        // Set up the drawing context
        g.setColour (juce::Colours::white);
        g.setFont (15.0f);
        
        // Draw labels
        g.drawFittedText ("Gain", 30, 20, 90, 20, juce::Justification::centred, 1);
        g.drawFittedText ("Distortion", 130, 20, 90, 20, juce::Justification::centred, 1);
        g.drawFittedText ("Frequency Recording", 20, 160, 360, 20, juce::Justification::centred, 1);
    }
    catch (const std::exception& e) {
        juce::Logger::writeToLog("Exception in paint: " + juce::String(e.what()));
    }
}

void FXPluginEditor::resized()
{
    try {
        // This is generally called when the editor is resized.
        // Set the position and size of the slider UI component
        auto area = getLocalBounds();
        auto topSection = area.removeFromTop(40);
        auto pathArea = area.removeFromTop(30);
        auto statusArea = area.removeFromTop(30);
        auto sliderArea = area;
        
        // Layout buttons in top section
        recordButton.setBounds(topSection.removeFromLeft(100).reduced(5));
        chooseFileButton.setBounds(topSection.removeFromLeft(120).reduced(5));
        resetButton.setBounds(topSection.removeFromLeft(80).reduced(5));
        
        // Layout labels
        filePathLabel.setBounds(pathArea.reduced(5));
        statusLabel.setBounds(statusArea.reduced(5));
        
        // Layout sliders
        gainSlider.setBounds(sliderArea.removeFromLeft(sliderArea.getWidth() / 2).reduced(10));
        distortionSlider.setBounds(sliderArea.reduced(10));
    }
    catch (const std::exception& e) {
        juce::Logger::writeToLog("Exception in resized: " + juce::String(e.what()));
    }
}

void FXPluginEditor::timerCallback()
{
    try {
        if (!uiInitialized.load())
            return;
            
        auto* processor = dynamic_cast<FXPluginProcessor*>(&audioProcessor);
        if (processor) {
            // Update recording status if active
            if (processor->isRecording()) {
                static int dotCount = 0;
                juce::String dots;
                for (int i = 0; i < (dotCount % 4); ++i)
                    dots += ".";
                    
                statusLabel.setText("Recording" + dots, juce::dontSendNotification);
                dotCount++;
            }
            
            // Update sliders with current parameter values
            auto* gainParam = processor->getParameterValuePointer("gain");
            auto* distortionParam = processor->getParameterValuePointer("distortion");
            
            if (gainParam) {
                gainSlider.setValue(*gainParam, juce::dontSendNotification);
            }
            
            if (distortionParam) {
                distortionSlider.setValue(*distortionParam, juce::dontSendNotification);
            }
        }
    } catch (const std::exception& e) {
        juce::Logger::writeToLog("Exception in timerCallback: " + juce::String(e.what()));
    }
}

void FXPluginEditor::sliderValueChanged(juce::Slider* slider)
{
    try {
        if (!uiInitialized.load())
            return;
        
        if (slider == &gainSlider)
            safeSetParameter("gain", static_cast<float>(gainSlider.getValue()));
        else if (slider == &distortionSlider)
            safeSetParameter("distortion", static_cast<float>(distortionSlider.getValue()));
    }
    catch (const std::exception& e) {
        juce::Logger::writeToLog("Exception in sliderValueChanged: " + juce::String(e.what()));
    }
}

void FXPluginEditor::safeSetParameter(const juce::String& paramID, float value)
{
    try {
        // Get the parameter from the processor
        auto& params = audioProcessor.getParameterTree();
        auto* param = params.getParameter(paramID);
        
        if (param != nullptr) {
            param->setValueNotifyingHost(param->convertTo0to1(value));
        }
        else {
            juce::Logger::writeToLog("Parameter not found: " + paramID);
        }
    }
    catch (const std::exception& e) {
        juce::Logger::writeToLog("Error setting parameter: " + juce::String(e.what()));
    }
}

void FXPluginEditor::buttonClicked(juce::Button* button)
{
    try {
        if (!uiInitialized.load())
            return;
            
        if (button == &recordButton)
        {
            auto* processor = getAudioProcessor();
            auto* fxProcessor = dynamic_cast<FXPluginProcessor*>(processor);
            
            if (fxProcessor != nullptr)
            {
                if (fxProcessor->isRecording())
                {
                    // Stop recording
                    fxProcessor->stopRecording();
                    statusLabel.setText("Recording stopped", juce::dontSendNotification);
                    recordButton.setButtonText("Start Recording");
                }
                else
                {
                    // Check if we have an output path (should always be true now with default path)
                    if (fxProcessor->getOutputFilePath().isEmpty())
                    {
                        // If somehow the path is empty, create a new default path
                        fxProcessor->setupDefaultOutputPath();
                        
                        // Update the display
                        juce::String outputFilePath = fxProcessor->getOutputFilePath();
                        if (!outputFilePath.isEmpty()) {
                            juce::File file(outputFilePath);
                            juce::String filename = file.getFileName();
                            juce::String directory = file.getParentDirectory().getFileName();
                            filePathLabel.setText("Output: ..." + directory + "/" + filename, juce::dontSendNotification);
                        }
                    }
                    
                    // Start recording
                    fxProcessor->startRecording();
                    statusLabel.setText("Recording frequency data...", juce::dontSendNotification);
                    recordButton.setButtonText("Stop Recording");
                }
            }
        }
        else if (button == &chooseFileButton)
        {
            chooseOutputFilePath();
        }
        else if (button == &resetButton)
        {
            auto* processor = getAudioProcessor();
            auto* fxProcessor = dynamic_cast<FXPluginProcessor*>(processor);
            
            if (fxProcessor != nullptr)
            {
                // Stop recording if active
                if (fxProcessor->isRecording()) {
                    fxProcessor->stopRecording();
                    recordButton.setButtonText("Start Recording");
                } else {
                    // Just reset state if not recording
                    fxProcessor->resetRecordingState();
                }
                
                // Update the UI with the new file path
                juce::String outputFilePath = fxProcessor->getOutputFilePath();
                if (!outputFilePath.isEmpty()) {
                    juce::File file(outputFilePath);
                    juce::String filename = file.getFileName();
                    juce::String directory = file.getParentDirectory().getFileName();
                    filePathLabel.setText("Output: ..." + directory + "/" + filename, juce::dontSendNotification);
                }
                
                statusLabel.setText("Recording state reset, ready for new recording", juce::dontSendNotification);
            }
        }
    }
    catch (const std::exception& e) {
        juce::Logger::writeToLog("Exception in buttonClicked: " + juce::String(e.what()));
    }
}

bool FXPluginEditor::chooseOutputFilePath()
{
    try {
        auto* processor = getAudioProcessor();
        auto* fxProcessor = dynamic_cast<FXPluginProcessor*>(processor);
        
        if (fxProcessor == nullptr)
            return false;
            
        // Store a pointer to the file chooser so it stays alive during the modal dialog
        fileChooser = std::make_unique<juce::FileChooser>(
            "Select output file for frequency data",
            juce::File::getSpecialLocation(juce::File::userDesktopDirectory),
            "*.csv",
            true
        );
        
        auto fileChooserFlags = 
            juce::FileBrowserComponent::saveMode | 
            juce::FileBrowserComponent::canSelectFiles |
            juce::FileBrowserComponent::warnAboutOverwriting;
            
        fileChooser->launchAsync(fileChooserFlags, [this, fxProcessor](const juce::FileChooser& fc) {
            if (fc.getResults().size() > 0) {
                juce::File file = fc.getResult();
                
                // Make sure the file has .csv extension
                if (!file.getFileName().endsWithIgnoreCase(".csv")) {
                    file = file.withFileExtension("csv");
                }
                
                fxProcessor->setOutputFilePath(file.getFullPathName());
                filePathLabel.setText("Output: " + file.getFullPathName(), juce::dontSendNotification);
                statusLabel.setText("Output path selected", juce::dontSendNotification);
            }
        });
        
        return true;
    }
    catch (const std::exception& e) {
        juce::Logger::writeToLog("Exception in chooseOutputFilePath: " + juce::String(e.what()));
        statusLabel.setText("Error choosing file: " + juce::String(e.what()), juce::dontSendNotification);
    }
    
    return false;
} 
