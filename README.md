# FX Plugin - Frequency Analysis Audio Plugin

A real-time frequency analysis JUCE audio plugin that provides gain, distortion, and detailed frequency analysis capabilities.

## Features

- **Real-time frequency analysis** with JSON output
- **Gain and distortion controls** for basic audio processing
- **Detailed frequency metrics**, including:
  - RMS level
  - True peak level
  - Total energy
  - Peak frequency detection
  - Frequency band energy analysis (sub, low, low-mid, mid, high-mid, high, air)
  - Phase correlation, stereo width, and transient information

## Requirements

- CMake 3.15+
- C++17 compatible compiler
- macOS 10.13+ (for Audio Unit support)

## Building

The project uses CMake for building. Use the included build script:

```bash
./build_and_install.sh
```

This will build the plugin and install it to your user's Audio Units directory.

## Usage

1. Load the plugin in any compatible DAW (Logic Pro, Ableton Live, etc.)
2. Adjust gain and distortion parameters as needed
3. Click "Start Recording" to begin frequency analysis
4. Click "Stop Recording" when finished
5. The analyzed data is saved as a JSON file in `Documents/FXPlugin/`
6. Use the "Reset" button to prepare for new recording sessions

## JSON Output Format

The plugin generates JSON files with the following structure:

```json
{
  "sample_rate": 44100,
  "bit_depth": 32,
  "frame_duration_sec": 0.01,
  "analysis": [
    {
      "time_sec": 0.25,
      "rms_db": -24.5,
      "true_peak_dbfs": -18.2,
      "z_score": -1.23,
      "total_energy_db": -14.5,
      "peak_frequency_hz": 1024,
      "band_energy": {
        "sub": -60.2,
        "low": -40.5,
        "low_mid": -30.8,
        "mid": -25.3,
        "high_mid": -28.7,
        "high": -35.4,
        "air": -45.9
      },
      "phase_correlation": 0.98,
      "stereo_width": 0.05,
      "transient_sharpness": 0.02,
      "rms_rise_time_ms": 8.5,
      "onset_detected": false
    }
    // More frames...
  ]
}
```

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgements

Built with [JUCE](https://juce.com/), the C++ audio application framework. 