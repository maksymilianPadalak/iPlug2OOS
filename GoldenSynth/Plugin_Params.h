#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// PARAMETER DEFINITIONS - Single Source of Truth
// ═══════════════════════════════════════════════════════════════════════════════
//
// This file defines all plugin parameters in one place.
// Both Plugin.h and Plugin_PresetList.h include this file.
//
// ───────────────────────────────────────────────────────────────────────────────
// AI: TO ADD A NEW PARAMETER:
//   1. Add it to EParams enum below (before kNumParams)
//   2. Initialize it in Plugin.cpp constructor (GetParam()->Init...)
//   3. Handle it in Plugin_DSP.h SetParam() switch statement
//   4. Optionally use it in presets (Plugin_PresetList.h)
//
// AI: TO USE PARAMETERS IN PRESETS:
//   - Use these kParamXxx constants directly in Plugin_PresetList.h
//   - Example: {kParamFilterCutoff, 800.} sets filter cutoff to 800 Hz
//   - See each parameter's comment for valid value ranges
//
// ═══════════════════════════════════════════════════════════════════════════════

enum EParams
{
  // ─────────────────────────────────────────────────────────────────────────────
  // CORE PARAMETERS
  // ─────────────────────────────────────────────────────────────────────────────
  kParamGain = 0,               // Master output gain (0-100%)
  kParamWaveform,               // Osc1 waveform: 0=Sine, 1=Saw, 2=Square, 3=Triangle, 4=Pulse, 5=FM, 6=Wavetable
  kParamWavetablePosition,      // Morph position within wavetable (0-100%)
  kParamAttack,                 // Envelope attack time (1-1000 ms)
  kParamDecay,                  // Envelope decay time (1-2000 ms)
  kParamSustain,                // Envelope sustain level (0-100%)
  kParamRelease,                // Envelope release time (1-5000 ms)
  kParamEnvVelocity,            // Velocity → envelope time modulation (0-100%)

  // ─────────────────────────────────────────────────────────────────────────────
  // FILTER PARAMETERS
  // ─────────────────────────────────────────────────────────────────────────────
  kParamFilterEnable,           // Filter on/off: 0=Off (bypassed), 1=On
  kParamFilterCutoff,           // Filter cutoff frequency (20-20000 Hz)
  kParamFilterResonance,        // Filter resonance (0-100%)
  kParamFilterType,             // Filter type: 0=Lowpass, 1=Highpass, 2=Bandpass, 3=Notch

  // ─────────────────────────────────────────────────────────────────────────────
  // FILTER ENVELOPE PARAMETERS
  // ─────────────────────────────────────────────────────────────────────────────
  kParamFilterEnvAttack,        // Filter envelope attack time (1-1000 ms)
  kParamFilterEnvDecay,         // Filter envelope decay time (1-2000 ms)
  kParamFilterEnvSustain,       // Filter envelope sustain level (0-100%)
  kParamFilterEnvRelease,       // Filter envelope release time (1-5000 ms)
  kParamFilterEnvDepth,         // Filter envelope depth (-100% to +100%, ±4 octaves)

  // ─────────────────────────────────────────────────────────────────────────────
  // OSC1 PARAMETERS
  // ─────────────────────────────────────────────────────────────────────────────
  kParamPulseWidth,             // Pulse width (5-95%), 50% = square wave
  kParamFMRatio,                // FM coarse ratio: 0=0.5x, 1=1x, 2=2x, 3=3x, 4=4x, 5=5x, 6=6x, 7=7x, 8=8x
  kParamFMFine,                 // FM fine ratio offset (-50 to +50%)
  kParamFMDepth,                // FM modulation depth/index (0-100%)
  kParamOsc1Level,              // Osc1 mix level (0-100%)
  kParamOsc1Octave,             // Osc1 octave: 0=-2oct, 1=-1oct, 2=0oct, 3=+1oct, 4=+2oct
  kParamOsc1Detune,             // Osc1 fine detune in cents (-100 to +100)
  kParamOsc1Pan,                // Osc1 stereo pan (-100 to +100%)

  // ─────────────────────────────────────────────────────────────────────────────
  // OSC2 PARAMETERS (fully independent, Serum-style)
  // ─────────────────────────────────────────────────────────────────────────────
  kParamOsc2Waveform,           // Osc2 waveform: 0=Sine, 1=Saw, 2=Square, 3=Triangle, 4=Pulse, 5=FM, 6=Wavetable
  kParamOsc2Octave,             // Osc2 octave: 0=-2oct, 1=-1oct, 2=0oct, 3=+1oct, 4=+2oct
  kParamOsc2Detune,             // Osc2 fine detune in cents (-100 to +100)
  kParamOsc2Level,              // Osc2 mix level (0-100%)
  kParamOsc2Morph,              // Osc2 wavetable morph position (0-100%)
  kParamOsc2PulseWidth,         // Osc2 pulse width (5-95%)
  kParamOsc2FMRatio,            // Osc2 FM coarse ratio: 0=0.5x, 1=1x, 2=2x, 3=3x, 4=4x, 5=5x, 6=6x, 7=7x, 8=8x
  kParamOsc2FMFine,             // Osc2 FM fine ratio offset (-50 to +50%)
  kParamOsc2FMDepth,            // Osc2 FM modulation depth (0-100%)
  kParamOsc2Pan,                // Osc2 stereo pan (-100 to +100%)

  // ─────────────────────────────────────────────────────────────────────────────
  // SUB OSCILLATOR - Simple waveform for bass foundation (Serum-style)
  // ─────────────────────────────────────────────────────────────────────────────
  kParamSubOscEnable,           // Sub oscillator on/off: 0=Off, 1=On
  kParamSubOscWaveform,         // Sub waveform: 0=Sine, 1=Triangle, 2=Saw, 3=Square
  kParamSubOscOctave,           // Sub octave: 0=-1oct, 1=-2oct, 2=-3oct
  kParamSubOscLevel,            // Sub oscillator level (0-100%)
  kParamSubOscPan,              // Sub oscillator pan (-100 to +100%)
  kParamSubOscDirectOut,        // Direct out: 0=Through filter, 1=Bypass filter+FX

  // ─────────────────────────────────────────────────────────────────────────────
  // OSC1 UNISON - Stack multiple detuned voices for massive sound
  // ─────────────────────────────────────────────────────────────────────────────
  kParamOsc1UnisonVoices,       // Osc1 unison voices (1-8), 1 = off
  kParamOsc1UnisonDetune,       // Osc1 detune spread (0-100%)
  kParamOsc1UnisonWidth,        // Osc1 stereo spread (0-100%)
  kParamOsc1UnisonBlend,        // Osc1 center vs detuned mix (0-100%)

  // ─────────────────────────────────────────────────────────────────────────────
  // OSC2 UNISON - Independent unison for second oscillator
  // ─────────────────────────────────────────────────────────────────────────────
  kParamOsc2UnisonVoices,       // Osc2 unison voices (1-8), 1 = off
  kParamOsc2UnisonDetune,       // Osc2 detune spread (0-100%)
  kParamOsc2UnisonWidth,        // Osc2 stereo spread (0-100%)
  kParamOsc2UnisonBlend,        // Osc2 center vs detuned mix (0-100%)

  // ─────────────────────────────────────────────────────────────────────────────
  // OSCILLATOR SYNC - Classic analog sync sound
  // ─────────────────────────────────────────────────────────────────────────────
  kParamOscSync,                // Sync mode: 0=Off, 1=Hard (Osc2 syncs to Osc1)

  // ─────────────────────────────────────────────────────────────────────────────
  // LFO1 - Low Frequency Oscillator 1 for modulation
  // ─────────────────────────────────────────────────────────────────────────────
  kParamLFO1Enable,             // LFO1 on/off: 0=Off, 1=On
  kParamLFO1Rate,               // LFO1 rate (0.01-20 Hz)
  kParamLFO1Sync,               // LFO1 tempo sync: 0=Off, 1=4/1, 2=2/1, 3=1/1, 4=1/2, 5=1/4, 6=1/8, 7=1/16, 8=1/32...
  kParamLFO1Low,                // LFO1 low point (-100 to +100%)
  kParamLFO1High,               // LFO1 high point (-100 to +100%)
  kParamLFO1Waveform,           // LFO1 waveform: 0=Sine, 1=Tri, 2=SawUp, 3=SawDown, 4=Square, 5=S&H
  kParamLFO1Retrigger,          // LFO1 retrigger: 0=Free, 1=Retrig
  kParamLFO1Destination,        // LFO1 dest: 0=Off, 1=Filter, 2=Pitch, 3=PW, 4=Amp, 5=FM, 6=Wavetable

  // ─────────────────────────────────────────────────────────────────────────────
  // LFO2 - Low Frequency Oscillator 2 for modulation
  // ─────────────────────────────────────────────────────────────────────────────
  kParamLFO2Enable,             // LFO2 on/off: 0=Off, 1=On
  kParamLFO2Rate,               // LFO2 rate (0.01-20 Hz)
  kParamLFO2Sync,               // LFO2 tempo sync: 0=Off, 1=4/1, 2=2/1, 3=1/1, 4=1/2, 5=1/4, 6=1/8, 7=1/16, 8=1/32...
  kParamLFO2Low,                // LFO2 low point (-100 to +100%)
  kParamLFO2High,               // LFO2 high point (-100 to +100%)
  kParamLFO2Waveform,           // LFO2 waveform: 0=Sine, 1=Tri, 2=SawUp, 3=SawDown, 4=Square, 5=S&H
  kParamLFO2Retrigger,          // LFO2 retrigger: 0=Free, 1=Retrig
  kParamLFO2Destination,        // LFO2 dest: 0=Off, 1=Filter, 2=Pitch, 3=PW, 4=Amp, 5=FM, 6=Wavetable

  // ─────────────────────────────────────────────────────────────────────────────
  // STEREO DELAY - Tempo-syncable delay effect
  // ─────────────────────────────────────────────────────────────────────────────
  kParamDelayEnable,            // Delay on/off: 0=Off, 1=On
  kParamDelayTime,              // Delay time (1-2000 ms)
  kParamDelaySync,              // Tempo sync: 0=Off, 1=1/1, 2=1/2, 3=1/4, 4=1/8, 5=1/16, 6=1/32...
  kParamDelayFeedback,          // Feedback amount (0-90%)
  kParamDelayDry,               // Dry signal level (0-100%)
  kParamDelayWet,               // Wet signal level (0-100%)
  kParamDelayMode,              // Stereo mode: 0=Stereo, 1=Ping-Pong

  // ─────────────────────────────────────────────────────────────────────────────
  // REVERB - Dattorro plate reverb effect
  // ─────────────────────────────────────────────────────────────────────────────
  kParamReverbEnable,           // Reverb on/off: 0=Off, 1=On
  kParamReverbDecay,            // Decay time (0-100%)
  kParamReverbSize,             // Room size (0-100%)
  kParamReverbDamping,          // High-frequency damping (0-100%)
  kParamReverbWidth,            // Stereo width (0-100%)
  kParamReverbDry,              // Dry signal level (0-100%)
  kParamReverbWet,              // Wet signal level (0-100%)
  kParamReverbPreDelay,         // Pre-delay time (0-100 ms)
  kParamReverbMode,             // Mode: 0=Plate, 1=Chamber, 2=Hall, 3=Cathedral
  kParamReverbColor,            // Color: 0=Bright, 1=Neutral, 2=Dark, 3=Studio
  kParamReverbModRate,          // Internal modulation rate (0.1-2.0 Hz)
  kParamReverbModDepth,         // Internal modulation depth (0-100%)
  kParamReverbLowCut,           // Low-frequency cut filter (20-500 Hz)
  kParamReverbDensity,          // Diffusion density (0-100%)
  kParamReverbEarlyLate,        // Early/Late balance (0=early, 100=late)
  kParamReverbFreeze,           // Freeze mode: 0=Off, 1=On (infinite sustain)

  // ─────────────────────────────────────────────────────────────────────────────
  // VOICE MODE & GLIDE - Mono/Poly selection and portamento
  // ─────────────────────────────────────────────────────────────────────────────
  kParamVoiceMode,              // Voice mode: 0=Poly, 1=Mono, 2=Legato
  kParamGlideEnable,            // Glide on/off: 0=Off, 1=On
  kParamGlideTime,              // Glide time (1-2000 ms)

  // ─────────────────────────────────────────────────────────────────────────────
  // SYSTEM PARAMETERS
  // ─────────────────────────────────────────────────────────────────────────────
  kParamVoiceCount,             // Active voice count (0-32) - read-only for UI
  kParamPresetSelect,           // Preset selector (auto-populated from presets)

  kNumParams                    // Total parameter count - must be last!
};
