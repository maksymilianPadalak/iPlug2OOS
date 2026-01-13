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
inline constexpr int kMaxPresetParams = 32;

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

  // ═══════════════════════════════════════════════════════════════════════════
  // AI: ADD NEW PRESETS BELOW THIS LINE
  // ═══════════════════════════════════════════════════════════════════════════
  //
  // Instructions:
  //   1. Copy the "Classic Lead" block above
  //   2. Paste it here (before the closing brace)
  //   3. Change the preset name string
  //   4. Modify parameter values as needed
  //   5. Always end with {END, 0}
  //
  // Quick reference for enum parameters:
  //   Waveform: 0=Sine, 1=Saw, 2=Square, 3=Triangle, 4=Pulse, 5=FM, 6=Wavetable
  //   Octave:   0=-2oct, 1=-1oct, 2=0oct, 3=+1oct, 4=+2oct
  //   Filter:   0=Off, 1=On | Type: 0=LP, 1=HP, 2=BP, 3=Notch
  //   LFO Dest: 0=Off, 1=Filter, 2=Pitch, 3=PW, 4=Amp, 5=FM, 6=Wavetable
  //
  // ═══════════════════════════════════════════════════════════════════════════

};

// ═══════════════════════════════════════════════════════════════════════════════
// AUTO-DERIVED CONSTANTS - Do not edit!
// ═══════════════════════════════════════════════════════════════════════════════

inline constexpr int kPresetCount = sizeof(kPresetDefs) / sizeof(kPresetDefs[0]);

static_assert(kPresetCount >= 1, "Must have at least one preset");
static_assert(kPresetCount <= 128, "Maximum 128 presets supported");
