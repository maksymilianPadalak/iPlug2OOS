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
#include <q/synth/envelope_gen.hpp>

using namespace iplug;
namespace q = cycfi::q;
using namespace q::literals;

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
  kWaveformSquare,      // Square - odd harmonics
  kWaveformTriangle,    // Triangle - odd harmonics, 1/n² rolloff
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
      // Get pitch from voice allocator (1V/octave format)
      double pitch = mInputs[kVoiceControlPitch].endValue;
      double pitchBend = mInputs[kVoiceControlPitchBend].endValue;

      // Convert to frequency: 440 * 2^pitch
      double freq = 440.0 * std::pow(2.0, pitch + pitchBend);
      mPhase.set(q::frequency{static_cast<float>(freq)}, static_cast<float>(mSampleRate));

      // Update wavetable oscillator frequency
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

        // STEP 1: Generate oscillator sample
        float oscValue = 0.0f;
        switch (mWaveform)
        {
          case kWaveformSine:
            oscValue = q::sin(mPhase++);
            break;
          case kWaveformSaw:
            oscValue = q::saw(mPhase++);
            break;
          case kWaveformSquare:
            oscValue = q::square(mPhase++);
            break;
          case kWaveformTriangle:
            oscValue = q::triangle(mPhase++);
            break;
          case kWaveformWavetable:
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

  private:
    q::phase_iterator mPhase;
    q::adsr_envelope_gen mEnv{q::adsr_envelope_gen::config{}, 44100.0f};
    q::adsr_envelope_gen::config mEnvConfig;
    WavetableOscillator mWavetableOsc;  // Wavetable oscillator
    ResonantFilter mFilter;              // Per-voice resonant filter (Q library biquads)
    double mSampleRate = 44100.0;
    float mVelocity = 0.0f;
    bool mActive = false;
    int mWaveform = kWaveformSine;

    // Retrigger smoothing
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
