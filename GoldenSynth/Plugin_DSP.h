#pragma once

#include <cmath>
#include <array>
#include <algorithm>

// iPlug2 Synth Infrastructure
#include "MidiSynth.h"

// Q DSP Library
#include <q/support/literals.hpp>
#include <q/support/phase.hpp>
#include <q/synth/sin_osc.hpp>
#include <q/synth/saw_osc.hpp>
#include <q/synth/square_osc.hpp>
#include <q/synth/triangle_osc.hpp>
#include <q/synth/pulse_osc.hpp>
#include <q/synth/envelope_gen.hpp>

using namespace iplug;
namespace q = cycfi::q;
using namespace q::literals;

// ═══════════════════════════════════════════════════════════════════════════════
// Q LIBRARY OSCILLATORS - PolyBLEP/PolyBLAMP Anti-Aliasing
// ═══════════════════════════════════════════════════════════════════════════════
//
// WHAT ARE q::sin, q::saw, q::square, q::triangle?
// These are Q library's real-time oscillators that use PolyBLEP (Polynomial
// Band-Limited Step) anti-aliasing. They generate waveforms sample-by-sample
// with minimal aliasing at low CPU cost.
//
// HOW POLYBLEP WORKS:
// 1. Generate a "naive" waveform (e.g., saw = 2*phase - 1)
// 2. Detect discontinuities (jumps) in the waveform
// 3. Apply a polynomial correction near transitions to "round the corners"
//
// The correction polynomial (2nd order) is applied for ~2 samples around each
// discontinuity: `t + t - t*t - 1` smooths the step function.
//
// POLYBLAMP (for triangle):
// Triangle waves don't have amplitude discontinuities, but have SLOPE
// discontinuities (sharp corners). PolyBLAMP (Band-Limited rAMP) is the
// integrated version of PolyBLEP, using a cubic polynomial to smooth corners.
//
// POLYBLEP vs WAVETABLE (COMPARISON):
// ┌─────────────────────┬──────────────────────┬──────────────────────┐
// │ Aspect              │ PolyBLEP (Q library) │ Wavetable (ours)     │
// ├─────────────────────┼──────────────────────┼──────────────────────┤
// │ CPU per sample      │ Very low (O(1))      │ Low (table lookup)   │
// │ Memory              │ None                 │ ~1MB for mipmaps     │
// │ Anti-aliasing       │ Good (some residual) │ Perfect (band-limit) │
// │ High frequencies    │ Some aliasing        │ No aliasing          │
// │ Morphing support    │ No                   │ Yes (wavetable)      │
// │ Best for            │ Classic analog sound │ Complex/morphing     │
// └─────────────────────┴──────────────────────┴──────────────────────┘
//
// PHASE ITERATOR (q::phase_iterator):
// Q library represents phase as a 32-bit unsigned integer (fixed-point 1.31).
// - Full cycle = 2^32 (natural wraparound via integer overflow)
// - Step = (2^32 × frequency) / sampleRate
// - `mPhase++` advances phase by one step and returns oscillator output
// - `mPhase.set(freq, sps)` updates the step size for a new frequency
//
// REFERENCES:
// - Välimäki & Pekonen, "Perceptually informed synthesis of bandlimited
//   classical waveforms using integrated polynomial interpolation" (2012)
// - Q library source: q/synth/saw_osc.hpp, q/utility/antialiasing.hpp
//
// ═══════════════════════════════════════════════════════════════════════════════

// ═══════════════════════════════════════════════════════════════════════════════
// PULSE WIDTH MODULATION (PWM)
// ═══════════════════════════════════════════════════════════════════════════════
//
// WHAT IS A PULSE WAVE?
// A pulse wave alternates between +1 and -1 with variable "duty cycle" (pulse width).
// Duty cycle = percentage of time the wave is "high" per cycle.
//
//   50% (square):  ████████________  ← Only odd harmonics (hollow, clarinet)
//   25% (narrow):  ████____________  ← All harmonics (brighter, richer)
//   75% (wide):    ████████████____  ← All harmonics (same as 25%, inverted)
//
// HARMONIC CONTENT BY DUTY CYCLE:
// ┌────────────┬─────────────────────────────────────┬────────────────────────┐
// │ Duty Cycle │ Harmonics Present                   │ Character              │
// ├────────────┼─────────────────────────────────────┼────────────────────────┤
// │ 50%        │ Odd only (1, 3, 5, 7...)            │ Hollow, clarinet-like  │
// │ 25% / 75%  │ All except every 4th (4, 8, 12...)  │ Brighter, richer       │
// │ 12.5%      │ All except every 8th                │ Even brighter          │
// │ 10% / 90%  │ Approaches impulse train            │ Very bright, reedy     │
// │ 5% / 95%   │ Near impulse                        │ Thin, buzzy            │
// └────────────┴─────────────────────────────────────┴────────────────────────┘
//
// WHY 5-95% LIMITS?
// At 0% or 100%, the wave is DC (silence). Hardware synths typically limit to
// 5-95% to avoid silence and the "ripping" sound as the waveform collapses.
// This is an industry-standard range used by Roland, Moog, Sequential, etc.
//
// PWM EFFECT (Pulse Width Modulation):
// Slowly modulating the pulse width with an LFO creates a shimmering, chorus-like
// effect as the harmonic content constantly shifts. This is the classic analog
// synth pad sound heard in countless recordings.
//
// Classic PWM synths: Roland Juno-60/106, Sequential Prophet-5, Oberheim OB-X
//
// IMPLEMENTATION NOTES:
// - Uses Q library's pulse_osc with PolyBLEP anti-aliasing on both edges
// - Pulse width parameter is smoothed (~5ms) to prevent clicks during modulation
// - Without smoothing, sudden width changes cause the falling edge to jump
//   mid-cycle, creating an audible discontinuity
//
// KNOWN LIMITATION:
// At very high frequencies (>3kHz fundamental) combined with narrow pulse widths
// (<10%), both rising and falling edges can occur within a single sample. The
// PolyBLEP corrections may overlap, causing subtle artifacts. This edge case is
// rarely musically relevant - most PWM use is in the bass-to-mid frequency range.
//
// ═══════════════════════════════════════════════════════════════════════════════

// ═══════════════════════════════════════════════════════════════════════════════
// FM SYNTHESIS (Frequency Modulation) - DX7-Style Implementation
// ═══════════════════════════════════════════════════════════════════════════════
//
// WHAT IS FM SYNTHESIS?
// Instead of filtering harmonics (subtractive) or adding them (additive), FM
// creates harmonics by modulating one oscillator's frequency with another:
//
//   Modulator ──┐
//               ├──► Carrier ──► Output
//   (hidden)    │    (audible)
//               └── modulates frequency
//
// FORMULA (actually Phase Modulation, but called FM):
//   output = sin(carrierPhase + depth * sin(modulatorPhase))
//
// The modulator's output is added to the carrier's PHASE, not frequency directly.
// This is mathematically equivalent to FM but easier to implement. The DX7 uses
// the same technique (PM labeled as FM).
//
// KEY PARAMETERS (DX7-STYLE COARSE + FINE):
// ┌───────────────┬────────────────────────────────────────────────────────────┐
// │ Parameter     │ Effect                                                     │
// ├───────────────┼────────────────────────────────────────────────────────────┤
// │ Ratio (Coarse)│ Modulator freq = Carrier freq × Ratio                      │
// │               │ Preset harmonic values: 0.5, 1, 2, 3, 4, 5, 6, 7, 8        │
// │               │ Integer ratios produce harmonic partials (musical tones)   │
// ├───────────────┼────────────────────────────────────────────────────────────┤
// │ Fine          │ Percentage offset from coarse ratio (-50% to +50%)         │
// │               │ Final ratio = Coarse × (1 + Fine)                          │
// │               │ At 0%: Pure harmonic sound                                 │
// │               │ Non-zero: Inharmonic partials for bells, metallic sounds   │
// │               │ Example: Coarse 2:1 + Fine +20% = effective ratio 2.4      │
// ├───────────────┼────────────────────────────────────────────────────────────┤
// │ Depth (Index) │ How much the modulator affects the carrier                 │
// │               │ Low (0-20%): Subtle warmth, adds a few harmonics           │
// │               │ Medium (20-50%): Rich, evolving timbres                    │
// │               │ High (50-100%): Bright, metallic, aggressive               │
// │               │ Internally scaled to 0-4π radians (modulation index ~12)   │
// └───────────────┴────────────────────────────────────────────────────────────┘
//
// SOUND DESIGN GUIDE:
// ┌─────────────────────────────────────────────────────────────────────────────┐
// │ Sound Type        │ Coarse │ Fine  │ Depth │ Notes                         │
// ├───────────────────┼────────┼───────┼───────┼───────────────────────────────┤
// │ Electric Piano    │ 2:1    │ 0%    │ 30%   │ Classic Rhodes-like tone      │
// │ Bright Bell       │ 2:1    │ 0%    │ 70%   │ Church bell, chime            │
// │ Tubular Bell      │ 2:1    │ +41%  │ 60%   │ Fine ≈ √2 ratio (inharmonic)  │
// │ Gong / Metallic   │ 3:1    │ +17%  │ 80%   │ Complex inharmonic spectrum   │
// │ Bass Enhancement  │ 0.5:1  │ 0%    │ 40%   │ Sub-harmonic warmth           │
// │ Hollow/Clarinet   │ 1:1    │ 0%    │ 25%   │ Odd harmonics emphasis        │
// │ Aggressive Lead   │ 3:1    │ 0%    │ 90%   │ Bright, cutting tone          │
// └─────────────────────────────────────────────────────────────────────────────┘
//
// FM vs SUBTRACTIVE:
// ┌─────────────────┬──────────────────────┬──────────────────────┐
// │ Aspect          │ FM                   │ Subtractive          │
// ├─────────────────┼──────────────────────┼──────────────────────┤
// │ Harmonics       │ Created by modulation│ Filtered from rich   │
// │ Character       │ Bell, glass, metallic│ Warm, analog         │
// │ CPU cost        │ Very low (just sin)  │ Higher (filters)     │
// │ Classic sounds  │ E-piano, bells, bass │ Pads, leads, brass   │
// └─────────────────┴──────────────────────┴──────────────────────┘
//
// CLASSIC FM SYNTHS:
// Yamaha DX7 (1983), Yamaha TX81Z, Native Instruments FM8, Dexed (open source)
//
// IMPLEMENTATION NOTES:
// - Uses two sine oscillators: carrier (audible) and modulator (hidden)
// - Coarse ratio sets harmonic relationship (enum for clean UI)
// - Fine ratio allows inharmonic detuning (percentage offset)
// - Combined ratio = Coarse × (1 + Fine/100)
// - Depth controls modulation index (scaled to 0-4π radians internally)
// - No anti-aliasing needed - pure sine waves have no harmonics to alias
// - All parameters are smoothed to prevent clicks during modulation
//
// ═══════════════════════════════════════════════════════════════════════════════

// ═══════════════════════════════════════════════════════════════════════════════
// WAVETABLE OSCILLATOR - Band-Limited with Mip Levels
// ═══════════════════════════════════════════════════════════════════════════════
//
// WHAT IS A WAVETABLE?
// A wavetable stores pre-computed waveform cycles in memory. Instead of calculating
// waveforms mathematically each sample (expensive), we look up values from a table
// (fast). This also allows complex waveforms that can't be expressed as simple formulas.
//
// WHY MIPMAPS?
// At high frequencies, waveform harmonics can exceed Nyquist (sampleRate/2), causing
// aliasing artifacts. Mipmaps store multiple versions of the wavetable with progressively
// fewer harmonics. We select the appropriate mip level based on playback frequency.
//
// WHY STATIC ALLOCATION?
// WebAssembly (WASM) has a limited stack size (~64KB by default). A 1MB wavetable
// would overflow the stack if allocated as a local variable. Static storage places
// the data in the binary's data segment, avoiding stack limitations.
//
// ═══════════════════════════════════════════════════════════════════════════════

// Wavetable dimensions
constexpr int kWavetableSize = 2048;      // Samples per cycle (power of 2 for fast wrapping)
constexpr int kWavetableFrames = 16;      // Morph positions (more = smoother morphing)
constexpr int kNumMipLevels = 8;          // 8 mip levels: 128→64→32→16→8→4→2→1 harmonics
constexpr float kWavetableSizeF = static_cast<float>(kWavetableSize);

// Morph configuration - 4 classic waveforms
constexpr int kNumWaveShapes = 4;         // Sine, Triangle, Saw, Square

// Memory: 2048 × 16 × 8 × 4 bytes = 1MB (static allocation, not stack)

// ═══════════════════════════════════════════════════════════════════════════════
// DSP UTILITIES
// ═══════════════════════════════════════════════════════════════════════════════

// Calculate one-pole smoothing coefficient from time constant
// This creates a sample-rate independent smoothing filter.
// Formula: coeff = 1 - e^(-1 / (time * sampleRate))
// At each sample: smoothed += coeff * (target - smoothed)
inline float calcSmoothingCoeff(float timeSeconds, float sampleRate)
{
  return 1.0f - std::exp(-1.0f / (timeSeconds * sampleRate));
}

// ═══════════════════════════════════════════════════════════════════════════════
// DENORMAL PREVENTION
// ═══════════════════════════════════════════════════════════════════════════════
// Denormals are tiny floating point numbers (< 1e-38) that occur in filter
// feedback loops as signals decay. Processing denormals is 10-100x slower
// than normal floats, causing audio dropouts. We flush them to zero.
constexpr float kDenormalThreshold = 1e-15f;

inline float flushDenormal(float x)
{
  // Simple denormal flush - if |x| < threshold, return 0
  // More sophisticated approaches exist (FTZ/DAZ CPU flags) but this is portable
  return (std::abs(x) < kDenormalThreshold) ? 0.0f : x;
}

// Soft saturation using fast tanh approximation
// Creates warm, analog-style clipping instead of harsh digital clipping
inline float softSaturate(float x)
{
  // Pade approximant of tanh, accurate to ~0.1% for |x| < 3
  // Much faster than std::tanh
  if (x > 3.0f) return 1.0f;
  if (x < -3.0f) return -1.0f;
  float x2 = x * x;
  return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

// ═══════════════════════════════════════════════════════════════════════════════
// RESONANT FILTER - Using Q Library's Biquad Filters
// ═══════════════════════════════════════════════════════════════════════════════
//
// This filter uses Q library's biquad implementations (Audio-EQ Cookbook).
// - Lowpass, Highpass: True 2nd-order biquad responses
// - Bandpass: Constant Skirt Gain (CSG) - peak rises with Q
// - Notch: Complementary filter (input - bandpass_cpg)
//
// WHY BIQUADS WITH SMOOTHING?
// Biquads can cause zipper noise when coefficients change abruptly. We solve
// this by smoothing the cutoff and resonance parameters before updating
// coefficients. This gives us:
// - Full 20Hz-20kHz frequency range (no stability limits)
// - True filter responses for LP/HP/BP
// - Click-free parameter automation
//
// NOTCH IMPLEMENTATION:
// We use the complementary filter technique: notch = input - bandpass.
// The bandpass_cpg (Constant Peak Gain) ensures unity gain at center frequency,
// so subtraction creates a true notch with full attenuation at cutoff.
//
// WHAT IS Q (RESONANCE)?
// Q controls the "sharpness" of the filter at the cutoff frequency.
// - Q = 0.707: Butterworth (maximally flat passband)
// - Q = 1-5: Moderate resonance, musical "sweep" sound
// - Q = 10+: Sharp peak, near self-oscillation
//
// ═══════════════════════════════════════════════════════════════════════════════

#include <q/fx/biquad.hpp>

enum class FilterType
{
  Lowpass = 0,
  Highpass,
  Bandpass,
  Notch,
  kNumFilterTypes
};

class ResonantFilter
{
public:
  void SetSampleRate(float sampleRate)
  {
    mSampleRate = sampleRate;
    mSmoothCoeff = calcSmoothingCoeff(0.005f, sampleRate);  // 5ms smoothing

    // Biquads are stable up to Nyquist, but we cap at 0.45 * sampleRate
    // to avoid numerical issues very close to Nyquist
    mMaxCutoffHz = sampleRate * 0.45f;  // ~21.6kHz at 48kHz

    // Reinitialize all filters at new sample rate
    UpdateAllFilters();
  }

  // Set cutoff frequency in Hz (20 - 20000)
  void SetCutoff(float freqHz)
  {
    mTargetCutoffHz = std::max(20.0f, std::min(mMaxCutoffHz, freqHz));
  }

  // Set resonance (0.0 - 1.0) - mapped to Q range
  void SetResonance(float resonance)
  {
    // Map 0-1 resonance to Q range: 0.5 (gentle) to 15 (near self-oscillation)
    // Using exponential mapping for more musical feel
    mTargetResonance = std::max(0.0f, std::min(1.0f, resonance));
  }

  void SetType(FilterType type)
  {
    if (mType != type)
    {
      mType = type;
      // Reset filter states when switching types to avoid artifacts
      ResetFilterStates();
      // Ensure the new filter type has current coefficients
      UpdateAllFilters();
    }
  }

  void Reset()
  {
    ResetFilterStates();
    mCurrentCutoffHz = mTargetCutoffHz;
    mCurrentResonance = mTargetResonance;
    // Sync biquad coefficients to current values
    // Without this, the filter uses stale coefficients after note trigger
    UpdateAllFilters();
  }

  // Process a single sample through the filter
  float Process(float input)
  {
    // ─────────────────────────────────────────────────────────────────────────
    // SMOOTH PARAMETERS
    // Prevents zipper noise by interpolating towards target values
    // ─────────────────────────────────────────────────────────────────────────
    bool needsUpdate = false;

    float cutoffDiff = mTargetCutoffHz - mCurrentCutoffHz;
    if (std::abs(cutoffDiff) > 0.1f)
    {
      mCurrentCutoffHz += mSmoothCoeff * cutoffDiff;
      needsUpdate = true;
    }

    float resoDiff = mTargetResonance - mCurrentResonance;
    if (std::abs(resoDiff) > 0.001f)
    {
      mCurrentResonance += mSmoothCoeff * resoDiff;
      needsUpdate = true;
    }

    // Recalculate coefficients for ALL filters when parameters change
    // This ensures switching filter types always has correct coefficients
    if (needsUpdate)
    {
      UpdateAllFilters();
    }

    // ─────────────────────────────────────────────────────────────────────────
    // FILTER PROCESSING
    // Each filter type uses its own biquad with correct coefficients
    // ─────────────────────────────────────────────────────────────────────────
    float output;
    switch (mType)
    {
      case FilterType::Lowpass:
        output = mLowpass(input);
        FlushBiquadDenormals(mLowpass);
        break;

      case FilterType::Highpass:
        output = mHighpass(input);
        FlushBiquadDenormals(mHighpass);
        break;

      case FilterType::Bandpass:
        output = mBandpass(input);
        FlushBiquadDenormals(mBandpass);
        break;

      case FilterType::Notch:
        // Notch = input - bandpass_cpg (complementary filter technique)
        // bandpass_cpg has unity gain at center frequency, so:
        //   At cutoff: output = input - input = 0 (full attenuation)
        //   Away from cutoff: output ≈ input (passthrough)
        {
          float bpOut = mNotchBandpass(input);
          FlushBiquadDenormals(mNotchBandpass);
          output = input - bpOut;
        }
        break;

      default:
        output = mLowpass(input);
        break;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // SOFT SATURATION
    // At high resonance, filter can exceed unity gain. Apply gentle saturation
    // to prevent harsh digital clipping while preserving the character.
    // ─────────────────────────────────────────────────────────────────────────
    if (mCurrentResonance > 0.5f)
    {
      float satAmount = (mCurrentResonance - 0.5f) * 2.0f;
      float dry = output;
      float wet = softSaturate(output * 1.5f);
      output = dry + satAmount * (wet - dry);
    }

    return output;
  }

private:
  // Map resonance (0-1) to Q factor using exponential curve
  // Formula: Q = 0.5 × 30^resonance
  //   0.0 → Q = 0.5   (very gentle, below Butterworth)
  //   0.5 → Q ≈ 2.74  (moderate resonance)
  //   1.0 → Q = 15    (high resonance, near self-oscillation)
  float ResonanceToQ(float resonance) const
  {
    return 0.5f * std::pow(30.0f, resonance);
  }

  void UpdateAllFilters()
  {
    if (mSampleRate <= 0.0f) return;

    q::frequency freq{mCurrentCutoffHz};
    double qVal = ResonanceToQ(mCurrentResonance);

    mLowpass.config(freq, mSampleRate, qVal);
    mHighpass.config(freq, mSampleRate, qVal);
    mBandpass.config(freq, mSampleRate, qVal);
    mNotchBandpass.config(freq, mSampleRate, qVal);
  }

  void ResetFilterStates()
  {
    mLowpass.x1 = mLowpass.x2 = mLowpass.y1 = mLowpass.y2 = 0.0f;
    mHighpass.x1 = mHighpass.x2 = mHighpass.y1 = mHighpass.y2 = 0.0f;
    mBandpass.x1 = mBandpass.x2 = mBandpass.y1 = mBandpass.y2 = 0.0f;
    mNotchBandpass.x1 = mNotchBandpass.x2 = mNotchBandpass.y1 = mNotchBandpass.y2 = 0.0f;
  }

  void FlushBiquadDenormals(q::biquad& bq)
  {
    bq.x1 = flushDenormal(bq.x1);
    bq.x2 = flushDenormal(bq.x2);
    bq.y1 = flushDenormal(bq.y1);
    bq.y2 = flushDenormal(bq.y2);
  }

  float mSampleRate = 48000.0f;
  float mMaxCutoffHz = 20000.0f;
  float mTargetCutoffHz = 10000.0f;
  float mTargetResonance = 0.0f;
  float mCurrentCutoffHz = 10000.0f;
  float mCurrentResonance = 0.0f;
  float mSmoothCoeff = 0.01f;

  // Q library biquad filters - one for each type
  // Initialized with default values, reconfigured in SetSampleRate()
  q::lowpass mLowpass{q::frequency{1000.0f}, 48000.0f, 0.707};
  q::highpass mHighpass{q::frequency{1000.0f}, 48000.0f, 0.707};
  q::bandpass_csg mBandpass{q::frequency{1000.0f}, 48000.0f, 0.707};
  // Notch uses bandpass_cpg (Constant Peak Gain) + subtraction
  // CPG has unity gain at center frequency, so input - bandpass = true notch
  // (CSG would have variable peak gain, causing incomplete notch attenuation)
  q::bandpass_cpg mNotchBandpass{q::frequency{1000.0f}, 48000.0f, 0.707};

  FilterType mType = FilterType::Lowpass;
};

class WavetableOscillator
{
public:
  // Mipmapped wavetable: [mipLevel][frame][sample]
  using MipLevel = std::array<std::array<float, kWavetableSize>, kWavetableFrames>;
  using WavetableData = std::array<MipLevel, kNumMipLevels>;

  void SetWavetable(const WavetableData* table) { mTable = table; }

  void SetSampleRate(float sampleRate)
  {
    mSampleRate = sampleRate;
    mNyquist = sampleRate * 0.5f;
    // Position smoothing: 10ms time constant for glitch-free morphing
    // This prevents audible clicks when the WT Position knob is moved quickly
    mSmoothCoeff = calcSmoothingCoeff(0.01f, sampleRate);
  }

  void SetFrequency(float freq, float sampleRate)
  {
    mPhaseInc = freq / sampleRate;
    mFrequency = freq;
    mNyquist = sampleRate * 0.5f;
  }

  void SetPosition(float pos)
  {
    mTargetPosition = std::max(0.0f, std::min(1.0f, pos)) * (kWavetableFrames - 1);
  }

  void Reset() { mPhase = 0.0f; }

  float Process()
  {
    if (!mTable) return 0.0f;

    // ─────────────────────────────────────────────────────────────────────────
    // STEP 1: Smooth the morph position (one-pole lowpass filter)
    // This prevents clicks when the position changes rapidly
    // ─────────────────────────────────────────────────────────────────────────
    mPosition += mSmoothCoeff * (mTargetPosition - mPosition);

    // ─────────────────────────────────────────────────────────────────────────
    // STEP 2: Calculate mip level based on playback frequency and Nyquist
    // We select the mip level that has enough harmonics without aliasing.
    // Formula: mip = log2(baseHarmonics * frequency / Nyquist)
    //
    // Example at 48kHz (Nyquist = 24kHz), 128 base harmonics:
    //   100Hz: log2(128 * 100 / 24000) = log2(0.53) ≈ -0.9 → mip 0 (128 harmonics)
    //   440Hz: log2(128 * 440 / 24000) = log2(2.35) ≈ 1.2 → mip 1 (64 harmonics)
    //   2kHz:  log2(128 * 2000 / 24000) = log2(10.7) ≈ 3.4 → mip 3 (16 harmonics)
    //
    // DESIGN NOTE: We use floor() for mip selection, which prioritizes brightness
    // over perfect anti-aliasing. At transition points (e.g., 440Hz using mip 1),
    // some harmonics may slightly exceed Nyquist, but the crossfade to the next
    // mip level (fewer harmonics) attenuates them. ceil() would be alias-free but darker.
    // ─────────────────────────────────────────────────────────────────────────
    constexpr float kBaseHarmonics = 128.0f;
    float mipFloat = std::log2(std::max(1.0f, kBaseHarmonics * mFrequency / mNyquist));
    mipFloat = std::max(0.0f, std::min(mipFloat, static_cast<float>(kNumMipLevels - 1)));

    int mip0 = static_cast<int>(mipFloat);  // floor - prioritizes brightness
    int mip1 = std::min(mip0 + 1, kNumMipLevels - 1);
    float mipFrac = mipFloat - mip0;

    // ─────────────────────────────────────────────────────────────────────────
    // STEP 3: Calculate frame indices for morph interpolation
    // ─────────────────────────────────────────────────────────────────────────
    int frame0 = static_cast<int>(mPosition);
    int frame1 = std::min(frame0 + 1, kWavetableFrames - 1);
    float frameFrac = mPosition - frame0;

    // ─────────────────────────────────────────────────────────────────────────
    // STEP 4: Calculate sample indices with wrapping
    // Using bitwise AND for fast modulo (works because size is power of 2)
    // ─────────────────────────────────────────────────────────────────────────
    float samplePos = mPhase * kWavetableSizeF;
    int idx0 = static_cast<int>(samplePos) & (kWavetableSize - 1);
    int idx1 = (idx0 + 1) & (kWavetableSize - 1);
    float sampleFrac = samplePos - std::floor(samplePos);

    // ─────────────────────────────────────────────────────────────────────────
    // STEP 5: Trilinear interpolation (mip × frame × sample)
    // We interpolate along 3 axes for smooth output:
    // - Between adjacent samples (removes stepping artifacts)
    // - Between adjacent frames (smooth morphing)
    // - Between adjacent mip levels (smooth anti-aliasing transition)
    //
    // Linear interpolation formula: result = a + frac * (b - a)
    // Quality note: Linear gives ~-40dB THD. Cubic Hermite would give ~-80dB.
    // ─────────────────────────────────────────────────────────────────────────
    auto sampleFrame = [&](int mip, int frame) {
      const auto& t = (*mTable)[mip][frame];
      return t[idx0] + sampleFrac * (t[idx1] - t[idx0]);
    };

    // Interpolate within each mip level (frame + sample)
    float s_mip0_f0 = sampleFrame(mip0, frame0);
    float s_mip0_f1 = sampleFrame(mip0, frame1);
    float s_mip0 = s_mip0_f0 + frameFrac * (s_mip0_f1 - s_mip0_f0);

    float s_mip1_f0 = sampleFrame(mip1, frame0);
    float s_mip1_f1 = sampleFrame(mip1, frame1);
    float s_mip1 = s_mip1_f0 + frameFrac * (s_mip1_f1 - s_mip1_f0);

    // Final mip interpolation
    float sample = s_mip0 + mipFrac * (s_mip1 - s_mip0);

    // ─────────────────────────────────────────────────────────────────────────
    // STEP 6: Advance phase accumulator
    // Phase wraps from 0.0 to 1.0, representing one cycle of the waveform
    // Using while loop handles edge case where phaseInc > 1.0 (freq > sampleRate)
    // ─────────────────────────────────────────────────────────────────────────
    mPhase += mPhaseInc;
    while (mPhase >= 1.0f) mPhase -= 1.0f;

    return sample;
  }

private:
  const WavetableData* mTable = nullptr;
  float mSampleRate = 48000.0f;
  float mNyquist = 24000.0f;  // Half sample rate, for mip level calculation
  float mFrequency = 440.0f;
  float mSmoothCoeff = 0.01f;
  float mPhase = 0.0f;
  float mPhaseInc = 0.0f;
  float mPosition = 0.0f;
  float mTargetPosition = 0.0f;
};

// ═══════════════════════════════════════════════════════════════════════════════
// WAVETABLE GENERATOR - Band-Limited Additive Synthesis
// ═══════════════════════════════════════════════════════════════════════════════
//
// WHY ADDITIVE SYNTHESIS?
// Naive waveforms (e.g., saw = 2*phase - 1) contain infinite harmonics, causing
// aliasing at high frequencies. Additive synthesis builds waveforms from individual
// sine harmonics, allowing us to stop before reaching Nyquist.
//
// FOURIER SERIES REFERENCE:
// - Saw:      Σ sin(n·x) / n           (all harmonics)
// - Square:   Σ sin(n·x) / n           (odd harmonics only: 1, 3, 5, ...)
// - Triangle: Σ (-1)^k · sin(n·x) / n² (odd harmonics, alternating sign)
//
// ═══════════════════════════════════════════════════════════════════════════════

class WavetableGenerator
{
public:
  using WavetableData = WavetableOscillator::WavetableData;
  static constexpr float kPi = 3.14159265359f;
  static constexpr float k2Pi = 2.0f * kPi;

  // Harmonics per mip level - halves each octave to stay below Nyquist
  // Mip 0 has 128 harmonics (used for fundamentals up to ~187Hz at 48kHz)
  // Formula: maxFreq = Nyquist / harmonics = 24000 / 128 ≈ 187Hz
  // Each mip halves harmonics: mip1=64, mip2=32, ... mip7=1
  static constexpr int kBaseHarmonics = 128;
  static int GetMaxHarmonics(int mipLevel)
  {
    return std::max(1, kBaseHarmonics >> mipLevel);
  }

  // ─────────────────────────────────────────────────────────────────────────────
  // BAND-LIMITED WAVEFORM GENERATORS
  // Each function computes the Fourier series sum for one sample position
  // ─────────────────────────────────────────────────────────────────────────────

  // Sawtooth: sum of all harmonics with 1/n amplitude rolloff
  // Fourier: saw(x) = -(2/π) · Σ sin(n·x)/n, n=1 to ∞
  static float GenerateBandLimitedSaw(float phase, int maxHarmonics)
  {
    float sample = 0.0f;
    for (int h = 1; h <= maxHarmonics; h++)
    {
      sample -= std::sin(phase * k2Pi * h) / static_cast<float>(h);
    }
    return sample * (2.0f / kPi);  // Normalize to [-1, 1]
  }

  // Square: odd harmonics only (1, 3, 5, ...) with 1/n amplitude
  // Fourier: square(x) = (4/π) · Σ sin(n·x)/n, n=1,3,5,...
  static float GenerateBandLimitedSquare(float phase, int maxHarmonics)
  {
    float sample = 0.0f;
    for (int h = 1; h <= maxHarmonics; h += 2)  // Odd harmonics only
    {
      sample += std::sin(phase * k2Pi * h) / static_cast<float>(h);
    }
    return sample * (4.0f / kPi);  // Normalize to [-1, 1]
  }

  // Triangle: odd harmonics with 1/n² rolloff and alternating signs
  // Fourier: tri(x) = (8/π²) · Σ (-1)^k · sin(n·x)/n², n=1,3,5,..., k=(n-1)/2
  static float GenerateBandLimitedTriangle(float phase, int maxHarmonics)
  {
    float sample = 0.0f;
    for (int h = 1; h <= maxHarmonics; h += 2)  // Odd harmonics only
    {
      // Alternating sign: +1 for h=1, -1 for h=3, +1 for h=5, ...
      float sign = ((h - 1) / 2 % 2 == 0) ? 1.0f : -1.0f;
      sample += sign * std::sin(phase * k2Pi * h) / static_cast<float>(h * h);
    }
    return sample * (8.0f / (kPi * kPi));  // Normalize to [-1, 1]
  }

  // ─────────────────────────────────────────────────────────────────────────────
  // WAVETABLE GENERATION
  // Creates a complete mipmapped wavetable with morphing between 4 shapes
  // ─────────────────────────────────────────────────────────────────────────────

  static const WavetableData& GenerateBasicShapes()
  {
    // STATIC ALLOCATION: Table lives in data segment, not stack!
    // This is critical for WASM where stack is limited to ~64KB
    static WavetableData table{};
    static bool generated = false;

    if (generated) return table;  // Return cached reference (no copy)
    generated = true;

    // Generate each mip level with appropriate harmonic content
    for (int mip = 0; mip < kNumMipLevels; mip++)
    {
      int maxHarmonics = GetMaxHarmonics(mip);

      for (int frame = 0; frame < kWavetableFrames; frame++)
      {
        // Map frame index to morph position [0, 1]
        float t = static_cast<float>(frame) / (kWavetableFrames - 1);

        for (int i = 0; i < kWavetableSize; i++)
        {
          float phase = static_cast<float>(i) / kWavetableSize;

          // Generate all 4 band-limited waveforms at this phase
          float sine = std::sin(phase * k2Pi);  // Sine has no harmonics to limit
          float triangle = GenerateBandLimitedTriangle(phase, maxHarmonics);
          float saw = GenerateBandLimitedSaw(phase, maxHarmonics);
          float square = GenerateBandLimitedSquare(phase, maxHarmonics);

          // ─────────────────────────────────────────────────────────────────
          // DATA-DRIVEN MORPHING
          // Maps morph position t ∈ [0,1] to interpolation between 4 shapes:
          //   t = 0.00 → Sine
          //   t = 0.33 → Triangle
          //   t = 0.66 → Saw
          //   t = 1.00 → Square
          //
          // Using named constants instead of magic numbers for clarity
          // ─────────────────────────────────────────────────────────────────
          float shapePos = t * (kNumWaveShapes - 1);  // 0 to 3
          int shape0 = static_cast<int>(shapePos);
          int shape1 = std::min(shape0 + 1, kNumWaveShapes - 1);
          float blend = shapePos - shape0;

          // Get waveform values for the two shapes we're blending between
          float shapes[kNumWaveShapes] = {sine, triangle, saw, square};
          float sample = shapes[shape0] + blend * (shapes[shape1] - shapes[shape0]);

          table[mip][frame][i] = sample;
        }
      }
    }
    return table;
  }
};

// ═══════════════════════════════════════════════════════════════════════════════
// WAVEFORM TYPES
// ═══════════════════════════════════════════════════════════════════════════════
// Defines available oscillator modes. The first 4 use Q library's direct
// oscillators (computed mathematically). kWaveformWavetable uses our mipmapped
// wavetable with morphing between Sine→Triangle→Saw→Square.
//
// MORPH ORDER RATIONALE (Sine→Triangle→Saw→Square):
// Ordered by increasing harmonic richness and timbral complexity:
//   Sine:     1 harmonic  (fundamental only) - pure, clean
//   Triangle: Odd harmonics, 1/n² rolloff   - soft, flute-like
//   Saw:      All harmonics, 1/n rolloff    - rich, full (128 harmonics)
//   Square:   Odd harmonics, 1/n rolloff    - hollow, nasal (64 harmonics)
// Note: Saw has MORE harmonics than Square (all vs odd-only), but Square
// sounds harsher/buzzier due to its hollow spectrum. This order goes from
// pure → soft → rich → hollow, a natural timbral progression for sweeps.
// ═══════════════════════════════════════════════════════════════════════════════
enum EWaveform
{
  kWaveformSine = 0,    // Pure sine - 1 harmonic
  kWaveformSaw,         // Sawtooth - all harmonics
  kWaveformSquare,      // Square - odd harmonics (50% duty cycle)
  kWaveformTriangle,    // Triangle - odd harmonics, 1/n² rolloff
  kWaveformPulse,       // Pulse - variable duty cycle (PWM), PolyBLEP anti-aliased
  kWaveformFM,          // FM synthesis - modulator modulates carrier phase
  kWaveformWavetable,   // Morphing wavetable (Sine→Triangle→Saw→Square)
  kNumWaveforms
};

// ═══════════════════════════════════════════════════════════════════════════════
// PLUGIN DSP ENGINE
// ═══════════════════════════════════════════════════════════════════════════════
//
// ARCHITECTURE OVERVIEW:
// PluginInstanceDSP is the main DSP engine that manages synthesis and audio output.
// It uses iPlug2's MidiSynth infrastructure for voice allocation and MIDI handling.
//
// SIGNAL FLOW:
//   MIDI → MidiSynth → Voice[] → Mix → Master Gain → Output
//
//   1. MIDI events are queued via ProcessMidiMsg()
//   2. MidiSynth allocates/triggers voices (polyphonic, 8 voices default)
//   3. Each Voice generates: Oscillator → Filter → Envelope → output
//   4. Voice outputs are summed (accumulated) into the output buffer
//   5. Master gain is applied with smoothing for click-free automation
//
// VOICE ARCHITECTURE:
//   Each Voice contains independent instances of all DSP components,
//   allowing per-note filtering and envelope behavior. Voices accumulate
//   their output (+=) rather than overwriting, enabling polyphony.
//
// ═══════════════════════════════════════════════════════════════════════════════
template<typename T>
class PluginInstanceDSP
{
public:
  // ─────────────────────────────────────────────────────────────────────────────
  // VOICE - Single Polyphonic Voice
  // ─────────────────────────────────────────────────────────────────────────────
  // A Voice represents one "note" in the polyphonic synth. Each voice has:
  //   - Its own oscillator (phase, frequency, waveform selection)
  //   - Its own filter (cutoff/resonance applied per-note)
  //   - Its own ADSR envelope (attack/decay/sustain/release)
  //
  // Voices are managed by iPlug2's MidiSynth/VoiceAllocator. When a MIDI note-on
  // arrives, VoiceAllocator finds a free voice (or steals one) and calls Trigger().
  // When note-off arrives, it calls Release(). Voices accumulate their output
  // into the shared buffer, enabling polyphony.
  //
  // RETRIGGER BEHAVIOR:
  // When a voice is retriggered while still sounding (isRetrigger=true), we
  // preserve the oscillator phase for smooth legato, but reset the envelope
  // with a brief crossfade to avoid clicks. See Trigger() for details.
  // ─────────────────────────────────────────────────────────────────────────────
#pragma mark - Voice
  class Voice : public SynthVoice
  {
  public:
    Voice()
    {
      mEnvConfig = q::adsr_envelope_gen::config{
        q::duration{0.01f},   // 10ms attack
        q::duration{0.1f},    // 100ms decay
        q::lin_to_db(0.7f),   // 70% sustain level (in dB via lin_to_db)
        50_s,                 // Sustain hold time - how long to hold at sustain level
                              // before auto-release. 50s = effectively "forever" (until note-off)
        q::duration{0.2f}     // 200ms release
      };
    }

    // Set shared wavetable data (called once from DSP class)
    void SetWavetable(const WavetableOscillator::WavetableData* table)
    {
      mWavetableOsc.SetWavetable(table);
    }

    bool GetBusy() const override
    {
      return mActive && !mEnv.in_idle_phase();
    }

    void Trigger(double level, bool isRetrigger) override
    {
      // Capture current envelope level BEFORE creating new envelope
      // This is needed for smooth retriggering (legato playing)
      float currentLevel = mActive ? mEnv.current() : 0.0f;

      mActive = true;
      mVelocity = static_cast<float>(level);

      if (!isRetrigger)
      {
        // New note - reset oscillator phase and filter for consistent attack
        mPhase = q::phase_iterator{};
        mWavetableOsc.Reset();
        mFilter.Reset();  // Clear filter state to avoid artifacts from previous notes
        mFMModulatorPhase = 0.0f;  // Reset FM modulator for consistent FM timbre on attack
      }

      // Create fresh envelope generator at current sample rate
      mEnv = q::adsr_envelope_gen{mEnvConfig, static_cast<float>(mSampleRate)};
      mEnv.attack();

      // ─────────────────────────────────────────────────────────────────────────
      // RETRIGGER SMOOTHING
      // When retriggering a voice that's still sounding, we need to smoothly
      // transition from the current level to avoid clicks. We use a fast
      // exponential decay (~5ms) to bridge the gap.
      // ─────────────────────────────────────────────────────────────────────────
      if (currentLevel > 0.01f)
      {
        mRetriggerOffset = currentLevel;
        // Calculate decay coefficient for ~5ms fade (sample-rate independent)
        mRetriggerDecay = 1.0f - calcSmoothingCoeff(0.005f, static_cast<float>(mSampleRate));
      }
      else
      {
        mRetriggerOffset = 0.0f;
        mRetriggerDecay = 1.0f;
      }
    }

    void Release() override
    {
      mEnv.release();
    }

    void ProcessSamplesAccumulating(T** inputs, T** outputs,
                                     int nInputs, int nOutputs,
                                     int startIdx, int nFrames) override
    {
      // ─────────────────────────────────────────────────────────────────────────
      // PITCH CALCULATION (1V/Octave Format)
      // iPlug2's MidiSynth provides pitch in "1V/octave" format where:
      //   pitch = 0 → A4 (440Hz)
      //   pitch = 1 → A5 (880Hz)  - one octave up
      //   pitch = -1 → A3 (220Hz) - one octave down
      //
      // The formula freq = 440 × 2^pitch converts this to Hz.
      // MIDI note 69 (A4) gives pitch=0, each semitone is pitch += 1/12.
      //
      // Pitch bend is added before exponentiation for correct tuning behavior.
      // ─────────────────────────────────────────────────────────────────────────
      double pitch = mInputs[kVoiceControlPitch].endValue;
      double pitchBend = mInputs[kVoiceControlPitchBend].endValue;

      // Convert to frequency: 440Hz × 2^(pitch + bend)
      double freq = 440.0 * std::pow(2.0, pitch + pitchBend);

      // Update Q library phase iterator with new frequency
      mPhase.set(q::frequency{static_cast<float>(freq)}, static_cast<float>(mSampleRate));

      // Update wavetable oscillator frequency (for mip level calculation)
      mWavetableOsc.SetFrequency(static_cast<float>(freq), static_cast<float>(mSampleRate));

      for (int i = startIdx; i < startIdx + nFrames; i++)
      {
        // Get envelope amplitude
        float envAmp = mEnv();

        // ─────────────────────────────────────────────────────────────────────────
        // RETRIGGER CROSSFADE
        // When a voice is retriggered, the new envelope starts from zero while
        // the previous note may have been at full amplitude. This causes a click.
        // Solution: We capture the previous level in mRetriggerOffset (see Trigger())
        // and crossfade from it to the new envelope over ~5ms.
        //
        // Logic: If envelope is below the offset, use the offset. The offset
        // decays exponentially, so once the new envelope catches up, we follow it.
        // ─────────────────────────────────────────────────────────────────────────
        if (mRetriggerOffset > 0.001f)
        {
          // Hold at previous level until new envelope catches up
          if (envAmp < mRetriggerOffset)
            envAmp = mRetriggerOffset;
          // Exponential decay towards zero (~5ms time constant)
          mRetriggerOffset *= mRetriggerDecay;
        }
        else
        {
          // Below threshold - clear to avoid denormals
          mRetriggerOffset = 0.0f;
        }

        // Clamp envelope
        if (envAmp < 0.0f) envAmp = 0.0f;
        if (envAmp > 1.0f) envAmp = 1.0f;

        // Check if voice should deactivate
        if (envAmp < 0.0001f && mEnv.in_idle_phase())
        {
          mActive = false;
          return;
        }

        // ─────────────────────────────────────────────────────────────────────────
        // SIGNAL CHAIN: OSC → FILTER → AMP
        // This is the classic subtractive synthesis topology
        // ─────────────────────────────────────────────────────────────────────────

        // ─────────────────────────────────────────────────────────────────────────
        // STEP 1: Generate oscillator sample
        // Q library oscillators use PolyBLEP/PolyBLAMP anti-aliasing (see header).
        // Wavetable uses our mipmapped implementation with trilinear interpolation.
        // ─────────────────────────────────────────────────────────────────────────
        float oscValue = 0.0f;
        switch (mWaveform)
        {
          case kWaveformSine:
            // Sine: Table lookup (sin_lu) - no aliasing, only 1 harmonic
            oscValue = q::sin(mPhase++);
            break;
          case kWaveformSaw:
            // Sawtooth: PolyBLEP corrects the falling edge discontinuity
            // Naive: 2*phase - 1, then subtract poly_blep correction
            oscValue = q::saw(mPhase++);
            break;
          case kWaveformSquare:
            // Square: PolyBLEP corrects BOTH rising and falling edges
            // Two discontinuities per cycle require two corrections
            oscValue = q::square(mPhase++);
            break;
          case kWaveformTriangle:
            // Triangle: PolyBLAMP corrects slope discontinuities (corners)
            // Uses cubic polynomial (integrated BLEP) for smooth corners
            oscValue = q::triangle(mPhase++);
            break;
          case kWaveformPulse:
            // Pulse: Variable duty cycle with PolyBLEP on both edges
            // At 50% = square wave, <50% = narrow pulse, >50% = wide pulse
            // PWM (pulse width modulation) creates classic chorus/ensemble effects
            //
            // Smooth pulse width to prevent clicks when parameter changes rapidly.
            // Without smoothing, sudden width changes cause the falling edge to
            // jump position mid-cycle, creating a discontinuity (audible click).
            mPulseWidth += mPulseWidthSmoothCoeff * (mPulseWidthTarget - mPulseWidth);
            mPulseOsc.width(mPulseWidth);
            oscValue = mPulseOsc(mPhase++);
            break;
          case kWaveformFM:
          {
            // FM Synthesis: Modulator modulates carrier's phase
            // Formula: output = sin(carrierPhase + depth * sin(modulatorPhase))
            //
            // Combine coarse + fine ratio (DX7-style)
            // Fine is a percentage offset: ratio = coarse * (1 + fine)
            // e.g., coarse 2.0 + fine 0.2 (+20%) = ratio 2.4
            mFMRatioTarget = mFMRatioCoarse * (1.0f + mFMRatioFine);

            // Smooth FM parameters to prevent clicks during modulation
            mFMRatio += mFMSmoothCoeff * (mFMRatioTarget - mFMRatio);
            mFMDepth += mFMSmoothCoeff * (mFMDepthTarget - mFMDepth);

            // Advance modulator phase (runs at carrier_freq × ratio)
            // Convert carrier phase increment to radians, then scale by ratio
            constexpr float kTwoPi = 2.0f * 3.14159265f;
            float carrierPhaseIncRadians = static_cast<float>(mPhase._step.rep) /
                                           static_cast<float>(0xFFFFFFFFu) * kTwoPi;
            mFMModulatorPhase += carrierPhaseIncRadians * mFMRatio;

            // Wrap modulator phase to prevent floating point precision issues
            // Use while loop because at high frequencies + high ratios,
            // phase can advance more than 2π per sample (e.g., 20kHz × ratio 8 = 160kHz)
            while (mFMModulatorPhase >= kTwoPi)
              mFMModulatorPhase -= kTwoPi;

            // Get modulator output (simple sine)
            float modulatorValue = std::sin(mFMModulatorPhase);

            // Apply modulation to carrier phase
            // depth is 0-1, we scale to 0-4π for musically useful range
            constexpr float kMaxModIndex = 4.0f * 3.14159265f;  // ~12.57 radians
            float phaseModulation = mFMDepth * kMaxModIndex * modulatorValue;

            // Get carrier phase as float (0 to 2π)
            float carrierPhase = static_cast<float>(mPhase._phase.rep) /
                                 static_cast<float>(0xFFFFFFFFu) * 2.0f * 3.14159265f;

            // Output = sin(carrierPhase + modulation)
            oscValue = std::sin(carrierPhase + phaseModulation);

            // Advance carrier phase
            mPhase++;
            break;
          }
          case kWaveformWavetable:
            // Wavetable: Perfect band-limiting via mipmapped additive synthesis
            // Supports morphing between Sine→Triangle→Saw→Square
            oscValue = mWavetableOsc.Process();
            break;
          default:
            oscValue = q::sin(mPhase++);
            break;
        }

        // STEP 2: Apply filter (sculpt the harmonics)
        float filtered = mFilter.Process(oscValue);

        // STEP 3: Apply envelope and velocity (shape the amplitude)
        float sample = filtered * envAmp * mVelocity;

        // Accumulate to outputs (don't overwrite - other voices add here too)
        outputs[0][i] += sample;
        if (nOutputs > 1)
          outputs[1][i] += sample;
      }
    }

    void SetSampleRateAndBlockSize(double sampleRate, int blockSize) override
    {
      mSampleRate = sampleRate;
      mEnv = q::adsr_envelope_gen{mEnvConfig, static_cast<float>(sampleRate)};
      mWavetableOsc.SetSampleRate(static_cast<float>(sampleRate));
      mFilter.SetSampleRate(static_cast<float>(sampleRate));

      // Pulse width smoothing: ~5ms for responsive but click-free modulation
      mPulseWidthSmoothCoeff = calcSmoothingCoeff(0.005f, static_cast<float>(sampleRate));

      // FM parameter smoothing: ~5ms for smooth ratio/depth changes
      mFMSmoothCoeff = calcSmoothingCoeff(0.005f, static_cast<float>(sampleRate));
    }

    // Parameter setters called from DSP class
    void SetWaveform(int waveform) { mWaveform = waveform; }
    void SetWavetablePosition(float pos) { mWavetableOsc.SetPosition(pos); }

    void SetAttack(float ms)
    {
      mEnvConfig.attack_rate = q::duration{ms * 0.001f};
    }

    void SetDecay(float ms)
    {
      mEnvConfig.decay_rate = q::duration{ms * 0.001f};
    }

    void SetSustain(float level)
    {
      mEnvConfig.sustain_level = q::lin_to_db(level);
    }

    void SetRelease(float ms)
    {
      mEnvConfig.release_rate = q::duration{ms * 0.001f};
    }

    // Filter setters
    void SetFilterCutoff(float freqHz) { mFilter.SetCutoff(freqHz); }
    void SetFilterResonance(float resonance) { mFilter.SetResonance(resonance); }
    void SetFilterType(int type) { mFilter.SetType(static_cast<FilterType>(type)); }

    // Pulse width setter (0.0-1.0, where 0.5 = square wave)
    // Sets target; actual value is smoothed per-sample to prevent clicks
    void SetPulseWidth(float width) { mPulseWidthTarget = width; }

    // FM synthesis setters (DX7-style coarse + fine)
    // Coarse ratio: harmonic values (0.5, 1, 2, 3, etc.)
    void SetFMRatioCoarse(float ratio) { mFMRatioCoarse = ratio; }
    // Fine ratio: percentage offset (-0.5 to +0.5) for inharmonic sounds
    void SetFMRatioFine(float fine) { mFMRatioFine = fine; }
    // Depth: modulation intensity (0.0-1.0)
    void SetFMDepth(float depth) { mFMDepthTarget = depth; }

  private:
    // ─────────────────────────────────────────────────────────────────────────
    // PHASE ITERATOR - Q library's oscillator driver
    // Fixed-point 1.31 format: 32-bit unsigned int where full cycle = 2^32
    // - _phase: Current position in cycle (0 to 2^32-1)
    // - _step: Phase increment per sample = (2^32 × freq) / sampleRate
    // Natural wraparound via integer overflow eliminates modulo operations.
    // Usage: q::sin(mPhase++) returns sine value and advances phase.
    // ─────────────────────────────────────────────────────────────────────────
    q::phase_iterator mPhase;

    // Pulse oscillator - PolyBLEP with variable duty cycle
    // Width: 0.0-1.0 where 0.5 = square wave, <0.5 = narrow pulse, >0.5 = wide pulse
    q::pulse_osc mPulseOsc{0.5f};

    // ─────────────────────────────────────────────────────────────────────────
    // PULSE WIDTH SMOOTHING
    // When pulse width changes suddenly, the falling edge position jumps,
    // causing a discontinuity (click). We smooth the pulse width over ~5ms
    // to prevent this. Faster than gain smoothing for responsive modulation.
    // ─────────────────────────────────────────────────────────────────────────
    float mPulseWidth = 0.5f;           // Current smoothed pulse width
    float mPulseWidthTarget = 0.5f;     // Target from parameter
    float mPulseWidthSmoothCoeff = 0.01f;  // ~5ms smoothing, set in SetSampleRateAndBlockSize

    // ─────────────────────────────────────────────────────────────────────────
    // FM SYNTHESIS - Modulator oscillator and parameters (DX7-style)
    // The modulator is a hidden sine wave that modulates the carrier's phase.
    // Coarse ratio sets harmonic relationship, Fine allows inharmonic detuning.
    // ─────────────────────────────────────────────────────────────────────────
    float mFMModulatorPhase = 0.0f;       // Modulator phase (radians, wraps at 2π)
    float mFMRatioCoarse = 2.0f;          // Coarse ratio (0.5, 1, 2, 3, etc.)
    float mFMRatioFine = 0.0f;            // Fine offset (-0.5 to +0.5)
    float mFMRatio = 2.0f;                // Current smoothed combined ratio
    float mFMRatioTarget = 2.0f;          // Target combined ratio
    float mFMDepth = 0.5f;                // Current smoothed depth (0-1)
    float mFMDepthTarget = 0.5f;          // Target depth from parameter
    float mFMSmoothCoeff = 0.01f;         // ~5ms smoothing for FM params

    // ADSR envelope generator - controls amplitude over note lifetime
    q::adsr_envelope_gen mEnv{q::adsr_envelope_gen::config{}, 44100.0f};
    q::adsr_envelope_gen::config mEnvConfig;

    WavetableOscillator mWavetableOsc;  // Wavetable oscillator (mipmapped, morphable)
    ResonantFilter mFilter;              // Per-voice resonant filter (Q library biquads)
    double mSampleRate = 44100.0;
    float mVelocity = 0.0f;              // MIDI velocity (0-1)
    bool mActive = false;                // Voice is currently sounding
    int mWaveform = kWaveformSine;       // Current waveform selection

    // ─────────────────────────────────────────────────────────────────────────
    // RETRIGGER SMOOTHING
    // When a voice is retriggered while still sounding, we crossfade from
    // the previous amplitude to avoid clicks. mRetriggerOffset holds the
    // level to fade from, mRetriggerDecay controls the ~5ms fade speed.
    // ─────────────────────────────────────────────────────────────────────────
    float mRetriggerOffset = 0.0f;
    float mRetriggerDecay = 1.0f;
  };

public:
#pragma mark - DSP
  PluginInstanceDSP(int nVoices = 8)
  {
    // Get pointer to static wavetable (no copy!)
    mWavetable = &WavetableGenerator::GenerateBasicShapes();

    for (int i = 0; i < nVoices; i++)
    {
      Voice* voice = new Voice();
      voice->SetWavetable(mWavetable);  // Share wavetable data
      mSynth.AddVoice(voice, 0);
    }
  }

  void ProcessBlock(T** inputs, T** outputs, int nOutputs, int nFrames)
  {
    // Clear outputs first
    for (int c = 0; c < nOutputs; c++)
    {
      memset(outputs[c], 0, nFrames * sizeof(T));
    }

    // MidiSynth processes MIDI queue and calls voice ProcessSamplesAccumulating
    mSynth.ProcessBlock(inputs, outputs, 0, nOutputs, nFrames);

    // ─────────────────────────────────────────────────────────────────────────
    // MASTER GAIN WITH SMOOTHING
    // We use a one-pole lowpass filter to smooth gain changes, preventing
    // clicks when the user moves the gain knob. The smoothing coefficient
    // is calculated in Reset() to be sample-rate independent (~20ms time).
    // ─────────────────────────────────────────────────────────────────────────
    for (int s = 0; s < nFrames; s++)
    {
      mGainSmoothed += mGainSmoothCoeff * (mGain - mGainSmoothed);

      // Polyphony headroom compensation: reduces amplitude to manage clipping
      // Worst-case (8 voices × full velocity × full envelope): 8.0 amplitude
      // With kPolyScale=0.35: 8 × 0.35 = 2.8, exceeds 1.0 so will hard-clip.
      // Typical use (3-4 voices, varied velocities): ~1.0-1.5, minimal clipping.
      // The safety clamp below catches peaks; per-voice filter saturation also helps.
      constexpr float kPolyScale = 0.35f;
      float gainScale = kPolyScale * mGainSmoothed;

      for (int c = 0; c < nOutputs; c++)
      {
        outputs[c][s] *= gainScale;

        // Safety clamp
        if (outputs[c][s] > 1.0f) outputs[c][s] = 1.0f;
        if (outputs[c][s] < -1.0f) outputs[c][s] = -1.0f;
      }
    }
  }

  void Reset(double sampleRate, int blockSize)
  {
    mSynth.SetSampleRateAndBlockSize(sampleRate, blockSize);
    mSynth.Reset();
    mGainSmoothed = mGain;

    // Calculate gain smoothing coefficient for ~20ms time constant
    // This ensures consistent smoothing behavior across all sample rates
    mGainSmoothCoeff = calcSmoothingCoeff(0.02f, static_cast<float>(sampleRate));
  }

  void ProcessMidiMsg(const IMidiMsg& msg)
  {
    mSynth.AddMidiMsgToQueue(msg);
  }

  void SetParam(int paramIdx, double value)
  {
    switch (paramIdx)
    {
      case kParamGain:
        mGain = static_cast<float>(value / 100.0);
        break;

      case kParamWaveform:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          dynamic_cast<Voice&>(voice).SetWaveform(static_cast<int>(value));
        });
        break;

      case kParamWavetablePosition:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          dynamic_cast<Voice&>(voice).SetWavetablePosition(static_cast<float>(value / 100.0));
        });
        break;

      case kParamAttack:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          dynamic_cast<Voice&>(voice).SetAttack(static_cast<float>(value));
        });
        break;

      case kParamDecay:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          dynamic_cast<Voice&>(voice).SetDecay(static_cast<float>(value));
        });
        break;

      case kParamSustain:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          dynamic_cast<Voice&>(voice).SetSustain(static_cast<float>(value / 100.0));
        });
        break;

      case kParamRelease:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          dynamic_cast<Voice&>(voice).SetRelease(static_cast<float>(value));
        });
        break;

      // Filter parameters
      case kParamFilterCutoff:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          dynamic_cast<Voice&>(voice).SetFilterCutoff(static_cast<float>(value));
        });
        break;

      case kParamFilterResonance:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          dynamic_cast<Voice&>(voice).SetFilterResonance(static_cast<float>(value / 100.0));
        });
        break;

      case kParamFilterType:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          dynamic_cast<Voice&>(voice).SetFilterType(static_cast<int>(value));
        });
        break;

      // Pulse width modulation
      case kParamPulseWidth:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          // Convert 5-95% to 0.05-0.95 for Q library pulse_osc
          dynamic_cast<Voice&>(voice).SetPulseWidth(static_cast<float>(value / 100.0));
        });
        break;

      // FM synthesis parameters (DX7-style coarse + fine)
      case kParamFMRatio:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          // Convert enum index to coarse ratio: 0→0.5, 1→1, 2→2, 3→3, etc.
          int idx = static_cast<int>(value);
          float ratio = (idx == 0) ? 0.5f : static_cast<float>(idx);
          dynamic_cast<Voice&>(voice).SetFMRatioCoarse(ratio);
        });
        break;

      case kParamFMFine:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          // Convert -50% to +50% to -0.5 to +0.5
          dynamic_cast<Voice&>(voice).SetFMRatioFine(static_cast<float>(value / 100.0));
        });
        break;

      case kParamFMDepth:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          // Convert 0-100% to 0.0-1.0
          dynamic_cast<Voice&>(voice).SetFMDepth(static_cast<float>(value / 100.0));
        });
        break;

      default:
        break;
    }
  }

private:
  MidiSynth mSynth{VoiceAllocator::kPolyModePoly};
  const WavetableOscillator::WavetableData* mWavetable = nullptr;  // Pointer to static table
  float mGain = 0.8f;
  float mGainSmoothed = 0.8f;
  float mGainSmoothCoeff = 0.001f;  // Sample-rate aware, set in Reset()
};
