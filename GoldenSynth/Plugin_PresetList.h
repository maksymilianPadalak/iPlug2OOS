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
  // Preset 2: Deep Bass
  // Solid, punchy bass with saw harmonics and sine sub
  // Sits well in any mix - tight, focused, mono-compatible
  // ───────────────────────────────────────────────────────────────────────────
  {
    "Deep Bass",
    false,
    {
      // Envelope: Punchy and tight
      {kParamGain, 85.},
      {kParamAttack, 2.},                // Instant punch
      {kParamDecay, 180.},               // Quick decay
      {kParamSustain, 75.},              // Good sustain for held notes
      {kParamRelease, 120.},             // Tight release
      {kParamEnvVelocity, 35.},          // Some dynamics

      // Osc1: Saw for harmonics
      {kParamWaveform, 1},               // Saw
      {kParamOsc1Level, 100.},
      {kParamOsc1Octave, 2},             // Root octave
      {kParamOsc1Detune, 0.},
      {kParamOsc1Pan, 0.},               // Center (mono bass)

      // Osc1 Unison: Subtle thickening
      {kParamOsc1UnisonVoices, 1},       // 2 voices
      {kParamOsc1UnisonDetune, 8.},      // Tight
      {kParamOsc1UnisonWidth, 0.},       // Mono!
      {kParamOsc1UnisonBlend, 50.},

      // Osc2: Sine sub one octave down
      {kParamOsc2Waveform, 0},           // Sine - pure sub
      {kParamOsc2Octave, 1},             // -1 octave
      {kParamOsc2Detune, 0.},
      {kParamOsc2Level, 65.},            // Strong sub
      {kParamOsc2Pan, 0.},               // Center

      // Osc2 Unison: Off for clean sub
      {kParamOsc2UnisonVoices, 0},       // 1 voice

      // Filter: Tame the highs
      {kParamFilterEnable, 1},
      {kParamFilterCutoff, 1200.},       // Remove harshness
      {kParamFilterResonance, 20.},      // Subtle bump
      {kParamFilterType, 0},             // Lowpass

      // LFO1: Very subtle filter movement
      {kParamLFO1Enable, 1},
      {kParamLFO1Rate, 0.3},
      {kParamLFO1Sync, 0},
      {kParamLFO1Low, -10.},
      {kParamLFO1High, 10.},
      {kParamLFO1Waveform, 0},           // Sine
      {kParamLFO1Retrigger, 1},          // Retrigger for consistency
      {kParamLFO1Destination, 1},        // Filter

      // LFO2: Off
      {kParamLFO2Enable, 0},

      // Delay: Off - bass should be dry
      {kParamDelayEnable, 0},

      {END, 0}
    }
  },

  // ───────────────────────────────────────────────────────────────────────────
  // Preset 3: Bright Lead
  // Cutting, expressive lead with movement
  // Stereo width from detuned oscillators, filter resonance for presence
  // ───────────────────────────────────────────────────────────────────────────
  {
    "Bright Lead",
    false,
    {
      // Envelope: Snappy but not too short
      {kParamGain, 80.},
      {kParamAttack, 5.},
      {kParamDecay, 280.},
      {kParamSustain, 60.},
      {kParamRelease, 320.},
      {kParamEnvVelocity, 45.},          // Expressive

      // Osc1: Saw
      {kParamWaveform, 1},               // Saw
      {kParamOsc1Level, 100.},
      {kParamOsc1Octave, 2},             // Root
      {kParamOsc1Detune, 0.},
      {kParamOsc1Pan, -12.},             // Slight left

      // Osc1 Unison: 3 voices for width
      {kParamOsc1UnisonVoices, 2},       // 3 voices
      {kParamOsc1UnisonDetune, 18.},
      {kParamOsc1UnisonWidth, 55.},
      {kParamOsc1UnisonBlend, 65.},

      // Osc2: Saw detuned for thickness
      {kParamOsc2Waveform, 1},           // Saw
      {kParamOsc2Octave, 2},             // Same octave
      {kParamOsc2Detune, 7.},            // Classic detune
      {kParamOsc2Level, 70.},
      {kParamOsc2Pan, 12.},              // Slight right

      // Osc2 Unison: 2 voices
      {kParamOsc2UnisonVoices, 1},       // 2 voices
      {kParamOsc2UnisonDetune, 12.},
      {kParamOsc2UnisonWidth, 45.},
      {kParamOsc2UnisonBlend, 55.},

      // Filter: Presence without harshness
      {kParamFilterEnable, 1},
      {kParamFilterCutoff, 2800.},
      {kParamFilterResonance, 38.},      // Nice peak
      {kParamFilterType, 0},             // Lowpass

      // LFO1: Subtle filter movement
      {kParamLFO1Enable, 1},
      {kParamLFO1Rate, 0.7},
      {kParamLFO1Sync, 0},
      {kParamLFO1Low, -18.},
      {kParamLFO1High, 18.},
      {kParamLFO1Waveform, 0},           // Sine
      {kParamLFO1Retrigger, 0},          // Free
      {kParamLFO1Destination, 1},        // Filter

      // LFO2: Off
      {kParamLFO2Enable, 0},

      // Delay: Short slapback for space
      {kParamDelayEnable, 1},
      {kParamDelayTime, 180.},
      {kParamDelaySync, 0},              // Free time
      {kParamDelayFeedback, 25.},
      {kParamDelayDry, 100.},
      {kParamDelayWet, 18.},             // Subtle
      {kParamDelayMode, 0},              // Stereo

      {END, 0}
    }
  },

  // ───────────────────────────────────────────────────────────────────────────
  // Preset 4: Lush Pad
  // Warm, enveloping pad with slow movement
  // Long attack/release, wide stereo, ping-pong delay for space
  // ───────────────────────────────────────────────────────────────────────────
  {
    "Lush Pad",
    false,
    {
      // Envelope: Slow and smooth
      {kParamGain, 75.},
      {kParamAttack, 380.},              // Slow swell
      {kParamDecay, 500.},
      {kParamSustain, 70.},
      {kParamRelease, 1400.},            // Long fade
      {kParamEnvVelocity, 15.},          // Minimal dynamics

      // Osc1: Saw with unison
      {kParamWaveform, 1},               // Saw
      {kParamOsc1Level, 85.},
      {kParamOsc1Octave, 2},
      {kParamOsc1Detune, -4.},
      {kParamOsc1Pan, -22.},             // Left

      // Osc1 Unison: 4 voices for width
      {kParamOsc1UnisonVoices, 3},       // 4 voices
      {kParamOsc1UnisonDetune, 28.},
      {kParamOsc1UnisonWidth, 75.},
      {kParamOsc1UnisonBlend, 60.},

      // Osc2: Triangle for warmth
      {kParamOsc2Waveform, 3},           // Triangle
      {kParamOsc2Octave, 3},             // +1 octave for air
      {kParamOsc2Detune, 5.},
      {kParamOsc2Level, 45.},            // Supporting
      {kParamOsc2Pan, 22.},              // Right

      // Osc2 Unison: 3 voices
      {kParamOsc2UnisonVoices, 2},       // 3 voices
      {kParamOsc2UnisonDetune, 22.},
      {kParamOsc2UnisonWidth, 65.},
      {kParamOsc2UnisonBlend, 55.},

      // Filter: Smooth the top end
      {kParamFilterEnable, 1},
      {kParamFilterCutoff, 3200.},
      {kParamFilterResonance, 22.},
      {kParamFilterType, 0},             // Lowpass

      // LFO1: Slow filter sweep
      {kParamLFO1Enable, 1},
      {kParamLFO1Rate, 0.12},            // Very slow
      {kParamLFO1Sync, 0},
      {kParamLFO1Low, -22.},
      {kParamLFO1High, 22.},
      {kParamLFO1Waveform, 1},           // Triangle
      {kParamLFO1Retrigger, 0},          // Free
      {kParamLFO1Destination, 1},        // Filter

      // LFO2: Subtle pitch drift
      {kParamLFO2Enable, 1},
      {kParamLFO2Rate, 0.08},
      {kParamLFO2Sync, 0},
      {kParamLFO2Low, -6.},
      {kParamLFO2High, 6.},
      {kParamLFO2Waveform, 0},           // Sine
      {kParamLFO2Retrigger, 0},
      {kParamLFO2Destination, 2},        // Pitch - subtle drift

      // Delay: Ping-pong for space
      {kParamDelayEnable, 1},
      {kParamDelayTime, 400.},
      {kParamDelaySync, 6},              // 1/4 note
      {kParamDelayFeedback, 42.},
      {kParamDelayDry, 85.},
      {kParamDelayWet, 32.},
      {kParamDelayMode, 1},              // Ping-Pong

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
