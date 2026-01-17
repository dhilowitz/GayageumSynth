# Gayageum1

A digital waveguide physical modeling synthesizer that simulates the Korean gayageum (가야금), a traditional 12-string zither instrument.

## Overview

Gayageum1 is a JUCE-based audio plugin that implements a physical model of the gayageum using digital waveguide synthesis techniques. The synthesis engine is based on research by Cho et al. (2007) and employs the Karplus-Strong algorithm with Lagrange interpolation for accurate pitch control.

## Features

### Dual Operating Modes

- **Traditional Mode**: Simulates a 12-string gayageum with individual anjok (movable bridge) position control for each string
  - MIDI notes 60-71 (C4-B4) map to strings 1-12
  - Anjok position sliders control the pitch of each string independently
  - Traditional tuning range: E3 to F#5 (164.81 Hz to 739.99 Hz)

- **Free Play Mode**: Chromatic polyphonic playback with up to 32 simultaneous voices
  - Full MIDI note range support
  - Polyphonic voice allocation with voice stealing
  - Direct MIDI-to-frequency conversion for accurate chromatic playback

### Synthesis Engine

- **Digital Waveguide Model**: Efficient physical modeling using delay lines and filters
- **3rd-Order Lagrange Interpolation**: Fractional delay for precise pitch control
- **One-Pole Loop Filter**: Simulates string damping and frequency-dependent decay
- **Frequency-Compensated Damping**: Equal decay times across all frequencies
- **Dual Excitation System**: Blend between smooth triangle wave (tonal) and noise burst (percussive) excitation
- **Body Resonator**: 5-band resonant filter simulating the instrument body with peaks at 180, 380, 720, 1200, and 2400 Hz

### Controls

- **Play Mode Toggle**: Switch between Traditional and Free Play modes
- **Anjok Position (1-12)**: Individual pitch control for each string (Traditional mode only)
- **Damping**: Controls the decay time of the strings (0.0 = short decay, 1.0 = long sustain)
- **Body Resonance**: Amount of body resonator effect (0.0 = dry, 1.0 = full resonance)
- **Excitation Blend**: Mix between tonal and percussive attack (0.0 = smooth, 1.0 = bright/noisy)

## Technical Details

### Architecture

- **Maximum Voices**: 32 polyphonic voices
- **Delay Line Length**: 4096 samples per voice
- **Sample Rate**: 44.1 kHz (or host-defined)
- **Voice Allocation**: Round-robin with voice stealing when all voices are in use

### Class Structure

- `G1GayageumString`: Individual string/voice implementation with digital waveguide synthesis
- `G1BodyResonator`: Multi-band resonant filter for body simulation
- `Gayageum1AudioProcessor`: Main plugin processor handling MIDI, voice management, and synthesis
- `Gayageum1AudioProcessorEditor`: GUI with parameter controls and mode toggle

### Research Foundation

The physical modeling algorithms are based on:

**Cho, J., Kim, J., & Choi, Y. (2007)**. "Digital Waveguide Modeling of the Gayageum"

Key implementation details:
- Equation 4: Delay length calculation (fs / f0)
- Equation 5: 3rd-order Lagrange interpolation for fractional delay
- Equation 6: One-pole loop filter for damping
- Section 3.3: Leaky integrator method for anjok position frequency calculation

## Building

### Requirements

- JUCE Framework 8.0.8 or later
- Xcode (macOS) or other JUCE-compatible IDE
- Projucer (for project regeneration if needed)

### Build Instructions

1. Open `Gayageum1.jucer` in Projucer (if project files need regeneration)
2. Save and open in your IDE (Xcode on macOS)
3. Build the project:
   ```bash
   cd Builds/MacOSX
   xcodebuild -project Gayageum1.xcodeproj \
              -scheme "Gayageum1 - Standalone Plugin" \
              -configuration Debug build
   ```
4. The built plugin will be in `Builds/MacOSX/build/Debug/`

### Available Formats

- Standalone Application (.app)
- VST3 Plugin (.vst3)
- Audio Unit (.component)

## Usage

### Traditional Mode

1. Launch the plugin/app
2. Ensure "Free Play Mode" toggle is OFF
3. Play MIDI notes C4 (60) through B4 (71) to trigger strings 1-12
4. Adjust individual anjok position sliders to tune each string
5. Control damping, body resonance, and excitation blend globally

### Free Play Mode

1. Toggle "Free Play Mode" ON
2. Play any MIDI note across the full keyboard range
3. Anjok position controls are bypassed in this mode
4. Supports up to 32 simultaneous notes
5. Global controls (damping, resonance, excitation) still apply

## Parameters

| Parameter | Range | Description |
|-----------|-------|-------------|
| Play Mode | Off/On | Traditional (12 strings) or Free Play (chromatic) |
| Anjok 1-12 | 0.3-0.9m | Position of movable bridge (Traditional mode) |
| Damping | 0.0-1.0 | String decay time (0=short, 1=long) |
| Body Resonance | 0.0-1.0 | Body resonator amount (0=dry, 1=full) |
| Excitation Blend | 0.0-1.0 | Tonal vs percussive attack (0=smooth, 1=bright) |

## Credits

- **Physical Modeling Research**: Cho, J., Kim, J., & Choi, Y. (2007)
- **JUCE Framework**: [JUCE](https://juce.com/)
- **Synthesis Techniques**: Karplus-Strong algorithm, digital waveguide synthesis

## License

MIT License. See LICENSE file for details.

## Version History

- **Current**: Dual-mode synthesizer with traditional and chromatic playback
  - 32-voice polyphony in Free Play mode
  - Frequency-compensated damping
  - Mode-aware parameter updates
  - Modular class architecture (G1GayageumString, G1BodyResonator)

---

*Gayageum1 - Digital Waveguide Physical Modeling Synthesizer*
