#pragma once

#include "Plugin_Params.h"  // EParams enum - single source of truth

// ═══════════════════════════════════════════════════════════════════════════════
// PRESET SYSTEM - AI-Proof Single Source of Truth
// ═══════════════════════════════════════════════════════════════════════════════
//
// AI: TO ADD A NEW PRESET:
//   1. Add a new entry to kPresetDefs[] array below (before the closing brace)
//   2. That's it! Everything else auto-updates.
//
// No manual counting. No syncing files. Just add the preset definition.
//
// ───────────────────────────────────────────────────────────────────────────────
// TEMPLATE - Copy this block and modify:
// ───────────────────────────────────────────────────────────────────────────────
//
//   {
//     "My Preset Name",
//     false,  // false = custom parameters, true = all defaults
//     {
//       {kParamWaveform, 0},        // 0=Sine, 1=Saw, 2=Square, 3=Triangle, 4=Pulse, 5=FM, 6=Wavetable
//       {kParamFilterEnable, 1},    // 0=Off, 1=On
//       {kParamFilterCutoff, 1000.},// 20-20000 Hz
//       {kParamFilterResonance, 50.},// 0-100%
//       {kParamFilterType, 0},      // 0=LP, 1=HP, 2=BP, 3=Notch
//       {kParamOsc2Octave, 2},      // 0=-2oct, 1=-1oct, 2=0, 3=+1oct, 4=+2oct
//       {kParamAttack, 10.},        // 1-1000 ms
//       {kParamDecay, 200.},        // 1-2000 ms
//       {kParamSustain, 70.},       // 0-100%
//       {kParamRelease, 300.},      // 1-5000 ms
//       {kParamGain, 80.},          // 0-100%
//       {END, 0}                    // ← REQUIRED: Always end with this!
//     }
//   },
//
// See Plugin_Params.h for the full list of kParamXxx constants and their ranges.
//
// ═══════════════════════════════════════════════════════════════════════════════

// Maximum parameters per preset - increase if needed
inline constexpr int kMaxPresetParams = 64;

// Sentinel marking end of parameter list
inline constexpr int END = -1;

// Parameter entry: {paramIdx, value}
struct ParamEntry {
  int idx;
  double val;
};

// Preset definition struct
struct PresetDef {
  const char* name;
  bool isDefault;  // true = use all defaults, false = use params below
  ParamEntry params[kMaxPresetParams];  // terminated by {END, 0}
};

// ═══════════════════════════════════════════════════════════════════════════════
// PRESET DEFINITIONS
// ═══════════════════════════════════════════════════════════════════════════════
// AI: Add new presets to this array. Copy an existing entry and modify.
// Use kParamXxx constants from Plugin_Params.h for parameter indices.
// ═══════════════════════════════════════════════════════════════════════════════

inline constexpr PresetDef kPresetDefs[] = {

  // ───────────────────────────────────────────────────────────────────────────
  // Preset 0: Init
  // Clean default sound - starting point for sound design
  // ───────────────────────────────────────────────────────────────────────────
  {
    "Init",
    true,   // Use all default parameter values
    {}
  },

  // ───────────────────────────────────────────────────────────────────────────
  // Preset 1: Classic Lead
  // Warm filtered lead with Square + Triangle oscillators
  // Inspired by classic Moog/Prophet style patches
  // ───────────────────────────────────────────────────────────────────────────
  {
    "Classic Lead",
    false,  // false = use custom parameters below
    {
      // Waveform values: 0=Sine, 1=Saw, 2=Square, 3=Triangle, 4=Pulse, 5=FM, 6=Wavetable
      {kParamWaveform, 2},           // Osc1: Square (punchy, hollow)
      {kParamOsc1Level, 100.},       // Osc1: Full volume (0-100%)
      {kParamOsc2Waveform, 3},       // Osc2: Triangle (soft, warm)
      {kParamOsc2Level, 80.},        // Osc2: Slightly lower for blend (0-100%)
      // Octave values: 0=-2oct, 1=-1oct, 2=0oct, 3=+1oct, 4=+2oct
      {kParamOsc2Octave, 3},         // Osc2: +1 octave
      {kParamFilterEnable, 1},       // Filter: ON (0=Off, 1=On)
      {kParamFilterCutoff, 800.},    // Filter: Low cutoff for warmth (20-20000 Hz)
      {kParamFilterResonance, 60.},  // Filter: High resonance (0-100%)
      // FilterType values: 0=Lowpass, 1=Highpass, 2=Bandpass, 3=Notch
      {kParamFilterType, 0},         // Filter: Lowpass
      {kParamAttack, 5.},            // Env: Fast attack (1-1000 ms)
      {kParamDecay, 300.},           // Env: Medium decay (1-2000 ms)
      {kParamSustain, 50.},          // Env: Medium sustain (0-100%)
      {kParamRelease, 400.},         // Env: Medium release (1-5000 ms)
      {kParamGain, 70.},             // Output: 70% (0-100%)
      {END, 0}                       // ← REQUIRED: Always end with this!
    }
  },

  // ───────────────────────────────────────────────────────────────────────────
  // Preset 2: Massive Supersaw
  // Modern EDM supersaw with huge unison stacks, filter sweep, and stereo delay
  // Inspired by: Avicii, Martin Garrix, Swedish House Mafia
  // Uses ALL parameters to showcase full synth capability
  // ───────────────────────────────────────────────────────────────────────────
  {
    "Massive Supersaw",
    false,
    {
      // ═══ CORE ═══
      {kParamGain, 75.},                 // Slightly reduced to avoid clipping with unison
      {kParamAttack, 3.},                // Very fast attack for punchy transients
      {kParamDecay, 400.},               // Medium-long decay for sustain
      {kParamSustain, 85.},              // High sustain for full chords
      {kParamRelease, 600.},             // Long release for smooth tails
      {kParamEnvVelocity, 30.},          // Subtle velocity response

      // ═══ OSC1: Main Supersaw ═══
      {kParamWaveform, 1},               // Saw wave - the supersaw foundation
      {kParamWavetablePosition, 0.},     // Not used for Saw, but set explicitly
      {kParamPulseWidth, 50.},           // Not used for Saw
      {kParamFMRatio, 2},                // Not used for Saw (2:1 default)
      {kParamFMFine, 0.},                // Not used for Saw
      {kParamFMDepth, 0.},               // Not used for Saw
      {kParamOsc1Level, 100.},           // Full volume
      {kParamOsc1Octave, 2},             // 0 octave (center)
      {kParamOsc1Detune, 0.},            // No base detune (unison handles this)
      {kParamOsc1Pan, -15.},             // Slightly left for width

      // ═══ OSC1 UNISON: 7 voices for massive width ═══
      {kParamOsc1UnisonVoices, 6},       // 7 voices (index 6 = 7)
      {kParamOsc1UnisonDetune, 45.},     // Moderate detune spread
      {kParamOsc1UnisonWidth, 90.},      // Wide stereo spread
      {kParamOsc1UnisonBlend, 80.},      // Favor detuned voices

      // ═══ OSC2: Supporting Saw one octave up ═══
      {kParamOsc2Waveform, 1},           // Also Saw
      {kParamOsc2Octave, 3},             // +1 octave for brightness
      {kParamOsc2Detune, 5.},            // Slight detune from Osc1
      {kParamOsc2Level, 60.},            // Lower than Osc1
      {kParamOsc2Morph, 0.},             // Not used for Saw
      {kParamOsc2PulseWidth, 50.},       // Not used for Saw
      {kParamOsc2FMRatio, 2},            // Not used
      {kParamOsc2FMFine, 0.},            // Not used
      {kParamOsc2FMDepth, 0.},           // Not used
      {kParamOsc2Pan, 15.},              // Slightly right (opposite of Osc1)

      // ═══ OSC2 UNISON: 5 voices for additional thickness ═══
      {kParamOsc2UnisonVoices, 4},       // 5 voices (index 4 = 5)
      {kParamOsc2UnisonDetune, 35.},     // Less detune than Osc1
      {kParamOsc2UnisonWidth, 85.},      // Wide but slightly narrower
      {kParamOsc2UnisonBlend, 70.},      // Balanced blend

      // ═══ OSC SYNC: Off for clean supersaw ═══
      {kParamOscSync, 0},                // Off

      // ═══ FILTER: Lowpass with subtle sweep ═══
      {kParamFilterEnable, 1},           // On
      {kParamFilterCutoff, 4000.},       // Mid-high cutoff
      {kParamFilterResonance, 15.},      // Subtle resonance
      {kParamFilterType, 0},             // Lowpass

      // ═══ LFO1: Slow filter sweep ═══
      {kParamLFO1Enable, 1},             // On
      {kParamLFO1Rate, 0.15},            // Very slow
      {kParamLFO1Sync, 0},               // Free running
      {kParamLFO1Low, -30.},             // Subtle negative
      {kParamLFO1High, 30.},             // Subtle positive
      {kParamLFO1Waveform, 1},           // Triangle for smooth sweep
      {kParamLFO1Retrigger, 0},          // Free running
      {kParamLFO1Destination, 1},        // Filter

      // ═══ LFO2: Subtle stereo movement via amp ═══
      {kParamLFO2Enable, 1},             // On
      {kParamLFO2Rate, 0.08},            // Very slow
      {kParamLFO2Sync, 0},               // Free
      {kParamLFO2Low, -5.},              // Very subtle
      {kParamLFO2High, 5.},              // Very subtle
      {kParamLFO2Waveform, 0},           // Sine
      {kParamLFO2Retrigger, 0},          // Free
      {kParamLFO2Destination, 4},        // Amp - subtle volume swell

      // ═══ DELAY: Tempo-synced 1/8 dotted for rhythmic depth ═══
      {kParamDelayEnable, 1},            // On
      {kParamDelayTime, 375.},           // ~375ms (ignored when synced)
      {kParamDelaySync, 8},              // 1/8 dotted
      {kParamDelayFeedback, 35.},        // Moderate feedback
      {kParamDelayDry, 100.},            // Full dry
      {kParamDelayWet, 25.},             // Subtle wet
      {kParamDelayMode, 0},              // Stereo

      {END, 0}
    }
  },

  // ───────────────────────────────────────────────────────────────────────────
  // Preset 3: FM Bell Pad
  // Evolving crystalline pad using FM synthesis and wavetable
  // Dual LFO modulation creates movement, ping-pong delay adds space
  // Inspired by: Brian Eno, Boards of Canada, Tycho
  // Uses ALL parameters including FM, Wavetable, Sync, and both LFOs
  // ───────────────────────────────────────────────────────────────────────────
  {
    "FM Bell Pad",
    false,
    {
      // ═══ CORE ═══
      {kParamGain, 70.},                 // Moderate gain for pad
      {kParamAttack, 350.},              // Slow attack for pad swell
      {kParamDecay, 800.},               // Long decay
      {kParamSustain, 65.},              // Medium sustain
      {kParamRelease, 2500.},            // Very long release for ambient tails
      {kParamEnvVelocity, 20.},          // Minimal velocity response

      // ═══ OSC1: FM Bell tones ═══
      {kParamWaveform, 5},               // FM synthesis
      {kParamWavetablePosition, 0.},     // Not used for FM
      {kParamPulseWidth, 50.},           // Not used for FM
      {kParamFMRatio, 4},                // 4:1 ratio for bell harmonics
      {kParamFMFine, 7.},                // Slight inharmonicity for metallic tone
      {kParamFMDepth, 65.},              // Strong FM depth
      {kParamOsc1Level, 85.},            // Main oscillator
      {kParamOsc1Octave, 3},             // +1 octave for brightness
      {kParamOsc1Detune, -3.},           // Slight detune down
      {kParamOsc1Pan, -25.},             // Left of center

      // ═══ OSC1 UNISON: Subtle doubling ═══
      {kParamOsc1UnisonVoices, 1},       // 2 voices (index 1 = 2)
      {kParamOsc1UnisonDetune, 12.},     // Subtle detune
      {kParamOsc1UnisonWidth, 60.},      // Moderate width
      {kParamOsc1UnisonBlend, 50.},      // Equal blend

      // ═══ OSC2: Wavetable for evolving texture ═══
      {kParamOsc2Waveform, 6},           // Wavetable
      {kParamOsc2Octave, 2},             // Same octave
      {kParamOsc2Detune, 8.},            // Detune up from Osc1
      {kParamOsc2Level, 55.},            // Supporting role
      {kParamOsc2Morph, 35.},            // Starting wavetable position
      {kParamOsc2PulseWidth, 50.},       // Not used for Wavetable
      {kParamOsc2FMRatio, 3},            // Not used
      {kParamOsc2FMFine, 0.},            // Not used
      {kParamOsc2FMDepth, 0.},           // Not used
      {kParamOsc2Pan, 25.},              // Right of center (stereo spread)

      // ═══ OSC2 UNISON: Minimal for clarity ═══
      {kParamOsc2UnisonVoices, 2},       // 3 voices (index 2 = 3)
      {kParamOsc2UnisonDetune, 18.},     // Moderate detune
      {kParamOsc2UnisonWidth, 70.},      // Good width
      {kParamOsc2UnisonBlend, 60.},      // Slight favor to detuned

      // ═══ OSC SYNC: On for harmonic complexity ═══
      {kParamOscSync, 1},                // Hard sync enabled

      // ═══ FILTER: Bandpass for focused mids ═══
      {kParamFilterEnable, 1},           // On
      {kParamFilterCutoff, 2200.},       // Mid frequency focus
      {kParamFilterResonance, 35.},      // Noticeable resonance
      {kParamFilterType, 2},             // Bandpass

      // ═══ LFO1: Filter modulation ═══
      {kParamLFO1Enable, 1},             // On
      {kParamLFO1Rate, 0.25},            // Slow
      {kParamLFO1Sync, 0},               // Free running for organic feel
      {kParamLFO1Low, -45.},             // Moderate depth
      {kParamLFO1High, 45.},             // Moderate depth
      {kParamLFO1Waveform, 0},           // Sine for smooth
      {kParamLFO1Retrigger, 0},          // Free
      {kParamLFO1Destination, 1},        // Filter

      // ═══ LFO2: Wavetable morphing ═══
      {kParamLFO2Enable, 1},             // On
      {kParamLFO2Rate, 0.12},            // Very slow
      {kParamLFO2Sync, 0},               // Free
      {kParamLFO2Low, -60.},             // Wide range
      {kParamLFO2High, 60.},             // Wide range
      {kParamLFO2Waveform, 1},           // Triangle
      {kParamLFO2Retrigger, 0},          // Free
      {kParamLFO2Destination, 14},       // Osc2 Wavetable position

      // ═══ DELAY: Ping-pong for spatial depth ═══
      {kParamDelayEnable, 1},            // On
      {kParamDelayTime, 500.},           // Base time (ignored when synced)
      {kParamDelaySync, 6},              // 1/4 note
      {kParamDelayFeedback, 50.},        // High feedback for ambient
      {kParamDelayDry, 85.},             // Mostly dry
      {kParamDelayWet, 40.},             // Prominent wet
      {kParamDelayMode, 1},              // Ping-Pong

      {END, 0}
    }
  },

  // ───────────────────────────────────────────────────────────────────────────
  // Preset 4: Dubstep Wobble Bass
  // Heavy wobble bass with tempo-synced filter LFO and pulse width modulation
  // Aggressive resonance and short punchy envelope
  // Inspired by: Skrillex, Excision, Zomboy
  // Uses ALL parameters with focus on filter modulation and PWM
  // ───────────────────────────────────────────────────────────────────────────
  {
    "Wobble Bass",
    false,
    {
      // ═══ CORE ═══
      {kParamGain, 90.},                 // Loud bass
      {kParamAttack, 1.},                // Instant attack
      {kParamDecay, 150.},               // Short decay
      {kParamSustain, 95.},              // Near-full sustain for sustained notes
      {kParamRelease, 80.},              // Quick release for tight bass
      {kParamEnvVelocity, 60.},          // Velocity sensitive for dynamics

      // ═══ OSC1: Pulse wave with PWM ═══
      {kParamWaveform, 4},               // Pulse wave
      {kParamWavetablePosition, 0.},     // Not used for Pulse
      {kParamPulseWidth, 35.},           // Narrow pulse for nasal character
      {kParamFMRatio, 2},                // Not used
      {kParamFMFine, 0.},                // Not used
      {kParamFMDepth, 0.},               // Not used
      {kParamOsc1Level, 100.},           // Full
      {kParamOsc1Octave, 1},             // -1 octave for bass
      {kParamOsc1Detune, 0.},            // No detune
      {kParamOsc1Pan, 0.},               // Center for mono bass

      // ═══ OSC1 UNISON: Doubled for thickness ═══
      {kParamOsc1UnisonVoices, 1},       // 2 voices (index 1 = 2)
      {kParamOsc1UnisonDetune, 8.},      // Tight detune
      {kParamOsc1UnisonWidth, 0.},       // Mono (no stereo spread for bass)
      {kParamOsc1UnisonBlend, 50.},      // Equal

      // ═══ OSC2: Sub sine for low-end weight ═══
      {kParamOsc2Waveform, 0},           // Sine - pure sub
      {kParamOsc2Octave, 0},             // -2 octaves for deep sub
      {kParamOsc2Detune, 0.},            // No detune on sub
      {kParamOsc2Level, 70.},            // Strong sub presence
      {kParamOsc2Morph, 0.},             // Not used for Sine
      {kParamOsc2PulseWidth, 50.},       // Not used
      {kParamOsc2FMRatio, 1},            // Not used
      {kParamOsc2FMFine, 0.},            // Not used
      {kParamOsc2FMDepth, 0.},           // Not used
      {kParamOsc2Pan, 0.},               // Center

      // ═══ OSC2 UNISON: Off for clean sub ═══
      {kParamOsc2UnisonVoices, 0},       // 1 voice (off)
      {kParamOsc2UnisonDetune, 0.},      // N/A
      {kParamOsc2UnisonWidth, 0.},       // N/A
      {kParamOsc2UnisonBlend, 50.},      // N/A

      // ═══ OSC SYNC: Off for pure bass ═══
      {kParamOscSync, 0},                // Off

      // ═══ FILTER: Aggressive lowpass with high resonance ═══
      {kParamFilterEnable, 1},           // On
      {kParamFilterCutoff, 400.},        // Low cutoff - wobble opens this
      {kParamFilterResonance, 75.},      // High resonance for aggressive wobble
      {kParamFilterType, 0},             // Lowpass

      // ═══ LFO1: Tempo-synced wobble to filter ═══
      {kParamLFO1Enable, 1},             // On
      {kParamLFO1Rate, 4.},              // Base rate (ignored when synced)
      {kParamLFO1Sync, 10},              // 1/8 note for classic wobble
      {kParamLFO1Low, -100.},            // Full negative
      {kParamLFO1High, 100.},            // Full positive - maximum wobble
      {kParamLFO1Waveform, 0},           // Sine for smooth wobble
      {kParamLFO1Retrigger, 1},          // Retrigger for consistent wobble phase
      {kParamLFO1Destination, 1},        // Filter

      // ═══ LFO2: PWM modulation for movement ═══
      {kParamLFO2Enable, 1},             // On
      {kParamLFO2Rate, 2.5},             // Moderate rate
      {kParamLFO2Sync, 7},               // 1/4 triplet for polyrhythm
      {kParamLFO2Low, -40.},             // Moderate depth
      {kParamLFO2High, 40.},             // Moderate depth
      {kParamLFO2Waveform, 1},           // Triangle
      {kParamLFO2Retrigger, 1},          // Retrigger
      {kParamLFO2Destination, 9},        // Osc1 PW

      // ═══ DELAY: Off for tight bass ═══
      {kParamDelayEnable, 0},            // Off - bass should be dry
      {kParamDelayTime, 125.},           // Short if enabled
      {kParamDelaySync, 0},              // Off
      {kParamDelayFeedback, 20.},        // Low
      {kParamDelayDry, 100.},            // Full dry
      {kParamDelayWet, 0.},              // No wet
      {kParamDelayMode, 0},              // Stereo

      {END, 0}
    }
  },

  // ═══════════════════════════════════════════════════════════════════════════
  // AI: ADD NEW PRESETS BELOW THIS LINE
  // ═══════════════════════════════════════════════════════════════════════════
  //
  // Instructions:
  //   1. Copy a preset block above
  //   2. Paste it here (before the closing brace)
  //   3. Change the preset name string
  //   4. Modify parameter values as needed
  //   5. Always end with {END, 0}
  //
  // Quick reference for enum parameters:
  //   Waveform: 0=Sine, 1=Saw, 2=Square, 3=Triangle, 4=Pulse, 5=FM, 6=Wavetable
  //   Octave:   0=-2oct, 1=-1oct, 2=0oct, 3=+1oct, 4=+2oct
  //   Filter:   0=Off, 1=On | Type: 0=LP, 1=HP, 2=BP, 3=Notch
  //   LFO Wave: 0=Sine, 1=Tri, 2=SawUp, 3=SawDown, 4=Square, 5=S&H
  //   LFO Dest: 0=Off, 1=Filter, 2=Pitch, 3=PW, 4=Amp, 5=FM, 6=WT,
  //             7=Osc1Pitch, 8=Osc2Pitch, 9=Osc1PW, 10=Osc2PW,
  //             11=Osc1FM, 12=Osc2FM, 13=Osc1WT, 14=Osc2WT
  //   Delay Sync: 0=Off, 1=1/1, 2=1/2D, 3=1/2, 4=1/2T, 5=1/4D, 6=1/4, 7=1/4T,
  //               8=1/8D, 9=1/8, 10=1/8T, 11=1/16D, 12=1/16, 13=1/16T, 14=1/32
  //
  // ═══════════════════════════════════════════════════════════════════════════

};

// ═══════════════════════════════════════════════════════════════════════════════
// AUTO-DERIVED CONSTANTS - Do not edit!
// ═══════════════════════════════════════════════════════════════════════════════

inline constexpr int kPresetCount = sizeof(kPresetDefs) / sizeof(kPresetDefs[0]);

static_assert(kPresetCount >= 1, "Must have at least one preset");
static_assert(kPresetCount <= 128, "Maximum 128 presets supported");
