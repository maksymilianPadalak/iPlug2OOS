#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// DSP UTILITIES - Standalone audio processing utilities
// ═══════════════════════════════════════════════════════════════════════════════
// This file contains pure utility functions for audio DSP with no external
// dependencies beyond the C++ standard library. Safe to include anywhere.
// ═══════════════════════════════════════════════════════════════════════════════

#include <cmath>
#include <algorithm>
#include <cstdint>

// ═══════════════════════════════════════════════════════════════════════════════
// DENORMAL PROTECTION - Prevent CPU spikes from tiny floating-point values
// ═══════════════════════════════════════════════════════════════════════════════
// Denormal (subnormal) numbers are extremely small floats that require special
// CPU handling, causing 10-100x slowdowns. They commonly occur in:
// - Filter feedback loops decaying to silence
// - Reverb/delay tails fading out
// - Envelope release stages
//
// Solution: Set FTZ (Flush To Zero) and DAZ (Denormals Are Zero) CPU flags.
// These treat denormals as zero, eliminating the performance penalty.
//
// Platform support:
// - x86/x64 with SSE: Use MXCSR register
// - ARM with NEON: Use FPCR register
// - Other: Graceful fallback (no-op)
// ═══════════════════════════════════════════════════════════════════════════════
#if defined(__SSE__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 1)
  #include <xmmintrin.h>  // SSE: _mm_setcsr, _mm_getcsr
  #define HAS_SSE_DENORMAL_CONTROL 1
#elif defined(__ARM_NEON) || defined(__ARM_NEON__)
  #define HAS_ARM_DENORMAL_CONTROL 1
#endif

// RAII helper to enable denormal flushing for a scope
// Usage: DenormalGuard guard; // at start of ProcessBlock
class DenormalGuard
{
public:
  DenormalGuard()
  {
#if defined(HAS_SSE_DENORMAL_CONTROL)
    // Save current MXCSR state
    mPreviousState = _mm_getcsr();
    // Set FTZ (bit 15) and DAZ (bit 6) flags
    // 0x8040 = FTZ | DAZ
    _mm_setcsr(mPreviousState | 0x8040);
#elif defined(HAS_ARM_DENORMAL_CONTROL)
    // ARM: Set FZ bit in FPCR (Floating-Point Control Register)
    uint64_t fpcr;
    __asm__ __volatile__("mrs %0, fpcr" : "=r"(fpcr));
    mPreviousState = static_cast<unsigned int>(fpcr);
    fpcr |= (1 << 24);  // FZ bit
    __asm__ __volatile__("msr fpcr, %0" : : "r"(fpcr));
#endif
  }

  ~DenormalGuard()
  {
#if defined(HAS_SSE_DENORMAL_CONTROL)
    // Restore previous MXCSR state
    _mm_setcsr(mPreviousState);
#elif defined(HAS_ARM_DENORMAL_CONTROL)
    // Restore previous FPCR state
    uint64_t fpcr = mPreviousState;
    __asm__ __volatile__("msr fpcr, %0" : : "r"(fpcr));
#endif
  }

  // Non-copyable
  DenormalGuard(const DenormalGuard&) = delete;
  DenormalGuard& operator=(const DenormalGuard&) = delete;

private:
  unsigned int mPreviousState = 0;
};

// ═══════════════════════════════════════════════════════════════════════════════
// MATHEMATICAL CONSTANTS
// ═══════════════════════════════════════════════════════════════════════════════
// Named constants for commonly used values. Improves readability and avoids
// magic numbers scattered throughout the codebase.
// ═══════════════════════════════════════════════════════════════════════════════
constexpr float kPi = 3.14159265358979323846f;
constexpr float kTwoPi = 2.0f * kPi;              // Full circle in radians
constexpr float kHalfPi = 0.5f * kPi;             // Quarter circle
constexpr float kQuarterPi = 0.25f * kPi;         // Eighth circle (used in constant-power pan)
constexpr float kSqrtHalf = 0.7071067811865476f;  // 1/sqrt(2), equal power stereo

// ═══════════════════════════════════════════════════════════════════════════════
// DSP UTILITIES
// ═══════════════════════════════════════════════════════════════════════════════

// Calculate one-pole smoothing coefficient from time constant
// This creates a sample-rate independent smoothing filter.
// Formula: coeff = 1 - e^(-1 / (time * sampleRate))
// At each sample: smoothed += coeff * (target - smoothed)
inline float calcSmoothingCoeff(float timeSeconds, float sampleRate)
{
  // Protect against invalid inputs (zero, negative, NaN, Inf)
  // Returns 1.0f (instant response) for any invalid input
  if (!(timeSeconds > 0.0f) || !(sampleRate > 0.0f)) return 1.0f;  // Catches NaN too (NaN > x is always false)
  float product = timeSeconds * sampleRate;
  float result = 1.0f - std::exp(-1.0f / product);
  // Final sanity check - coefficient must be in (0, 1]
  if (!(result > 0.0f) || !(result <= 1.0f)) return 1.0f;
  return result;
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
// FAST 2^x - Optimized power-of-two exponential
// ═══════════════════════════════════════════════════════════════════════════════
// std::pow(2.0f, x) is expensive because it handles general cases (any base).
// For 2^x specifically, std::exp2f is much faster:
// - Dedicated CPU instruction on many architectures (x87, SSE, AVX)
// - No need to compute log(base) internally
// - ~3-5x faster than std::pow(2.0f, x)
//
// Used for:
// - Pitch modulation: freq * 2^(semitones/12)
// - Filter cutoff modulation: cutoff * 2^(octaves)
// - Any octave-based calculation
// ═══════════════════════════════════════════════════════════════════════════════
inline float fastExp2(float x)
{
  return std::exp2f(x);
}

// ═══════════════════════════════════════════════════════════════════════════════
// FAST SINE - Polynomial approximation for radians input
// ═══════════════════════════════════════════════════════════════════════════════
// std::sin is expensive (~50-100 cycles). This parabolic approximation is ~5-10x
// faster with accuracy of ~99.9% (max error ~0.001).
//
// Used for FM synthesis where we need sin(radians) many times per sample.
// The Q library's q::sin() uses phase iterators; this handles raw radians.
//
// Algorithm: Attempt's parabolic approximation with precision boost
// Reference: Attempt (devmaster.net forums), widely used in audio DSP
//   1. Wrap input to [-π, π]
//   2. Parabolic: y = 4/π·x - 4/π²·x·|x|
//   3. Precision boost: y = P·(y·|y| - y) + y, where P ≈ 0.225
// ═══════════════════════════════════════════════════════════════════════════════
inline float fastSin(float x)
{
  // Wrap to [-π, π] using fast floor
  constexpr float invTwoPi = 1.0f / kTwoPi;
  x = x - kTwoPi * std::floor(x * invTwoPi + 0.5f);

  // Parabolic approximation
  constexpr float B = 4.0f / kPi;
  constexpr float C = -4.0f / (kPi * kPi);
  float y = B * x + C * x * std::abs(x);

  // Precision boost (corrects the parabola to match sine more closely)
  constexpr float P = 0.225f;
  return P * (y * std::abs(y) - y) + y;
}

// Fast cosine using fastSin with phase shift
inline float fastCos(float x)
{
  return fastSin(x + kPi * 0.5f);
}

// ═══════════════════════════════════════════════════════════════════════════════
// FAST tanh - Padé approximant for soft clipping
// ═══════════════════════════════════════════════════════════════════════════════
// std::tanh is expensive (~50-100 cycles). This rational approximation is ~10x
// faster with accuracy of ~3% for |x| < 3 (sufficient for soft clipping).
//
// Formula: tanh(x) ≈ x(27 + x²) / (27 + 9x²)
// This is a [1,2] Padé approximant that's exact at x=0 and asymptotes correctly.
//
// Accuracy:
//   - |x| < 1: error < 0.5%
//   - |x| < 2: error < 2%
//   - |x| < 3: error < 3%
//   - |x| > 3: clamps to ±1 (good enough for saturation)
// ═══════════════════════════════════════════════════════════════════════════════
inline float fastTanh(float x)
{
  // Clamp extreme values to prevent numerical issues
  if (x > 3.0f) return 1.0f;
  if (x < -3.0f) return -1.0f;

  float x2 = x * x;
  return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

// ═══════════════════════════════════════════════════════════════════════════════
// FAST e^x - Schraudolph's approximation using IEEE 754 bit manipulation
// ═══════════════════════════════════════════════════════════════════════════════
// std::exp is expensive (~50-100 cycles). This approximation is ~10x faster
// with accuracy of ~2-3% (sufficient for limiter compression curves).
//
// Algorithm: Exploits IEEE 754 float format where exponent bits directly
// represent powers of 2. Since e^x = 2^(x/ln(2)), we can compute this by
// manipulating the exponent bits directly.
//
// Valid range: approximately -87 to +88 (float exp limits)
// ═══════════════════════════════════════════════════════════════════════════════
inline float fastExp(float x)
{
  // Clamp to valid range to avoid overflow/underflow
  x = std::max(-87.0f, std::min(88.0f, x));

  // Schraudolph's method: interpret scaled value as float bits
  // Magic numbers: 12102203 ≈ 2^23/ln(2), 1065353216 = 127 << 23 (bias)
  // The 486411 adjustment improves average accuracy
  union { float f; int32_t i; } u;
  u.i = static_cast<int32_t>(12102203.0f * x + 1065353216.0f - 486411.0f);
  return u.f;
}

// ═══════════════════════════════════════════════════════════════════════════════
// NaN/INFINITY PROTECTION - Critical for stable audio processing
// ═══════════════════════════════════════════════════════════════════════════════
// NaN (Not a Number) and Infinity can occur from:
// - Filter instability at high resonance
// - Division by zero
// - sqrt of negative numbers
// - Unbounded feedback loops
//
// Once NaN enters the signal chain, it corrupts ALL subsequent calculations.
// Example: NaN + anything = NaN, NaN * anything = NaN
// This causes the "oscillator stops working" bug.
//
// DETECTION: std::isnan() and std::isinf() are portable but can be slow.
// We use a fast check based on IEEE 754: NaN and Inf have all exponent bits set.
// ═══════════════════════════════════════════════════════════════════════════════

// Fast check for NaN or Infinity using bit pattern
inline bool isAudioCorrupt(float x)
{
  // IEEE 754: exponent bits are 23-30. If all 8 are 1, it's NaN or Inf.
  // This is faster than std::isnan() + std::isinf() on most platforms.
  union { float f; uint32_t i; } u;
  u.f = x;
  return ((u.i & 0x7F800000) == 0x7F800000);
}

// Sanitize audio sample: replace NaN/Inf with 0, clamp extreme values
inline float sanitizeAudio(float x)
{
  if (isAudioCorrupt(x)) return 0.0f;
  // Also clamp to reasonable range to prevent buildup
  return std::max(-10.0f, std::min(10.0f, x));
}

// Wrap a phase angle to [0, 2π) - fast floor-based implementation
// More robust than std::fmod which can have precision issues near 2π
inline float wrapPhase(float phase)
{
  // Protect against NaN/Inf - clamp to middle of range (π) instead of 0
  // to minimize the discontinuity (max jump is π instead of 2π)
  if (isAudioCorrupt(phase)) return kPi;

  // Fast wrap using floor - handles both positive and negative values correctly
  // floor(-0.1 / 2π) = -1, so phase becomes -0.1 - 2π*(-1) = 2π - 0.1 ✓
  constexpr float invTwoPi = 1.0f / kTwoPi;
  phase -= kTwoPi * std::floor(phase * invTwoPi);
  return phase;
}
