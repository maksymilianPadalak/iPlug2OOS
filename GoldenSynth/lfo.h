#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// LFO (Low Frequency Oscillator) - Modulation Source
// ═══════════════════════════════════════════════════════════════════════════════
// Standalone LFO implementation using Q DSP library oscillators for efficient,
// well-tested waveform generation.
// ═══════════════════════════════════════════════════════════════════════════════

#include <cmath>
#include <algorithm>

// Q DSP Library
#include <q/support/literals.hpp>
#include <q/support/phase.hpp>
#include <q/synth/sin_osc.hpp>
#include <q/synth/saw_osc.hpp>
#include <q/synth/square_osc.hpp>
#include <q/synth/triangle_osc.hpp>
#include <q/synth/noise_gen.hpp>

namespace q = cycfi::q;
using namespace q::literals;

// ═══════════════════════════════════════════════════════════════════════════════
// LFO (Low Frequency Oscillator) - Modulation Source
// ═══════════════════════════════════════════════════════════════════════════════
//
// WHAT IS AN LFO?
// An LFO is an oscillator running at sub-audio frequencies (typically 0.01-20 Hz)
// used to modulate other parameters over time. It creates movement and life in
// sounds that would otherwise be static.
//
// COMMON LFO DESTINATIONS:
// ┌─────────────────────┬────────────────────────────────────────────────────────┐
// │ Destination         │ Effect                                                 │
// ├─────────────────────┼────────────────────────────────────────────────────────┤
// │ Filter Cutoff       │ Wah-wah, dubstep wobble, evolving pads                 │
// │ Pitch               │ Vibrato (subtle), siren (extreme)                      │
// │ Amplitude           │ Tremolo, helicopter effect                             │
// │ Pulse Width         │ Classic analog PWM pad sound                           │
// │ FM Depth            │ Evolving FM timbres                                    │
// │ Wavetable Position  │ Morphing wavetable sounds                              │
// └─────────────────────┴────────────────────────────────────────────────────────┘
//
// WAVEFORM CHARACTERISTICS:
// ┌─────────────────────┬────────────────────────────────────────────────────────┐
// │ Waveform            │ Character                                              │
// ├─────────────────────┼────────────────────────────────────────────────────────┤
// │ Sine                │ Smooth, gentle, natural sounding modulation            │
// │ Triangle            │ Linear ramps, slightly more "pointed" than sine        │
// │ Saw Up              │ Rising ramp then drop - "building" feel                │
// │ Saw Down            │ Drop then rising ramp - "falling" feel                 │
// │ Square              │ Abrupt on/off switching - gated/trance effects         │
// │ Sample & Hold       │ Random stepped values - classic analog randomness      │
// └─────────────────────┴────────────────────────────────────────────────────────┘
//
// RATE RANGE:
//   0.01 Hz = 100 second cycle (glacially slow evolving pads)
//   0.1 Hz  = 10 second cycle (slow atmospheric sweeps)
//   1 Hz    = 1 second cycle (typical wobble bass rate)
//   5 Hz    = Fast wobble, approaching vibrato territory
//   10+ Hz  = Vibrato, tremolo, audio-rate effects
//
// RETRIGGER vs FREE-RUNNING:
//   - Free-running: LFO continues regardless of notes. Each note starts at a
//     random point in the LFO cycle. Good for evolving pads.
//   - Retrigger: LFO resets to phase 0 on each note-on. Every note has identical
//     modulation shape. Good for consistent bass/lead sounds.
//
// ═══════════════════════════════════════════════════════════════════════════════

// ═══════════════════════════════════════════════════════════════════════════════
// LFO WAVEFORM TYPES
// ═══════════════════════════════════════════════════════════════════════════════
enum class LFOWaveform
{
  Sine = 0,
  Triangle,
  SawUp,
  SawDown,
  Square,
  SampleAndHold,
  kNumWaveforms
};

// ═══════════════════════════════════════════════════════════════════════════════
// LFO TEMPO SYNC RATES
// ═══════════════════════════════════════════════════════════════════════════════
// Musical note divisions for syncing LFO to host tempo.
// D = Dotted (1.5x length, 2/3 frequency), T = Triplet (2/3 length, 1.5x frequency)
// ═══════════════════════════════════════════════════════════════════════════════
enum class LFOSyncRate
{
  Off = 0,      // Use manual Hz rate
  Bars4,        // 4/1 - 4 bars (16 beats)
  Bars2,        // 2/1 - 2 bars (8 beats)
  Bars1,        // 1/1 - 1 bar (4 beats)
  Half,         // 1/2 - half note (2 beats)
  HalfDotted,   // 1/2D - dotted half (3 beats)
  HalfTriplet,  // 1/2T - half triplet (4/3 beats)
  Quarter,      // 1/4 - quarter note (1 beat)
  QuarterDotted,// 1/4D - dotted quarter (1.5 beats)
  QuarterTriplet,// 1/4T - quarter triplet (2/3 beat)
  Eighth,       // 1/8 - eighth note (0.5 beats)
  EighthDotted, // 1/8D - dotted eighth (0.75 beats)
  EighthTriplet,// 1/8T - eighth triplet (1/3 beat)
  Sixteenth,    // 1/16 - sixteenth (0.25 beats)
  SixteenthDotted,// 1/16D - dotted sixteenth (0.375 beats)
  SixteenthTriplet,// 1/16T - sixteenth triplet (1/6 beat)
  ThirtySecond, // 1/32 - thirty-second (0.125 beats)
  kNumSyncRates
};

// ═══════════════════════════════════════════════════════════════════════════════
// TEMPO SYNC HELPER - Convert sync rate to Hz based on BPM
// ═══════════════════════════════════════════════════════════════════════════════
// Formula: Hz = BPM / 60 / beats_per_cycle
// Where beats_per_cycle is the number of quarter notes in one LFO cycle.
// ═══════════════════════════════════════════════════════════════════════════════
inline float SyncRateToHz(LFOSyncRate syncRate, float bpm)
{
  if (syncRate == LFOSyncRate::Off || bpm <= 0.0f)
    return 0.0f;

  // Beats per cycle for each sync rate (in quarter notes)
  float beatsPerCycle = 1.0f;

  switch (syncRate)
  {
    case LFOSyncRate::Bars4:           beatsPerCycle = 16.0f; break;      // 4 bars
    case LFOSyncRate::Bars2:           beatsPerCycle = 8.0f; break;       // 2 bars
    case LFOSyncRate::Bars1:           beatsPerCycle = 4.0f; break;       // 1 bar
    case LFOSyncRate::Half:            beatsPerCycle = 2.0f; break;       // 1/2
    case LFOSyncRate::HalfDotted:      beatsPerCycle = 3.0f; break;       // 1/2D (2 * 1.5)
    case LFOSyncRate::HalfTriplet:     beatsPerCycle = 4.0f / 3.0f; break;// 1/2T (2 * 2/3)
    case LFOSyncRate::Quarter:         beatsPerCycle = 1.0f; break;       // 1/4
    case LFOSyncRate::QuarterDotted:   beatsPerCycle = 1.5f; break;       // 1/4D
    case LFOSyncRate::QuarterTriplet:  beatsPerCycle = 2.0f / 3.0f; break;// 1/4T
    case LFOSyncRate::Eighth:          beatsPerCycle = 0.5f; break;       // 1/8
    case LFOSyncRate::EighthDotted:    beatsPerCycle = 0.75f; break;      // 1/8D
    case LFOSyncRate::EighthTriplet:   beatsPerCycle = 1.0f / 3.0f; break;// 1/8T
    case LFOSyncRate::Sixteenth:       beatsPerCycle = 0.25f; break;      // 1/16
    case LFOSyncRate::SixteenthDotted: beatsPerCycle = 0.375f; break;     // 1/16D
    case LFOSyncRate::SixteenthTriplet:beatsPerCycle = 1.0f / 6.0f; break;// 1/16T
    case LFOSyncRate::ThirtySecond:    beatsPerCycle = 0.125f; break;     // 1/32
    default: return 0.0f;
  }

  // Hz = beats per minute / 60 seconds / beats per cycle
  return (bpm / 60.0f) / beatsPerCycle;
}

// ═══════════════════════════════════════════════════════════════════════════════
// LFO DESTINATION TYPES
// ═══════════════════════════════════════════════════════════════════════════════
// Each LFO can modulate one of these destinations.
// Multiple LFOs can target the same destination (effects are additive).
//
// PER-OSCILLATOR ROUTING:
// Professional synths (Serum, Vital) allow independent modulation per oscillator.
// This enables techniques like:
//   - Independent vibrato on Osc1 vs Osc2
//   - Osc1 with slow WT morph + Osc2 with fast WT morph
//   - Detuned oscillators with different movement = more "alive" sound
//
// We provide both "global" destinations (affect both oscillators) and
// per-oscillator destinations for maximum flexibility.
// ═══════════════════════════════════════════════════════════════════════════════
enum class LFODestination
{
  Off = 0,          // No modulation

  // ─────────────────────────────────────────────────────────────────────────────
  // GLOBAL DESTINATIONS - Affect both oscillators or the entire voice
  // ─────────────────────────────────────────────────────────────────────────────
  Filter,           // Filter cutoff (±4 octaves at 100% depth)
  Pitch,            // Both oscillators pitch (±24 semitones at 100% depth)
  PulseWidth,       // Both oscillators pulse width (±45% at 100% depth)
  Amplitude,        // Voice amplitude / tremolo (0-100% at 100% depth)
  FMDepth,          // Both oscillators FM depth (±100% at 100% depth)
  WavetablePos,     // Both oscillators WT position (±50% at 100% depth)

  // ─────────────────────────────────────────────────────────────────────────────
  // PER-OSCILLATOR DESTINATIONS - Independent control for Osc1 and Osc2
  // ─────────────────────────────────────────────────────────────────────────────
  Osc1Pitch,        // Osc1 only pitch modulation (±24 semitones)
  Osc2Pitch,        // Osc2 only pitch modulation (±24 semitones)
  Osc1PulseWidth,   // Osc1 only pulse width (±45%)
  Osc2PulseWidth,   // Osc2 only pulse width (±45%)
  Osc1FMDepth,      // Osc1 only FM depth (±100%)
  Osc2FMDepth,      // Osc2 only FM depth (±100%)
  Osc1WTPos,        // Osc1 only wavetable position (±50%)
  Osc2WTPos,        // Osc2 only wavetable position (±50%)

  kNumDestinations
};

// ═══════════════════════════════════════════════════════════════════════════════
// LFO CLASS - Using Q Library Oscillators
// ═══════════════════════════════════════════════════════════════════════════════
// This LFO uses the Q library's oscillator functions and phase_iterator for
// efficient, well-tested waveform generation. At LFO rates (0.01-20 Hz),
// aliasing is not a concern, so we use the basic (non-bandwidth-limited)
// oscillators for optimal performance.
//
// Q LIBRARY COMPONENTS USED:
//   q::phase_iterator - Fixed-point 1.31 phase accumulator with automatic wrap
//   q::sin            - Lookup table sine oscillator (fast, accurate)
//   q::basic_triangle - Non-bandlimited triangle (no aliasing at LFO rates)
//   q::basic_saw      - Non-bandlimited sawtooth
//   q::basic_square   - Non-bandlimited square
//   q::white_noise_gen- Fast PRNG for Sample & Hold random values
//
// WHY USE Q LIBRARY FOR LFO?
//   1. Consistent API with audio oscillators (same phase_iterator pattern)
//   2. Well-tested implementations from production-grade DSP library
//   3. q::sin uses lookup table - faster than std::sin for audio
//   4. q::white_noise_gen is optimized for audio (known good distribution)
//
// ═══════════════════════════════════════════════════════════════════════════════
class LFO
{
public:
  // Constructor: Initialize phase iterator with default values
  LFO()
  {
    // Initialize phase iterator immediately so LFO works even before SetRate/SetSampleRate
    mPhase.set(q::frequency{static_cast<double>(mRate)}, mSampleRate);
  }

  void SetRate(float hz)
  {
    mRate = std::max(0.001f, hz);  // Minimum rate to avoid division issues
    UpdatePhaseIterator();
  }

  void SetWaveform(LFOWaveform waveform)
  {
    mWaveform = waveform;
  }

  void SetSampleRate(float sampleRate)
  {
    mSampleRate = sampleRate;
    UpdatePhaseIterator();
  }

  // Reset phase to 0 (for retrigger mode)
  void Reset()
  {
    mPhase = mPhase.begin();  // Reset to phase 0
    // Generate initial S&H value for consistent retriggered behavior
    mSHValue = mNoiseGen();
  }

  // Process one sample, returns value in range [-1, +1]
  float Process()
  {
    float output = 0.0f;

    // Check if we're about to complete a cycle (for S&H update)
    bool wasLastSample = mPhase.last();

    switch (mWaveform)
    {
      case LFOWaveform::Sine:
        // Q library sine uses lookup table - faster than std::sin
        output = q::sin(mPhase++);
        break;

      case LFOWaveform::Triangle:
        // Q library basic_triangle: non-bandlimited (perfect for LFO)
        output = q::basic_triangle(mPhase++);
        break;

      case LFOWaveform::SawUp:
        // Q library basic_saw: rising sawtooth -1 to +1
        output = q::basic_saw(mPhase++);
        break;

      case LFOWaveform::SawDown:
        // Inverted saw for falling sawtooth +1 to -1
        output = -q::basic_saw(mPhase++);
        break;

      case LFOWaveform::Square:
        // Q library basic_square: +1 for first half, -1 for second half
        output = q::basic_square(mPhase++);
        break;

      case LFOWaveform::SampleAndHold:
        // Return held value, update at cycle boundary
        output = mSHValue;
        ++mPhase;  // Advance phase but don't use its output
        break;

      default:
        output = q::sin(mPhase++);
        break;
    }

    // For S&H, sample new random value at start of each cycle
    if (wasLastSample && mWaveform == LFOWaveform::SampleAndHold)
    {
      mSHValue = mNoiseGen();
    }

    return output;
  }

private:
  // Update phase iterator when rate or sample rate changes
  void UpdatePhaseIterator()
  {
    if (mSampleRate > 0.0f)
    {
      mPhase.set(q::frequency{static_cast<double>(mRate)}, mSampleRate);
    }
  }

  // ─────────────────────────────────────────────────────────────────────────────
  // Q LIBRARY PHASE ITERATOR
  // ─────────────────────────────────────────────────────────────────────────────
  // Fixed-point 1.31 format: 32-bit unsigned where full cycle = 2^32.
  // Natural wraparound via integer overflow eliminates modulo operations.
  // Usage: q::sin(mPhase++) returns sine value and advances phase.
  // ─────────────────────────────────────────────────────────────────────────────
  q::phase_iterator mPhase;

  // ─────────────────────────────────────────────────────────────────────────────
  // Q LIBRARY WHITE NOISE GENERATOR
  // ─────────────────────────────────────────────────────────────────────────────
  // Fast PRNG for Sample & Hold. Uses optimized algorithm from musicdsp.org.
  // Output range: [-1, +1] with uniform distribution.
  // ─────────────────────────────────────────────────────────────────────────────
  q::white_noise_gen mNoiseGen;

  float mRate = 1.0f;               // Rate in Hz
  float mSampleRate = 48000.0f;     // Sample rate
  LFOWaveform mWaveform = LFOWaveform::Sine;
  float mSHValue = 0.0f;            // Held value for Sample & Hold
};
