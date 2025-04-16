#pragma once

// Plugin name
#ifndef JucePlugin_Name
    #define JucePlugin_Name                   "FX Plugin"
#endif

// Plugin manufacturer
#ifndef JucePlugin_Manufacturer
    #define JucePlugin_Manufacturer           "YourCompany"
#endif

// Plugin manufacturer code
#ifndef JucePlugin_ManufacturerCode
    #define JucePlugin_ManufacturerCode       'Vbab'
#endif

// Plugin code
#ifndef JucePlugin_PluginCode
    #define JucePlugin_PluginCode             'Fxpl'
#endif

// Plugin AU main type
#ifndef JucePlugin_AUMainType
    #define JucePlugin_AUMainType             'aufx'
#endif

// Plugin format flags
#ifndef JucePlugin_IsSynth
    #define JucePlugin_IsSynth                0
#endif

#ifndef JucePlugin_WantsMidiInput
    #define JucePlugin_WantsMidiInput         0
#endif

#ifndef JucePlugin_ProducesMidiOutput
    #define JucePlugin_ProducesMidiOutput     0
#endif

#ifndef JucePlugin_IsMidiEffect
    #define JucePlugin_IsMidiEffect           0
#endif

// Plugin descriptions
#ifndef JucePlugin_Desc
    #define JucePlugin_Desc                   "A simple FX Audio Unit plugin"
#endif

#ifndef JucePlugin_VSTCategory
    #define JucePlugin_VSTCategory            kPlugCategEffect
#endif

#ifndef JucePlugin_AUExportPrefix
    #define JucePlugin_AUExportPrefix         FXPluginAU
#endif

#ifndef JucePlugin_AUExportPrefixQuoted
    #define JucePlugin_AUExportPrefixQuoted   "FXPluginAU"
#endif

#ifndef JucePlugin_CFBundleIdentifier
    #define JucePlugin_CFBundleIdentifier     com.yourcompany.FXPlugin
#endif

#ifndef JucePlugin_AAXIdentifier
    #define JucePlugin_AAXIdentifier          com.yourcompany.FXPlugin
#endif 