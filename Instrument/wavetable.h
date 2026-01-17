#pragma once

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

#include <cmath>
#include <array>
#include <algorithm>

#include "dsp_utilities.h"

// Wavetable dimensions
constexpr int kWavetableSize = 2048;      // Samples per cycle (power of 2 for fast wrapping)
constexpr int kWavetableFrames = 16;      // Morph positions (more = smoother morphing)
constexpr int kNumMipLevels = 8;          // 8 mip levels: 128→64→32→16→8→4→2→1 harmonics
constexpr float kWavetableSizeF = static_cast<float>(kWavetableSize);

// Morph configuration - 4 classic waveforms
constexpr int kNumWaveShapes = 4;         // Sine, Triangle, Saw, Square

// Memory: 2048 × 16 × 8 × 4 bytes = 1MB (static allocation, not stack)

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

  // ─────────────────────────────────────────────────────────────────────────────
  // PHASE ACCESS FOR HARD SYNC
  // Hard sync requires reading Osc1's phase to detect zero-crossings, and
  // resetting Osc2's phase when sync triggers. These methods expose the
  // internal phase for that purpose.
  // ─────────────────────────────────────────────────────────────────────────────
  float GetPhase() const { return mPhase; }
  void ResetPhase() { mPhase = 0.0f; }

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

  // ─────────────────────────────────────────────────────────────────────────────
  // PROCESS WITH PITCH MODULATION (for voice 0 with LFO pitch mod)
  // ─────────────────────────────────────────────────────────────────────────────
  // Same as Process() but applies a pitch modulation ratio to both the phase
  // increment and the frequency used for mip level calculation. This allows
  // voice 0 to receive LFO pitch modulation while still handling morph smoothing.
  // ─────────────────────────────────────────────────────────────────────────────
  float ProcessWithPitchMod(float pitchModRatio)
  {
    if (!mTable) return 0.0f;

    // Smooth the morph position
    mPosition += mSmoothCoeff * (mTargetPosition - mPosition);

    // Calculate mip level using modulated frequency
    float modulatedFreq = mFrequency * pitchModRatio;
    constexpr float kBaseHarmonics = 128.0f;
    float mipFloat = std::log2(std::max(1.0f, kBaseHarmonics * modulatedFreq / mNyquist));
    mipFloat = std::max(0.0f, std::min(mipFloat, static_cast<float>(kNumMipLevels - 1)));

    int mip0 = static_cast<int>(mipFloat);
    int mip1 = std::min(mip0 + 1, kNumMipLevels - 1);
    float mipFrac = mipFloat - mip0;

    // Frame interpolation (morph position)
    int frame0 = static_cast<int>(mPosition);
    int frame1 = std::min(frame0 + 1, kWavetableFrames - 1);
    float frameFrac = mPosition - frame0;

    // Sample position
    float samplePos = mPhase * kWavetableSizeF;
    int idx0 = static_cast<int>(samplePos) & (kWavetableSize - 1);
    int idx1 = (idx0 + 1) & (kWavetableSize - 1);
    float sampleFrac = samplePos - std::floor(samplePos);

    // Trilinear interpolation
    auto sampleFrame = [&](int mip, int frame) {
      const auto& t = (*mTable)[mip][frame];
      return t[idx0] + sampleFrac * (t[idx1] - t[idx0]);
    };

    float s_mip0_f0 = sampleFrame(mip0, frame0);
    float s_mip0_f1 = sampleFrame(mip0, frame1);
    float s_mip0 = s_mip0_f0 + frameFrac * (s_mip0_f1 - s_mip0_f0);

    float s_mip1_f0 = sampleFrame(mip1, frame0);
    float s_mip1_f1 = sampleFrame(mip1, frame1);
    float s_mip1 = s_mip1_f0 + frameFrac * (s_mip1_f1 - s_mip1_f0);

    float sample = s_mip0 + mipFrac * (s_mip1 - s_mip0);

    // Advance phase with pitch modulation
    mPhase += mPhaseInc * pitchModRatio;
    while (mPhase >= 1.0f) mPhase -= 1.0f;

    return sample;
  }

  // ─────────────────────────────────────────────────────────────────────────────
  // PROCESS AT EXTERNAL PHASE (for unison voices)
  // ─────────────────────────────────────────────────────────────────────────────
  // This method allows unison voices to share the wavetable and morph position
  // while each having their own phase for detuning. The external phase is passed
  // by reference and advanced after sampling.
  //
  // IMPORTANT: Call Process() or ProcessWithPitchMod() first for voice 0 to
  // update the morph position smoothing, then call ProcessAtPhase() for
  // additional unison voices.
  // ─────────────────────────────────────────────────────────────────────────────
  float ProcessAtPhase(float& phase, float phaseInc, float frequency)
  {
    if (!mTable) return 0.0f;

    // Use the already-smoothed morph position from the main oscillator
    // (mPosition was updated by Process() call for voice 0)

    // Calculate mip level for this voice's frequency
    constexpr float kBaseHarmonics = 128.0f;
    float mipFloat = std::log2(std::max(1.0f, kBaseHarmonics * frequency / mNyquist));
    mipFloat = std::max(0.0f, std::min(mipFloat, static_cast<float>(kNumMipLevels - 1)));

    int mip0 = static_cast<int>(mipFloat);
    int mip1 = std::min(mip0 + 1, kNumMipLevels - 1);
    float mipFrac = mipFloat - mip0;

    // Frame interpolation (morph position)
    int frame0 = static_cast<int>(mPosition);
    int frame1 = std::min(frame0 + 1, kWavetableFrames - 1);
    float frameFrac = mPosition - frame0;

    // Sample position
    float samplePos = phase * kWavetableSizeF;
    int idx0 = static_cast<int>(samplePos) & (kWavetableSize - 1);
    int idx1 = (idx0 + 1) & (kWavetableSize - 1);
    float sampleFrac = samplePos - std::floor(samplePos);

    // Trilinear interpolation
    auto sampleFrame = [&](int mip, int frame) {
      const auto& t = (*mTable)[mip][frame];
      return t[idx0] + sampleFrac * (t[idx1] - t[idx0]);
    };

    float s_mip0_f0 = sampleFrame(mip0, frame0);
    float s_mip0_f1 = sampleFrame(mip0, frame1);
    float s_mip0 = s_mip0_f0 + frameFrac * (s_mip0_f1 - s_mip0_f0);

    float s_mip1_f0 = sampleFrame(mip1, frame0);
    float s_mip1_f1 = sampleFrame(mip1, frame1);
    float s_mip1 = s_mip1_f0 + frameFrac * (s_mip1_f1 - s_mip1_f0);

    float sample = s_mip0 + mipFrac * (s_mip1 - s_mip0);

    // Advance external phase with robust wrapping
    phase += phaseInc;
    // Use fmod instead of while loop - handles corrupted large values safely
    if (phase >= 1.0f || phase < 0.0f)
    {
      phase = std::fmod(phase, 1.0f);
      if (phase < 0.0f) phase += 1.0f;
    }
    // Additional safety: clamp if still invalid (NaN protection)
    if (!(phase >= 0.0f && phase < 1.0f)) phase = 0.0f;

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
  // Using global kPi and kTwoPi constants defined at file level

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
      sample -= std::sin(phase * kTwoPi * h) / static_cast<float>(h);
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
      sample += std::sin(phase * kTwoPi * h) / static_cast<float>(h);
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
      sample += sign * std::sin(phase * kTwoPi * h) / static_cast<float>(h * h);
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
          float sine = std::sin(phase * kTwoPi);  // Sine has no harmonics to limit
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
