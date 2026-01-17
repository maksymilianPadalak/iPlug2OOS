#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// RESONANT FILTER - Cytomic Trapezoidal SVF (State Variable Filter)
// ═══════════════════════════════════════════════════════════════════════════════
// Standalone resonant filter implementation with no external dependencies
// beyond the C++ standard library and dsp_utilities.h.
// ═══════════════════════════════════════════════════════════════════════════════

#include <cmath>
#include <algorithm>
#include "dsp_utilities.h"

// ═══════════════════════════════════════════════════════════════════════════════
// RESONANT FILTER - Cytomic Trapezoidal SVF (State Variable Filter)
// ═══════════════════════════════════════════════════════════════════════════════
//
// Production-quality filter implementation using trapezoidal integration.
// This is the industry standard for synthesizer filters (Serum, Vital, Diva).
//
// KEY INSIGHT - Why SVF Instead of Biquad:
// ─────────────────────────────────────────────────────────────────────────────
// Biquad filters store "baked" intermediate values that explode when
// coefficients change rapidly (LFO modulation, fast sweeps). The SVF stores
// actual signal values (ic1eq, ic2eq) that are inherently bounded - making it
// stable under audio-rate modulation at any Q value.
//
// ALGORITHM:
// ─────────────────────────────────────────────────────────────────────────────
// Coefficients:
//   g  = tan(π × cutoff / sampleRate)  // Pre-warped frequency
//   k  = 1/Q                            // Damping (lower = more resonance)
//   a1 = 1 / (1 + g × (g + k))
//   a2 = g × a1
//   a3 = g × a2
//
// Process (per sample):
//   v3 = v0 - ic2eq
//   v1 = a1 × ic1eq + a2 × v3          // Bandpass
//   v2 = ic2eq + a2 × ic1eq + a3 × v3  // Lowpass
//   ic1eq = 2 × v1 - ic1eq             // State update (trapezoidal)
//   ic2eq = 2 × v2 - ic2eq
//   output = m0×v0 + m1×v1 + m2×v2     // Mix for filter type
//
// Mix Coefficients:
//   Lowpass:  m0=0, m1=0,  m2=1
//   Highpass: m0=1, m1=-k, m2=-1
//   Bandpass: m0=0, m1=1,  m2=0
//   Notch:    m0=1, m1=-k, m2=0
//
// ═══════════════════════════════════════════════════════════════════════════════

enum class FilterType { Lowpass = 0, Highpass, Bandpass, Notch, kNumFilterTypes };

class ResonantFilter
{
public:
  // Initialize for sample rate (call before processing)
  void SetSampleRate(float sampleRate)
  {
    mSampleRate = sampleRate;
    mMaxCutoffHz = sampleRate * 0.45f;  // Stay below Nyquist (tan→∞)
    mCutoffHz = std::min(mCutoffHz, mMaxCutoffHz);
    UpdateCoefficients();
  }

  // Set cutoff frequency in Hz (20 - ~21600 at 48kHz)
  // Safe to call every sample - SVF is stable under audio-rate modulation
  void SetCutoff(float freqHz)
  {
    mCutoffHz = std::max(20.0f, std::min(mMaxCutoffHz, freqHz));
    UpdateCoefficients();
  }

  // Set resonance 0.0-1.0, mapped exponentially to Q 0.5-25
  void SetResonance(float resonance)
  {
    mResonance = std::max(0.0f, std::min(1.0f, resonance));
    UpdateCoefficients();
  }

  // Set filter type (click-free, no state reset needed)
  void SetType(FilterType type)
  {
    if (mType != type) { mType = type; UpdateMixCoefficients(); }
  }

  // Reset state on note trigger (safe - stores signal values, not coefficients)
  void Reset() { mIC1eq = 0.0f; mIC2eq = 0.0f; }

  // Process single sample - the core SVF algorithm
  float Process(float v0)
  {
    // Bypass optimization for wide-open filter
    if (mType == FilterType::Lowpass && mCutoffHz >= mMaxCutoffHz * 0.98f) return v0;
    if (mType == FilterType::Highpass && mCutoffHz <= 25.0f) return v0;

    // SVF core - two trapezoidal integrators with feedback
    float v3 = v0 - mIC2eq;
    float v1 = mA1 * mIC1eq + mA2 * v3;
    float v2 = mIC2eq + mA2 * mIC1eq + mA3 * v3;

    // Trapezoidal state update (bounded to signal range - this is why it's stable)
    mIC1eq = 2.0f * v1 - mIC1eq;
    mIC2eq = 2.0f * v2 - mIC2eq;

    // Flush denormals to prevent CPU spikes when audio fades to silence
    mIC1eq = flushDenormal(mIC1eq);
    mIC2eq = flushDenormal(mIC2eq);

    // Mix outputs for filter type
    float output = mM0 * v0 + mM1 * v1 + mM2 * v2;

    // Soft saturation for analog character at high resonance
    if (std::abs(output) > 2.0f)
    {
      float sign = output > 0.0f ? 1.0f : -1.0f;
      output = sign * (2.0f + fastTanh(std::abs(output) - 2.0f));
    }

    return output;
  }

private:
  // Exponential resonance mapping: Q = 0.5 × 50^resonance (range 0.5-25)
  float ResonanceToQ(float resonance) const { return 0.5f * std::pow(50.0f, resonance); }

  void UpdateCoefficients()
  {
    if (mSampleRate <= 0.0f) return;
    float normalizedFreq = std::min(mCutoffHz / mSampleRate, 0.49f);
    mG = std::tan(static_cast<float>(M_PI) * normalizedFreq);
    mK = 1.0f / ResonanceToQ(mResonance);
    mA1 = 1.0f / (1.0f + mG * (mG + mK));
    mA2 = mG * mA1;
    mA3 = mG * mA2;
    UpdateMixCoefficients();
  }

  void UpdateMixCoefficients()
  {
    switch (mType)
    {
      case FilterType::Lowpass:  mM0 = 0.0f; mM1 = 0.0f; mM2 = 1.0f;  break;
      case FilterType::Highpass: mM0 = 1.0f; mM1 = -mK;  mM2 = -1.0f; break;
      case FilterType::Bandpass: mM0 = 0.0f; mM1 = 1.0f; mM2 = 0.0f;  break;
      case FilterType::Notch:    mM0 = 1.0f; mM1 = -mK;  mM2 = 0.0f;  break;
      default:                   mM0 = 0.0f; mM1 = 0.0f; mM2 = 1.0f;  break;
    }
  }

  // Sample rate
  float mSampleRate = 48000.0f;
  float mMaxCutoffHz = 20000.0f;

  // User parameters
  float mCutoffHz = 10000.0f;
  float mResonance = 0.0f;
  FilterType mType = FilterType::Lowpass;

  // SVF coefficients
  float mG = 0.0f, mK = 2.0f, mA1 = 0.0f, mA2 = 0.0f, mA3 = 0.0f;

  // Mix coefficients (determine filter type)
  float mM0 = 0.0f, mM1 = 0.0f, mM2 = 1.0f;

  // Internal state (actual signal values - inherently bounded, unlike biquad)
  float mIC1eq = 0.0f, mIC2eq = 0.0f;
};
