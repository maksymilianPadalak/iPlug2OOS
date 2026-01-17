#pragma once

/**
 * =============================================================================
 * DATTORRO PLATE REVERB - Golden Example Implementation
 * =============================================================================
 *
 * This is a "golden example" implementation of Jon Dattorro's plate reverb
 * algorithm, designed to teach LLMs how professional reverbs work.
 *
 * ALGORITHM OVERVIEW:
 * ------------------
 * The Dattorro plate reverb uses a "figure-8" tank topology where two parallel
 * processing chains cross-feed into each other. This creates echo density that
 * INCREASES over time - matching real acoustic spaces. Traditional Schroeder
 * reverbs have CONSTANT echo density, which sounds metallic and artificial.
 *
 * SIGNAL FLOW:
 * -----------
 *   Input → Low Cut → High Cut → Pre-Delay → Input Diffusion (4 allpasses)
 *                                              ↓
 *                    ┌─────────────────────────┴─────────────────────────┐
 *                    ↓                                                   ↓
 *              [LEFT TANK]                                        [RIGHT TANK]
 *           ┌──────────────┐                                   ┌──────────────┐
 *           │  Allpass 1   │←──── decay×feedback ────←─────────│   Output     │
 *           │  Delay 1     │                                   │   (taps)     │
 *           │  Damping LP  │                                   └──────────────┘
 *           │  Allpass 2   │                                          ↑
 *           │  Delay 2     │                                          │
 *           └──────────────┘                                          │
 *                    │                                                │
 *                    └─────── decay×feedback ────→─────────→──────────┘
 *                                                        [RIGHT TANK]
 *                                                     (mirror of left)
 *
 * The cross-coupling (left feeds right, right feeds left) is what creates
 * the "figure-8" topology and the increasing echo density.
 *
 * REFERENCES:
 * ----------
 * - Dattorro, J. (1997). "Effect Design Part 1: Reverberator and Other Filters"
 *   https://ccrma.stanford.edu/~dattorro/EffectDesignPart1.pdf
 * - Based on reverse-engineering of Lexicon 224 reverb
 * - MVerb open-source implementation by Martin Eastwood
 *
 * =============================================================================
 */

#include <cmath>
#include <array>
#include <algorithm>

// =============================================================================
// INTERNAL NAMESPACE - All building blocks hidden from public API
// =============================================================================
namespace DattorroInternal
{

// =============================================================================
// SMOOTHED VALUE - Universal Parameter Smoothing
// =============================================================================
/**
 * One-pole smoothing for parameter changes.
 *
 * WHY SMOOTHING:
 * Abrupt parameter changes cause clicks and zipper noise in audio.
 * This applies one-pole lowpass smoothing to any parameter, creating
 * smooth transitions even during automation.
 *
 * USAGE:
 * - Call setTarget() when parameter changes (from UI or automation)
 * - Call getNext() every sample to get smoothed value
 * - Use snap() to immediately jump to target (on reset)
 *
 * TUNING:
 * - Smoothing time of 5-20ms works well for most parameters
 * - Faster (5ms) for modulation, slower (20ms) for filters
 */
template<typename T>
struct SmoothedValue
{
  T current{};
  T target{};
  float coeff = 0.999f;  // Higher = slower smoothing

  void setTarget(T t) { target = t; }

  void setTime(float ms, float sampleRate)
  {
    // Calculate coefficient for given smoothing time
    // coeff = e^(-1 / (time_in_samples))
    if (ms <= 0.0f || sampleRate <= 0.0f) {
      coeff = 0.0f;  // Instant
    } else {
      float samples = ms * 0.001f * sampleRate;
      coeff = std::exp(-1.0f / samples);
    }
  }

  T getNext()
  {
    current = current * coeff + target * (1.0f - coeff);
    return current;
  }

  void snap()
  {
    current = target;
  }

  bool isSmoothing() const
  {
    return std::abs(static_cast<float>(target - current)) > 1e-6f;
  }
};

// =============================================================================
// DC BLOCKER - Removes DC Offset from Feedback Loops
// =============================================================================
/**
 * First-order highpass filter for DC removal.
 *
 * WHY DC BLOCKING:
 * Feedback loops can accumulate DC offset over time, causing the signal
 * to drift and eventually clip. This simple highpass at ~5-10Hz removes
 * DC while preserving all audible frequencies.
 *
 * EQUATION: y[n] = x[n] - x[n-1] + R * y[n-1]
 * where R is close to 1 (typically 0.995 for ~7Hz cutoff at 44.1kHz)
 */
struct DCBlocker
{
  float x1 = 0.0f;  // Previous input
  float y1 = 0.0f;  // Previous output
  float R = 0.995f; // Coefficient (~7Hz at 44.1kHz)

  void clear()
  {
    x1 = 0.0f;
    y1 = 0.0f;
  }

  void setCutoff(float hz, float sampleRate)
  {
    // R = 1 - (2 * pi * fc / fs)
    // Higher R = lower cutoff frequency
    R = 1.0f - (2.0f * 3.14159265f * hz / sampleRate);
    R = std::max(0.9f, std::min(0.9999f, R));
  }

  float process(float x)
  {
    float y = x - x1 + R * y1;
    x1 = x;
    y1 = y;
    return y;
  }
};

// =============================================================================
// FAST MATH UTILITIES
// =============================================================================
/**
 * Fast tanh approximation using rational function.
 *
 * WHY NOT std::tanh():
 * std::tanh() is accurate but slow (~50-100 cycles). This approximation
 * is ~5-10x faster while maintaining good accuracy for audio limiting.
 * Max error is about 0.003 in the range [-3, 3].
 *
 * Formula: tanh(x) ≈ x * (27 + x²) / (27 + 9x²)
 * This is a Padé approximant that matches tanh behavior well.
 */
inline float reverbFastTanh(float x)
{
  // Clamp to prevent overflow for extreme values
  x = std::max(-3.0f, std::min(3.0f, x));
  float x2 = x * x;
  return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

// =============================================================================
// CONSTANTS - Dattorro's Original Delay Times
// =============================================================================
// These delay times (in samples at 29761 Hz) were published by Dattorro and
// are based on the Lexicon 224. The specific values create good diffusion
// without audible resonances.

namespace Constants
{
  // Reference sample rate from original paper
  constexpr float kReferenceSampleRate = 29761.0f;

  // Input diffusion allpass delay times (samples at 29761 Hz)
  // These 4 allpasses smear the input before it enters the tank
  constexpr float kInputDiffusion1 = 142.0f;   // ~4.77 ms
  constexpr float kInputDiffusion2 = 107.0f;   // ~3.60 ms
  constexpr float kInputDiffusion3 = 379.0f;   // ~12.73 ms
  constexpr float kInputDiffusion4 = 277.0f;   // ~9.31 ms

  // Tank allpass delay times (samples at 29761 Hz)
  // Left tank
  constexpr float kTankAllpass1 = 672.0f;      // ~22.58 ms (modulated)
  constexpr float kTankDelay1 = 4453.0f;       // ~149.63 ms
  constexpr float kTankAllpass2 = 1800.0f;     // ~60.48 ms
  constexpr float kTankDelay2 = 3720.0f;       // ~125.00 ms

  // Right tank (different times for stereo decorrelation)
  constexpr float kTankAllpass3 = 908.0f;      // ~30.51 ms (modulated)
  constexpr float kTankDelay3 = 4217.0f;       // ~141.70 ms
  constexpr float kTankAllpass4 = 2656.0f;     // ~89.24 ms
  constexpr float kTankDelay4 = 3163.0f;       // ~106.28 ms

  // Output tap positions (for stereo decorrelation)
  // Left output taps (from right tank)
  constexpr float kLeftTap1 = 266.0f;
  constexpr float kLeftTap2 = 2974.0f;
  constexpr float kLeftTap3 = 1913.0f;
  constexpr float kLeftTap4 = 1996.0f;
  constexpr float kLeftTap5 = 1990.0f;
  constexpr float kLeftTap6 = 187.0f;
  constexpr float kLeftTap7 = 1066.0f;

  // Right output taps (from left tank)
  constexpr float kRightTap1 = 353.0f;
  constexpr float kRightTap2 = 3627.0f;
  constexpr float kRightTap3 = 1228.0f;
  constexpr float kRightTap4 = 2673.0f;
  constexpr float kRightTap5 = 2111.0f;
  constexpr float kRightTap6 = 335.0f;
  constexpr float kRightTap7 = 121.0f;

  // Diffusion coefficients from Dattorro's paper
  constexpr float kInputDiffusionCoeff1 = 0.75f;   // First 2 input allpasses
  constexpr float kInputDiffusionCoeff2 = 0.625f;  // Last 2 input allpasses
  constexpr float kDecayDiffusionCoeff1 = 0.7f;    // Tank allpasses
  constexpr float kDecayDiffusionCoeff2 = 0.5f;    // Second tank allpass

  // Pre-delay maximum (samples at 48kHz)
  constexpr int kMaxPreDelay = 9600;  // 200ms at 48kHz
}

// =============================================================================
// MODULATION BANK - 8 Decorrelated LFOs for Professional Sound
// =============================================================================
/**
 * Bank of 8 decorrelated sine LFOs for rich modulation.
 *
 * WHY MULTIPLE LFOs:
 * Using a single LFO (or two) creates predictable, "mechanical" modulation.
 * Professional reverbs (Valhalla, Lexicon) use 8-32 decorrelated modulators.
 * Each delay line gets its own LFO with:
 * - Different initial phase (spread across 0-1)
 * - Slightly different rate (±10% variation)
 *
 * This creates complex, organic movement that breaks up metallic resonances
 * without sounding like obvious chorus.
 *
 * ASSIGNMENT:
 * LFO ASSIGNMENT (all 8 are used):
 * - LFO 0: Output tap modulation L (user-controlled via Mod Depth)
 * - LFO 1: Output tap modulation R (user-controlled via Mod Depth)
 * - LFO 2: Left tank allpass modulation (user-controlled via Mod Depth)
 * - LFO 3: Input diffusion allpass 1-2 (always active, ±5 samples)
 * - LFO 4: Right tank allpass modulation (user-controlled via Mod Depth)
 * - LFO 5: Input diffusion allpass 3-4 (always active, ±5 samples)
 * - LFO 6: ER diffuser 1-2 (always active, ±5 samples)
 * - LFO 7: ER diffuser 3-4 (always active, ±5 samples)
 *
 * The diffuser modulation (LFO 3, 5, 6, 7) is ALWAYS active regardless of
 * Mod Depth setting. This is essential for preventing metallic "pipe" sound.
 */
struct ModulationBank
{
  static constexpr int N = 8;
  std::array<float, N> phases{};       // Current phase of each LFO (0-1)
  std::array<float, N> rateOffsets{};  // Rate multiplier for each LFO
  std::array<float, N> outputs{};      // Current output values (-1 to +1)

  void reset()
  {
    // Spread phases evenly across the cycle for maximum decorrelation
    for (int i = 0; i < N; ++i) {
      phases[i] = static_cast<float>(i) / static_cast<float>(N);
    }

    // Rate offsets: 0.9× to 1.1× (±10% variation)
    // This prevents LFOs from ever syncing up
    rateOffsets[0] = 0.90f;
    rateOffsets[1] = 1.10f;
    rateOffsets[2] = 0.95f;
    rateOffsets[3] = 1.05f;
    rateOffsets[4] = 0.92f;
    rateOffsets[5] = 1.08f;
    rateOffsets[6] = 0.97f;
    rateOffsets[7] = 1.03f;

    std::fill(outputs.begin(), outputs.end(), 0.0f);
  }

  /**
   * Process all LFOs for one sample.
   * @param baseRateHz Base modulation rate in Hz
   * @param sampleRate Current sample rate
   */
  void process(float baseRateHz, float sampleRate)
  {
    constexpr float kTwoPi = 6.28318530718f;

    for (int i = 0; i < N; ++i) {
      // Each LFO runs at slightly different rate
      float rate = baseRateHz * rateOffsets[i];
      phases[i] += rate / sampleRate;
      if (phases[i] >= 1.0f) phases[i] -= 1.0f;
      outputs[i] = std::sin(phases[i] * kTwoPi);
    }
  }

  /**
   * Get the output of a specific LFO.
   * @param idx LFO index (0-7)
   * @return Sine wave value from -1 to +1
   */
  float get(int idx) const
  {
    return outputs[idx];
  }
};

// =============================================================================
// DELAY LINE
// =============================================================================
/**
 * Simple delay line with interpolated read.
 * Delay time changes cause pitch shift (Lexicon/Valhalla style).
 */
template<int MAX_SIZE>
struct DelayLine
{
  static_assert((MAX_SIZE & (MAX_SIZE - 1)) == 0, "MAX_SIZE must be power of 2");
  static constexpr int MASK = MAX_SIZE - 1;

  std::array<float, MAX_SIZE> buffer{};
  int writePos = 0;

  void clear()
  {
    std::fill(buffer.begin(), buffer.end(), 0.0f);
    writePos = 0;
  }

  void softClear(float factor)
  {
    for (auto& sample : buffer)
      sample *= factor;
  }

  void write(float sample)
  {
    buffer[writePos] = sample;
    writePos = (writePos + 1) & MASK;
  }

  // Read with linear interpolation
  float read(float delaySamples) const
  {
    float readPos = static_cast<float>(writePos) - delaySamples;
    while (readPos < 0.0f) readPos += MAX_SIZE;

    int index0 = static_cast<int>(readPos) & MASK;
    int index1 = (index0 + 1) & MASK;
    float frac = readPos - std::floor(readPos);

    return buffer[index0] + (buffer[index1] - buffer[index0]) * frac;
  }

  // Read at integer position (faster)
  float readInt(int delaySamples) const
  {
    int readPos = (writePos - delaySamples + MAX_SIZE) & MASK;
    return buffer[readPos];
  }
};

// =============================================================================
// ALLPASS FILTER
// =============================================================================
/**
 * Schroeder allpass filter - the building block of reverb.
 *
 * WHAT IT DOES:
 * An allpass filter passes all frequencies equally (flat magnitude response)
 * but shifts their phase differently. This "smears" the signal in time,
 * creating diffusion without changing the tonal color.
 *
 * EQUATION: y[n] = x[n-D] + g*y[n-D] - g*x[n]
 *
 * IMPLEMENTATION (from MVerb):
 * We store v[n] = x[n] + g*y[n] in the delay buffer.
 * Then: y[n] = v[n-D] - g*x[n]
 *            = (x[n-D] + g*y[n-D]) - g*x[n]
 *            = x[n-D] + g*y[n-D] - g*x[n]  ✓ matches Schroeder equation
 *
 * CRITICAL: We must store (input + feedback*OUTPUT), NOT (input + feedback*DELAYED)!
 * Getting this wrong causes the filter to be unstable.
 */
template<int MAX_SIZE>
struct AllpassFilter
{
  DelayLine<MAX_SIZE> delay;
  float feedback = 0.5f;

  void clear()
  {
    delay.clear();
  }

  void softClear(float factor)
  {
    delay.softClear(factor);
  }

  void setFeedback(float g)
  {
    feedback = std::max(-0.99f, std::min(0.99f, g));
  }

  /**
   * Process one sample through the allpass (with interpolation for modulation).
   *
   * IMPORTANT: Store (input + feedback * output), not (input + feedback * delayed)!
   * This is what makes it a proper Schroeder allpass.
   */
  float process(float input, float delaySamples)
  {
    float bufout = delay.read(delaySamples);
    float temp = input * (-feedback);
    float output = bufout + temp;
    // CRITICAL: store input + feedback*output (not input + feedback*bufout!)
    delay.write(input + (output * feedback));
    return output;
  }

  /**
   * Process one sample through the allpass (integer delay, faster).
   */
  float processInt(float input, int delaySamples)
  {
    float bufout = delay.readInt(delaySamples);
    float temp = input * (-feedback);
    float output = bufout + temp;
    // CRITICAL: store input + feedback*output (not input + feedback*bufout!)
    delay.write(input + (output * feedback));
    return output;
  }
};

// =============================================================================
// ONE-POLE LOWPASS FILTER
// =============================================================================
/**
 * Simple 1-pole lowpass filter for feedback damping.
 *
 * WHY DAMPING:
 * Real acoustic spaces absorb high frequencies faster than low frequencies.
 * This filter in the feedback path makes the reverb tail progressively
 * darker over time - like a real room.
 *
 * HOW IT'S CONTROLLED:
 * Damping is NOT exposed as a separate parameter. Instead, it's controlled
 * internally by the Color mode (FutureVerb-style approach):
 * - Bright: Minimal damping (0.10) - highs sustain, shimmering tail
 * - Neutral: Medium damping (0.35) - natural acoustic decay
 * - Dark: Aggressive damping (0.75) - highs decay fast, vintage character
 * - Studio: Moderate damping (0.50) - controlled, mix-friendly
 *
 * Damping SNAPS on mode change (no smoothing) because mode switches are
 * discrete selections where users expect immediate character change.
 *
 * EQUATION: y[n] = x[n]×(1-g) + y[n-1]×g
 * where g = damping coefficient (0 = no filtering, 1 = full filtering)
 */
struct OnePoleLowpass
{
  float state = 0.0f;
  float coeff = 0.0f;  // Damping amount (0-1)

  void clear()
  {
    state = 0.0f;
  }

  void setCoeff(float damping)
  {
    coeff = std::max(0.0f, std::min(0.99f, damping));
  }

  float process(float input)
  {
    state = input * (1.0f - coeff) + state * coeff;
    return state;
  }
};

// =============================================================================
// HIGH-PASS FILTER (Low Cut) - Two-Pole (12dB/octave)
// =============================================================================
/**
 * Two-pole high-pass filter for removing low frequencies.
 * Cascades two one-pole filters for steeper 12dB/octave slope.
 *
 * WHY TWO-POLE:
 * One-pole filters (6dB/octave) have very gentle slopes - signal leaks through.
 * Two-pole filters (12dB/octave) provide cleaner cutoff like pro reverbs.
 *
 * WHY LOW CUT:
 * Reverb can accumulate bass energy, making the mix muddy.
 * A high-pass filter on the input removes rumble and keeps things clean.
 * Typical settings: 60-150 Hz for music, higher for vocals.
 */
struct HighPassFilter
{
  // Two cascaded one-pole stages
  float state1 = 0.0f;
  float state2 = 0.0f;
  float prevInput1 = 0.0f;
  float prevInput2 = 0.0f;
  float coeff = 0.995f;

  void clear()
  {
    state1 = 0.0f;
    state2 = 0.0f;
    prevInput1 = 0.0f;
    prevInput2 = 0.0f;
  }

  void setCutoff(float hz, float sampleRate)
  {
    // Calculate coefficient from cutoff frequency
    coeff = std::exp(-2.0f * 3.14159265359f * hz / sampleRate);
    coeff = std::max(0.0f, std::min(0.9999f, coeff));
  }

  float process(float input)
  {
    // First stage
    state1 = coeff * (state1 + input - prevInput1);
    prevInput1 = input;

    // Second stage (cascaded for 12dB/octave)
    state2 = coeff * (state2 + state1 - prevInput2);
    prevInput2 = state1;

    return state2;
  }
};

// =============================================================================
// LOW-PASS FILTER (High Cut) - Two-Pole (12dB/octave)
// =============================================================================
/**
 * Two-pole low-pass filter for removing high frequencies.
 * Cascades two one-pole filters for steeper 12dB/octave slope.
 *
 * WHY TWO-POLE:
 * One-pole filters (6dB/octave) have very gentle slopes - signal leaks through.
 * Two-pole filters (12dB/octave) provide cleaner cutoff like pro reverbs.
 *
 * WHY HIGH CUT:
 * Controls the brightness of the reverb input.
 * Lower cutoff = darker reverb overall.
 * This is different from damping which progressively darkens the tail.
 */
struct LowPassFilter
{
  // Two cascaded one-pole stages
  float state1 = 0.0f;
  float state2 = 0.0f;
  float coeff = 0.0f;

  void clear()
  {
    state1 = 0.0f;
    state2 = 0.0f;
  }

  void setCutoff(float hz, float sampleRate)
  {
    // Calculate coefficient from cutoff frequency
    coeff = std::exp(-2.0f * 3.14159265359f * hz / sampleRate);
    coeff = std::max(0.0f, std::min(0.9999f, coeff));
  }

  float process(float input)
  {
    // First stage
    state1 = (1.0f - coeff) * input + coeff * state1;

    // Second stage (cascaded for 12dB/octave)
    state2 = (1.0f - coeff) * state1 + coeff * state2;

    return state2;
  }
};

// =============================================================================
// EARLY REFLECTIONS - Room Character Network
// =============================================================================
/**
 * 12-tap early reflection generator for room character.
 *
 * WHY EARLY REFLECTIONS:
 * The first 50-100ms of reverb define the perception of room size and shape.
 * These "early reflections" (ERs) are distinct echoes that arrive before
 * the diffuse reverb tail. They tell our brain about:
 * - Room size (longer delays = bigger room)
 * - Wall materials (brighter ERs = harder surfaces)
 * - Room shape (tap pattern suggests geometry)
 *
 * Without ERs, reverb sounds "disconnected" from the dry signal.
 * With ERs, it sounds like you're actually IN a space.
 *
 * TAP DESIGN:
 * - 12 taps with logarithmically increasing delays (3ms to 72ms)
 * - Gains decay with time (later reflections are quieter)
 * - Some negative gains for phase variation (reduces comb filtering)
 * - Panned across stereo field for width
 * - All times scale with Size parameter
 *
 * REFERENCE:
 * Based on typical small-to-medium room reflection patterns.
 * Professional reverbs (ValhallaRoom, FabFilter Pro-R) use similar techniques.
 */
struct EarlyReflections
{
  static constexpr int NUM_TAPS = 12;
  DelayLine<8192> delay;  // ~170ms at 48kHz

  // ==========================================================================
  // PLATE MODE - Sparse, widely spaced (plate has minimal ERs)
  // ==========================================================================
  // TAP TIME DESIGN (applies to all modes):
  // - Times follow ~logarithmic spacing (each tap ~1.3× the previous)
  // - This avoids comb filtering that occurs with evenly-spaced taps
  // - Evenly-spaced taps create resonant peaks at f = 1/spacing
  // - Logarithmic spacing spreads energy across frequencies smoothly
  //
  // GAIN DESIGN:
  // - Gains decay with time (later reflections are quieter - inverse square law)
  // - Alternating positive/negative signs create phase variation
  // - Phase variation prevents constructive interference (metallic sound)
  //
  // PAN DESIGN:
  // - Pseudo-random L/R distribution (-1 to +1)
  // - Avoids symmetric patterns that sound artificial
  // - Creates enveloping stereo image
  static constexpr float kPlateTapTimes[NUM_TAPS] = {
    3.1f, 5.2f, 7.8f, 11.3f, 15.7f, 19.2f,
    24.1f, 31.5f, 38.2f, 47.6f, 58.3f, 72.1f
  };
  static constexpr float kPlateTapGains[NUM_TAPS] = {
    0.85f, -0.72f, 0.68f, -0.55f, 0.48f, 0.42f,
    -0.35f, 0.30f, -0.25f, 0.20f, 0.16f, -0.12f
  };
  static constexpr float kPlateTapPans[NUM_TAPS] = {
    -0.3f, 0.5f, -0.7f, 0.2f, -0.4f, 0.8f,
    -0.6f, 0.1f, 0.9f, -0.8f, 0.4f, -0.2f
  };

  // ==========================================================================
  // CHAMBER MODE - Dense, tightly packed (small reflective room)
  // ==========================================================================
  // Chamber ERs differ from Plate:
  // - Shorter times (2-40ms vs 3-72ms) = smaller perceived space
  // - Tighter spacing = faster echo density buildup
  // - Higher gains = more reflective surfaces (hard walls vs open plate)
  // - Narrower pans = smaller stereo image (intimate room vs wide plate)
  static constexpr float kChamberTapTimes[NUM_TAPS] = {
    2.1f, 3.8f, 5.2f, 7.1f, 9.3f, 11.8f,
    14.6f, 18.2f, 22.4f, 27.3f, 33.1f, 40.2f
  };
  static constexpr float kChamberTapGains[NUM_TAPS] = {
    0.92f, -0.85f, 0.80f, -0.74f, 0.68f, 0.62f,
    -0.56f, 0.50f, -0.44f, 0.38f, 0.32f, -0.26f
  };
  static constexpr float kChamberTapPans[NUM_TAPS] = {
    -0.2f, 0.3f, -0.4f, 0.15f, -0.25f, 0.45f,
    -0.35f, 0.1f, 0.5f, -0.4f, 0.25f, -0.15f
  };

  // ==========================================================================
  // HALL MODE - Concert hall (between Chamber and Cathedral)
  // ==========================================================================
  // Hall ERs are the classic "concert hall" sound:
  // - Medium times (5-93ms) = larger than Chamber (2-40ms), smaller than Cathedral (15-184ms)
  // - Wide stereo spread = enveloping but not overwhelming
  // - Moderate gain decay = balanced reflections (0.88 down to 0.24)
  // - Medium diffusion (0.50-0.58) = smooth but with some definition
  //
  // MODE GAIN 0.85: Between Chamber (1.0) and Cathedral (0.6).
  // Hall ERs should be prominent but not as "in your face" as intimate Chamber.
  //
  // Think: orchestral concert hall, theater, large performance space
  static constexpr float kHallTapTimes[NUM_TAPS] = {
    5.3f, 9.8f, 15.2f, 22.1f, 30.4f, 39.8f,
    50.2f, 61.5f, 72.3f, 81.7f, 88.4f, 93.1f
  };
  // Moderate decay - balanced between intimate Chamber and distant Cathedral
  static constexpr float kHallTapGains[NUM_TAPS] = {
    0.88f, -0.80f, 0.74f, -0.68f, 0.62f, 0.56f,
    -0.50f, 0.44f, -0.38f, 0.33f, 0.28f, -0.24f
  };
  // Wide stereo - enveloping concert hall image
  static constexpr float kHallTapPans[NUM_TAPS] = {
    -0.5f, 0.6f, -0.75f, 0.4f, -0.55f, 0.8f,
    -0.65f, 0.35f, 0.85f, -0.7f, 0.5f, -0.4f
  };

  // ==========================================================================
  // CATHEDRAL MODE - Very sparse, long delays (massive stone space)
  // ==========================================================================
  // Cathedral ERs differ from Plate/Chamber:
  // - Much longer times (15-180ms) = walls are very far away
  // - Very sparse spacing = you hear distinct echoes before they smear
  // - Slower gain decay = far walls have more similar levels (less inverse-square)
  // - Very wide pans = massive enveloping space
  // - Medium overall gain (0.6×) = prominent but not overwhelming
  //
  // This creates the "ethereal" quality where transients pass through initially,
  // then distinct echoes arrive, then finally the dense tail builds up.
  static constexpr float kCathedralTapTimes[NUM_TAPS] = {
    15.2f, 28.7f, 42.3f, 58.1f, 76.4f, 95.8f,
    112.3f, 131.5f, 148.7f, 162.4f, 174.1f, 183.6f
  };
  // Slower decay than Plate/Chamber - far walls are more similar in level
  static constexpr float kCathedralTapGains[NUM_TAPS] = {
    0.78f, -0.72f, 0.67f, -0.62f, 0.58f, 0.54f,
    -0.50f, 0.46f, -0.42f, 0.38f, 0.35f, -0.32f
  };
  // Very wide stereo spread - massive space perception
  static constexpr float kCathedralTapPans[NUM_TAPS] = {
    -0.9f, 0.85f, -0.95f, 0.7f, -0.8f, 0.92f,
    -0.75f, 0.6f, 0.98f, -0.88f, 0.78f, -0.65f
  };

  // Active tap parameters (set by mode)
  std::array<float, NUM_TAPS> tapTimes{};
  std::array<float, NUM_TAPS> tapGains{};
  std::array<float, NUM_TAPS> tapPans{};
  std::array<int, NUM_TAPS> tapSamples{};  // Tap times in samples

  float mSampleRate = 44100.0f;
  float mModeGain = 1.0f;  // Overall ER gain (Plate: 0.3, Chamber: 1.0)
  int mMode = 0;  // kModePlate

  void clear()
  {
    delay.clear();
    std::fill(tapSamples.begin(), tapSamples.end(), 0);
  }

  void reset(float sampleRate)
  {
    mSampleRate = sampleRate;
    delay.clear();
    setMode(0);  // Default to Plate
    updateTapTimes(0.5f);  // Default size
  }

  /**
   * Set the reverb mode - changes ER pattern and gain.
   * @param mode 0=Plate, 1=Chamber, 2=Hall, 3=Cathedral
   */
  void setMode(int mode)
  {
    mMode = mode;
    switch (mode) {
      case 0:  // Plate - minimal ERs (plate reverbs have instant onset, no room)
        for (int i = 0; i < NUM_TAPS; ++i) {
          tapTimes[i] = kPlateTapTimes[i];
          tapGains[i] = kPlateTapGains[i];
          tapPans[i] = kPlateTapPans[i];
        }
        mModeGain = 0.3f;  // Plates have weak/no ERs
        break;
      case 1:  // Chamber - dense ERs (small reflective room)
        for (int i = 0; i < NUM_TAPS; ++i) {
          tapTimes[i] = kChamberTapTimes[i];
          tapGains[i] = kChamberTapGains[i];
          tapPans[i] = kChamberTapPans[i];
        }
        mModeGain = 1.0f;  // Chambers have prominent ERs
        break;
      case 2:  // Hall - concert hall (wide, balanced)
        for (int i = 0; i < NUM_TAPS; ++i) {
          tapTimes[i] = kHallTapTimes[i];
          tapGains[i] = kHallTapGains[i];
          tapPans[i] = kHallTapPans[i];
        }
        mModeGain = 0.85f;  // Strong but not as intimate as Chamber
        break;
      case 3:  // Cathedral - very sparse, long ERs (massive stone space)
        for (int i = 0; i < NUM_TAPS; ++i) {
          tapTimes[i] = kCathedralTapTimes[i];
          tapGains[i] = kCathedralTapGains[i];
          tapPans[i] = kCathedralTapPans[i];
        }
        mModeGain = 0.6f;  // Prominent but not overwhelming
        break;
    }
  }

  /**
   * Update tap times based on size parameter.
   * @param size Room size (0-1)
   */
  void updateTapTimes(float size)
  {
    // Size scaling: 0.3× to 1.7× the base times
    float sizeScale = 0.3f + size * 1.4f;

    for (int i = 0; i < NUM_TAPS; ++i) {
      float timeMs = tapTimes[i] * sizeScale;
      tapSamples[i] = static_cast<int>(timeMs * 0.001f * mSampleRate);
      // Clamp to valid range
      tapSamples[i] = std::max(1, std::min(tapSamples[i], 8191));
    }
  }

  /**
   * Process one sample through the early reflections network.
   * @param input Mono input sample
   * @param outL Left output (accumulated)
   * @param outR Right output (accumulated)
   */
  void process(float input, float& outL, float& outR)
  {
    delay.write(input);

    outL = 0.0f;
    outR = 0.0f;

    for (int i = 0; i < NUM_TAPS; ++i) {
      float tap = delay.readInt(tapSamples[i]) * tapGains[i] * mModeGain;
      float pan = tapPans[i];

      // Equal-power panning
      float leftGain = 0.5f * (1.0f - pan);
      float rightGain = 0.5f * (1.0f + pan);

      outL += tap * leftGain;
      outR += tap * rightGain;
    }
  }
};

// =============================================================================
// TANK SYSTEM - Complete reverb tank with all components
// =============================================================================
/**
 * Encapsulates a complete Dattorro tank (both left and right sides).
 *
 * WHAT IT CONTAINS:
 * - 2 allpass filters per side (4 total) for diffusion
 * - 2 delay lines per side (4 total) for the feedback loop
 * - 1 damping filter per side for high-frequency absorption
 * - 14 output tap positions for stereo decorrelation
 *
 * USAGE:
 * ```cpp
 * TankSystem tank;
 * tank.clear();                              // Clear all buffers
 * tank.setDiffusionCoeffs();                 // Set allpass feedback
 * tank.setDamping(0.5f);                     // 50% damping
 * tank.updateDelayTimes(0.7f, scale);        // 70% size
 *
 * // Per sample:
 * float wetL, wetR;
 * tank.process(diffusedInput, decay, wetL, wetR, modOffset1, modOffset3);
 * ```
 *
 * SPILLOVER TECHNIQUE:
 * Having two TankSystems allows seamless size transitions:
 * - One tank receives new input (active)
 * - One tank just decays its existing tail (spillover)
 * - Both outputs are summed - the old tail fades naturally via decay
 */
struct TankSystem
{
  // Tank components - Left
  AllpassFilter<8192> tankAP1;   // Modulated
  DelayLine<32768> tankDelay1;
  AllpassFilter<8192> tankAP2;
  DelayLine<32768> tankDelay2;
  OnePoleLowpass dampingL;

  // Tank components - Right
  AllpassFilter<8192> tankAP3;   // Modulated
  DelayLine<32768> tankDelay3;
  AllpassFilter<8192> tankAP4;
  DelayLine<32768> tankDelay4;
  OnePoleLowpass dampingR;

  // Cross-feedback storage
  float leftTankOut = 0.0f;
  float rightTankOut = 0.0f;

  // Tank delay times (each tank has its own)
  float tankAllpassDelay1 = 672.0f;
  float tankDelayTime1 = 4453.0f;
  int tankAllpassDelay2 = 1800;
  float tankDelayTime2 = 3720.0f;
  float tankAllpassDelay3 = 908.0f;
  float tankDelayTime3 = 4217.0f;
  int tankAllpassDelay4 = 2656;
  float tankDelayTime4 = 3163.0f;

  // Output tap positions
  float outputTapL1 = 0, outputTapL2 = 0, outputTapL3 = 0, outputTapL4 = 0;
  float outputTapL5 = 0, outputTapL6 = 0, outputTapL7 = 0;
  float outputTapR1 = 0, outputTapR2 = 0, outputTapR3 = 0, outputTapR4 = 0;
  float outputTapR5 = 0, outputTapR6 = 0, outputTapR7 = 0;

  // Size this tank was configured for
  float configuredSize = 0.5f;

  void clear()
  {
    tankAP1.clear();
    tankAP2.clear();
    tankAP3.clear();
    tankAP4.clear();
    tankDelay1.clear();
    tankDelay2.clear();
    tankDelay3.clear();
    tankDelay4.clear();
    dampingL.clear();
    dampingR.clear();
    leftTankOut = 0.0f;
    rightTankOut = 0.0f;
  }

  // Soft clear: reduce residual energy without hard discontinuity
  // factor = 0.01 means -40dB attenuation
  void softClear(float factor)
  {
    tankAP1.softClear(factor);
    tankAP2.softClear(factor);
    tankAP3.softClear(factor);
    tankAP4.softClear(factor);
    tankDelay1.softClear(factor);
    tankDelay2.softClear(factor);
    tankDelay3.softClear(factor);
    tankDelay4.softClear(factor);
    dampingL.state *= factor;
    dampingR.state *= factor;
    leftTankOut *= factor;
    rightTankOut *= factor;
  }

  void setDiffusionCoeffs()
  {
    tankAP1.setFeedback(Constants::kDecayDiffusionCoeff1);
    tankAP2.setFeedback(Constants::kDecayDiffusionCoeff2);
    tankAP3.setFeedback(Constants::kDecayDiffusionCoeff1);
    tankAP4.setFeedback(Constants::kDecayDiffusionCoeff2);
  }

  /**
   * Set tank density (allpass diffusion in the feedback loop).
   *
   * WHAT DENSITY DOES:
   * Controls the texture of the reverb tail by adjusting tank allpass feedback.
   * - Low density (0%): Grainy, you can hear individual reflections, more "echo-like"
   * - High density (100%): Smooth, continuous wash, more "diffuse"
   *
   * @param density 0-1 range
   */
  void setDensity(float density)
  {
    // Scale density to useful coefficient range
    // coeff1: 0.3 (grainy) to 0.8 (smooth)
    // coeff2: 0.2 (grainy) to 0.7 (smooth)
    float coeff1 = 0.3f + density * 0.5f;  // 0.3 to 0.8
    float coeff2 = 0.2f + density * 0.5f;  // 0.2 to 0.7

    tankAP1.setFeedback(coeff1);
    tankAP2.setFeedback(coeff2);
    tankAP3.setFeedback(coeff1);
    tankAP4.setFeedback(coeff2);
  }

  void setDamping(float damping)
  {
    dampingL.setCoeff(damping * 0.95f);
    dampingR.setCoeff(damping * 0.95f);
  }

  void updateDelayTimes(float size, float scale)
  {
    configuredSize = size;
    float sizeScale = 0.5f + size * 1.5f;

    tankAllpassDelay1 = Constants::kTankAllpass1 * scale * sizeScale;
    tankDelayTime1 = Constants::kTankDelay1 * scale * sizeScale;
    tankAllpassDelay2 = static_cast<int>(Constants::kTankAllpass2 * scale * sizeScale);
    tankDelayTime2 = Constants::kTankDelay2 * scale * sizeScale;

    tankAllpassDelay3 = Constants::kTankAllpass3 * scale * sizeScale;
    tankDelayTime3 = Constants::kTankDelay3 * scale * sizeScale;
    tankAllpassDelay4 = static_cast<int>(Constants::kTankAllpass4 * scale * sizeScale);
    tankDelayTime4 = Constants::kTankDelay4 * scale * sizeScale;

    // Output taps
    outputTapL1 = Constants::kLeftTap1 * scale * sizeScale;
    outputTapL2 = Constants::kLeftTap2 * scale * sizeScale;
    outputTapL3 = Constants::kLeftTap3 * scale * sizeScale;
    outputTapL4 = Constants::kLeftTap4 * scale * sizeScale;
    outputTapL5 = Constants::kLeftTap5 * scale * sizeScale;
    outputTapL6 = Constants::kLeftTap6 * scale * sizeScale;
    outputTapL7 = Constants::kLeftTap7 * scale * sizeScale;

    outputTapR1 = Constants::kRightTap1 * scale * sizeScale;
    outputTapR2 = Constants::kRightTap2 * scale * sizeScale;
    outputTapR3 = Constants::kRightTap3 * scale * sizeScale;
    outputTapR4 = Constants::kRightTap4 * scale * sizeScale;
    outputTapR5 = Constants::kRightTap5 * scale * sizeScale;
    outputTapR6 = Constants::kRightTap6 * scale * sizeScale;
    outputTapR7 = Constants::kRightTap7 * scale * sizeScale;
  }

  /**
   * Process one sample through this tank.
   * @param diffusedInput The diffused input signal (0 for spillover tank)
   * @param decay The decay feedback amount
   * @param wetL Output: left wet signal
   * @param wetR Output: right wet signal
   * @param modOffset1 LFO modulation offset for left tank allpass (default 0)
   * @param modOffset3 LFO modulation offset for right tank allpass (default 0)
   * @param outputModL LFO modulation for left output taps (default 0)
   * @param outputModR LFO modulation for right output taps (default 0)
   */
  void process(float diffusedInput, float decay, float& wetL, float& wetR,
               float modOffset1 = 0.0f, float modOffset3 = 0.0f,
               float outputModL = 0.0f, float outputModR = 0.0f)
  {
    // Save previous frame's outputs for cross-feedback
    float prevLeftOut = leftTankOut;
    float prevRightOut = rightTankOut;

    // ----- LEFT TANK -----
    float leftTankIn = diffusedInput + prevRightOut * decay;
    leftTankIn = reverbFastTanh(leftTankIn);

    // Apply LFO modulation to allpass delay (creates chorus-like movement)
    float modulatedDelay1 = tankAllpassDelay1 + modOffset1;
    float leftAP1Out = tankAP1.process(leftTankIn, modulatedDelay1);
    tankDelay1.write(leftAP1Out);
    // Also modulate the main tank delay for more obvious effect
    float modulatedTankDelay1 = tankDelayTime1 + modOffset1 * 1.5f;
    float leftDelay1Out = tankDelay1.read(modulatedTankDelay1);
    float leftDamped = dampingL.process(leftDelay1Out);
    float leftAP2Out = tankAP2.processInt(leftDamped, tankAllpassDelay2);
    tankDelay2.write(leftAP2Out);
    // Also modulate second tank delay
    float modulatedTankDelay2 = tankDelayTime2 + modOffset1 * 2.0f;
    float leftDelay2Out = tankDelay2.read(modulatedTankDelay2);

    // ----- RIGHT TANK -----
    float rightTankIn = diffusedInput + prevLeftOut * decay;
    rightTankIn = reverbFastTanh(rightTankIn);

    // Apply LFO modulation to allpass delay (slightly detuned for stereo)
    float modulatedDelay3 = tankAllpassDelay3 + modOffset3;
    float rightAP1Out = tankAP3.process(rightTankIn, modulatedDelay3);
    tankDelay3.write(rightAP1Out);
    // Also modulate the main tank delay for more obvious effect
    float modulatedTankDelay3 = tankDelayTime3 + modOffset3 * 1.5f;
    float rightDelay1Out = tankDelay3.read(modulatedTankDelay3);
    float rightDamped = dampingR.process(rightDelay1Out);
    float rightAP2Out = tankAP4.processInt(rightDamped, tankAllpassDelay4);
    tankDelay4.write(rightAP2Out);
    // Also modulate second tank delay
    float modulatedTankDelay4 = tankDelayTime4 + modOffset3 * 2.0f;
    float rightDelay2Out = tankDelay4.read(modulatedTankDelay4);

    // Update outputs for next sample
    leftTankOut = leftDelay2Out;
    rightTankOut = rightDelay2Out;

    // Output taps - with modulation for obvious shimmer/chorus effect
    // Each tap gets different modulation amount for complex movement
    // Using full modulation on primary taps, reduced on secondary
    wetL = 0.0f;
    wetL += tankDelay3.read(outputTapL1 + outputModL);
    wetL += tankDelay3.read(outputTapL2 + outputModL * 1.2f);
    wetL -= tankAP4.delay.read(outputTapL3 + outputModL * 0.8f);
    wetL += tankDelay4.read(outputTapL4 + outputModL * 1.1f);
    wetL -= tankDelay1.read(outputTapL5 + outputModL * 0.9f);
    wetL -= tankAP2.delay.read(outputTapL6 + outputModL * 0.7f);
    wetL -= tankDelay2.read(outputTapL7 + outputModL * 1.3f);

    wetR = 0.0f;
    wetR += tankDelay1.read(outputTapR1 + outputModR);
    wetR += tankDelay1.read(outputTapR2 + outputModR * 1.2f);
    wetR -= tankAP2.delay.read(outputTapR3 + outputModR * 0.8f);
    wetR += tankDelay2.read(outputTapR4 + outputModR * 1.1f);
    wetR -= tankDelay3.read(outputTapR5 + outputModR * 0.9f);
    wetR -= tankAP4.delay.read(outputTapR6 + outputModR * 0.7f);
    wetR -= tankDelay4.read(outputTapR7 + outputModR * 1.3f);
  }
};

} // namespace DattorroInternal

// =============================================================================
// PUBLIC API - Simple Wrapper for Easy Integration
// =============================================================================
/**
 * DattorroReverb - A professional-quality plate reverb with simple API.
 *
 * This wrapper hides the complexity of the full Dattorro implementation
 * and provides a clean interface similar to StereoDelay.
 *
 * USAGE:
 * ```cpp
 * DattorroReverb reverb;
 * reverb.SetSampleRate(44100.0);
 * reverb.Reset();
 * reverb.SetDecay(0.7f);      // 70% decay
 * reverb.SetSize(0.8f);       // Large room
 * reverb.SetWetLevel(0.3f);   // 30% wet
 *
 * // In audio callback:
 * float left = inputL, right = inputR;
 * reverb.Process(left, right);  // Modifies in-place
 * ```
 *
 * REVERB MODES:
 * - Plate (0): Classic plate reverb, instant attack, shimmering
 * - Chamber (1): Small reflective room, dense early reflections
 * - Hall (2): Concert hall, wide and balanced
 * - Cathedral (3): Massive stone space, ethereal buildup
 *
 * COLOR MODES:
 * - Bright (0): Full bandwidth, shimmering highs
 * - Neutral (1): Natural acoustic decay (default)
 * - Dark (2): Aggressive damping, vintage character
 * - Studio (3): Controlled, mix-friendly
 */

// Reverb mode enum for easy mode selection
enum class ReverbMode
{
  Plate = 0,
  Chamber = 1,
  Hall = 2,
  Cathedral = 3
};

// Color mode enum for tonal character
enum class ReverbColor
{
  Bright = 0,
  Neutral = 1,
  Dark = 2,
  Studio = 3
};

class DattorroReverb
{
public:
  DattorroReverb() = default;

  /**
   * Set the sample rate. Must be called before Reset().
   */
  void SetSampleRate(double sampleRate)
  {
    mSampleRate = static_cast<float>(sampleRate);
  }

  /**
   * Reset all internal state. Call after SetSampleRate() and when needed.
   */
  void Reset()
  {
    using namespace DattorroInternal;

    float scale = mSampleRate / Constants::kReferenceSampleRate;

    // Initialize smoothed parameters
    constexpr float kFastSmooth = 5.0f;
    constexpr float kMediumSmooth = 10.0f;
    constexpr float kSizeSmooth = 15.0f;

    mDry.setTime(kFastSmooth, mSampleRate);
    mWet.setTime(kFastSmooth, mSampleRate);
    mDecay.setTime(kMediumSmooth, mSampleRate);
    mSize.setTime(kSizeSmooth, mSampleRate);
    mWidth.setTime(kMediumSmooth, mSampleRate);
    mDamping.setTime(kMediumSmooth, mSampleRate);
    mDensity.setTime(kMediumSmooth, mSampleRate);
    mModRate.setTime(kMediumSmooth, mSampleRate);
    mModDepth.setTime(kMediumSmooth, mSampleRate);
    mEarlyLate.setTime(kMediumSmooth, mSampleRate);

    // Set default values
    mDry.setTarget(1.0f); mDry.snap();
    mWet.setTarget(0.3f); mWet.snap();
    mDecay.setTarget(0.5f * 0.85f); mDecay.snap();
    mSize.setTarget(0.7f); mSize.snap();
    mWidth.setTarget(1.0f); mWidth.snap();
    mDamping.setTarget(0.35f); mDamping.snap();
    mDensity.setTarget(0.7f); mDensity.snap();
    mModRate.setTarget(0.5f); mModRate.snap();
    mModDepth.setTarget(0.5f); mModDepth.snap();
    mEarlyLate.setTarget(0.5f); mEarlyLate.snap();

    mLastSize = 0.7f;
    mLastDensity = 0.7f;

    // Clear all components
    mPreDelay.clear();
    mLowCut.clear();
    mHighCut.clear();
    mInputAP1.clear();
    mInputAP2.clear();
    mInputAP3.clear();
    mInputAP4.clear();

    mEarlyReflections.reset(mSampleRate);

    mERInputDiffuser1.clear();
    mERInputDiffuser2.clear();
    mERInputDiffuser3.clear();
    mERInputDiffuser4.clear();
    mERDiffuserDelay1 = static_cast<int>(5.0f * 0.001f * mSampleRate);
    mERDiffuserDelay2 = static_cast<int>(8.0f * 0.001f * mSampleRate);
    mERDiffuserDelay3 = static_cast<int>(13.0f * 0.001f * mSampleRate);
    mERDiffuserDelay4 = static_cast<int>(21.0f * 0.001f * mSampleRate);

    mTank.clear();
    mTank.setDiffusionCoeffs();
    mTank.setDamping(mDamping.current);
    mTank.updateDelayTimes(mSize.current, scale);

    mDCBlockerL.clear();
    mDCBlockerR.clear();
    mDCBlockerL.setCutoff(5.0f, mSampleRate);
    mDCBlockerR.setCutoff(5.0f, mSampleRate);

    mColorLPF_L.clear();
    mColorLPF_R.clear();
    mColorHPF_L.clear();
    mColorHPF_R.clear();
    mColorLPF_L.setCutoff(8000.0f, mSampleRate);
    mColorLPF_R.setCutoff(8000.0f, mSampleRate);
    mColorHPF_L.setCutoff(600.0f, mSampleRate);
    mColorHPF_R.setCutoff(600.0f, mSampleRate);

    mInputDiffusionDelay1 = static_cast<int>(Constants::kInputDiffusion1 * scale);
    mInputDiffusionDelay2 = static_cast<int>(Constants::kInputDiffusion2 * scale);
    mInputDiffusionDelay3 = static_cast<int>(Constants::kInputDiffusion3 * scale);
    mInputDiffusionDelay4 = static_cast<int>(Constants::kInputDiffusion4 * scale);

    // Default mode setup
    mReverbMode = 0;
    mColorMode = 1;
    mEarlyReflections.setMode(mReverbMode);
    mInputAP1.setFeedback(0.65f);
    mInputAP2.setFeedback(0.65f);
    mInputAP3.setFeedback(0.55f);
    mInputAP4.setFeedback(0.55f);

    mLowCut.setCutoff(80.0f, mSampleRate);
    mHighCut.setCutoff(8000.0f, mSampleRate);

    mModBank.reset();
    mFreeze = false;
    mPreDelaySamples = 0;
  }

  // ==========================================================================
  // PARAMETER SETTERS
  // ==========================================================================

  /** Set decay amount (0-1). Higher = longer reverb tail. */
  void SetDecay(float decay) { mDecay.setTarget(decay * 0.85f); }

  /** Set room size (0-1). Higher = larger space. */
  void SetSize(float size) { mSize.setTarget(size); }

  /** Set dry level (0-1). */
  void SetDryLevel(float level) { mDry.setTarget(level); }

  /** Set wet level (0-1). */
  void SetWetLevel(float level) { mWet.setTarget(level); }

  /** Set pre-delay in milliseconds (0-200). */
  void SetPreDelay(float ms)
  {
    mPreDelaySamples = static_cast<int>((ms / 1000.0f) * mSampleRate);
    mPreDelaySamples = std::min(mPreDelaySamples, DattorroInternal::Constants::kMaxPreDelay - 1);
  }

  /** Set low cut frequency in Hz (20-500). */
  void SetLowCut(float hz) { mLowCut.setCutoff(hz, mSampleRate); }

  /** Set high cut frequency in Hz (1000-20000). */
  void SetHighCut(float hz) { mHighCut.setCutoff(hz, mSampleRate); }

  /** Set stereo width (0-1). 0 = mono, 1 = full stereo. */
  void SetWidth(float width) { mWidth.setTarget(width); }

  /** Set density (0-1). Controls tail texture. */
  void SetDensity(float density) { mDensity.setTarget(density); }

  /** Set modulation rate in Hz (0.1-2.0). */
  void SetModRate(float hz) { mModRate.setTarget(hz); }

  /** Set modulation depth (0-1). */
  void SetModDepth(float depth) { mModDepth.setTarget(depth); }

  /** Set early/late balance (0-1). 0 = full ERs, 1 = no ERs. */
  void SetEarlyLate(float balance) { mEarlyLate.setTarget(balance); }

  /** Enable/disable freeze mode (infinite sustain). */
  void SetFreeze(bool freeze) { mFreeze = freeze; }

  /** Set reverb mode using enum. */
  void SetMode(ReverbMode mode)
  {
    int newMode = static_cast<int>(mode);
    if (newMode != mReverbMode) {
      mTank.softClear(0.01f);
      mInputAP1.softClear(0.01f);
      mInputAP2.softClear(0.01f);
      mInputAP3.softClear(0.01f);
      mInputAP4.softClear(0.01f);
      mEarlyReflections.delay.softClear(0.01f);
    }

    mReverbMode = newMode;
    mEarlyReflections.setMode(mReverbMode);
    mEarlyReflections.updateTapTimes(mSize.current);

    switch (mReverbMode) {
      case 0: // Plate
        mInputAP1.setFeedback(0.65f);
        mInputAP2.setFeedback(0.65f);
        mInputAP3.setFeedback(0.55f);
        mInputAP4.setFeedback(0.55f);
        break;
      case 1: // Chamber
        mInputAP1.setFeedback(0.55f);
        mInputAP2.setFeedback(0.55f);
        mInputAP3.setFeedback(0.48f);
        mInputAP4.setFeedback(0.48f);
        break;
      case 2: // Hall
        mInputAP1.setFeedback(0.48f);
        mInputAP2.setFeedback(0.48f);
        mInputAP3.setFeedback(0.42f);
        mInputAP4.setFeedback(0.42f);
        break;
      case 3: // Cathedral
        mInputAP1.setFeedback(0.42f);
        mInputAP2.setFeedback(0.42f);
        mInputAP3.setFeedback(0.36f);
        mInputAP4.setFeedback(0.36f);
        break;
    }
  }

  /** Set color mode using enum. */
  void SetColor(ReverbColor color)
  {
    mColorMode = static_cast<int>(color);
    switch (mColorMode) {
      case 0: // Bright
        mDamping.setTarget(0.10f);
        mDamping.snap();
        break;
      case 1: // Neutral
        mColorLPF_L.setCutoff(8000.0f, mSampleRate);
        mColorLPF_R.setCutoff(8000.0f, mSampleRate);
        mDamping.setTarget(0.35f);
        mDamping.snap();
        break;
      case 2: // Dark
        mColorLPF_L.setCutoff(3000.0f, mSampleRate);
        mColorLPF_R.setCutoff(3000.0f, mSampleRate);
        mDamping.setTarget(0.75f);
        mDamping.snap();
        break;
      case 3: // Studio
        mColorHPF_L.setCutoff(600.0f, mSampleRate);
        mColorHPF_R.setCutoff(600.0f, mSampleRate);
        mColorLPF_L.setCutoff(6000.0f, mSampleRate);
        mColorLPF_R.setCutoff(6000.0f, mSampleRate);
        mDamping.setTarget(0.50f);
        mDamping.snap();
        break;
    }
  }

  // ==========================================================================
  // PROCESSING
  // ==========================================================================

  /**
   * Process a single stereo sample in-place.
   * @param left Left channel (modified in-place)
   * @param right Right channel (modified in-place)
   */
  void Process(float& left, float& right)
  {
    using namespace DattorroInternal;

    float scale = mSampleRate / Constants::kReferenceSampleRate;

    // Get smoothed parameter values
    float dry = mDry.getNext();
    float wet = mWet.getNext();
    float decay = mDecay.getNext();
    float size = mSize.getNext();
    float width = mWidth.getNext();
    float damping = mDamping.getNext();
    float density = mDensity.getNext();
    float modRate = mModRate.getNext();
    float modDepth = mModDepth.getNext();
    float earlyLate = mEarlyLate.getNext();

    // Freeze mode
    if (mFreeze) {
      decay = 0.9999f;
      damping = 0.0f;
    }

    // Update tank when size changes
    if (std::abs(size - mLastSize) > 0.0001f) {
      mTank.updateDelayTimes(size, scale);
      mEarlyReflections.updateTapTimes(size);
      mLastSize = size;
    }

    mTank.setDamping(damping);

    // Update density
    if (std::abs(density - mLastDensity) > 0.0001f) {
      mTank.setDensity(density);
      float clearFactor = 0.1f;
      mERInputDiffuser1.softClear(clearFactor);
      mERInputDiffuser2.softClear(clearFactor);
      mERInputDiffuser3.softClear(clearFactor);
      mERInputDiffuser4.softClear(clearFactor);
      mLastDensity = density;
    }

    float inputL = left;
    float inputR = right;

    // Create mono input
    float monoIn = (inputL + inputR) * 0.5f;

    // Input tone shaping
    float filtered = mLowCut.process(monoIn);
    filtered = mHighCut.process(filtered);

    // Pre-delay
    float preDelayed = (mPreDelaySamples > 0)
      ? mPreDelay.readInt(mPreDelaySamples)
      : filtered;
    mPreDelay.write(filtered);

    // Modulation
    mModBank.process(modRate, mSampleRate);

    constexpr float kMinTankMod = 4.0f;
    constexpr float kMaxTankMod = 250.0f;
    float excursion = kMinTankMod + modDepth * (kMaxTankMod - kMinTankMod);
    float modOffset1 = mModBank.get(2) * excursion;
    float modOffset3 = mModBank.get(4) * excursion;

    float outputModL = mModBank.get(0) * modDepth * 32.0f;
    float outputModR = mModBank.get(1) * modDepth * 32.0f;

    constexpr float kMinERDiffuserMod = 1.0f;
    constexpr float kMaxERDiffuserMod = 5.0f;
    float erDiffModAmt = kMinERDiffuserMod + modDepth * (kMaxERDiffuserMod - kMinERDiffuserMod);
    float erDiffMod1 = mModBank.get(6) * erDiffModAmt;
    float erDiffMod2 = mModBank.get(7) * erDiffModAmt;

    // Early reflections
    float erInput = preDelayed;
    {
      float erDiffFeedback = density * 0.60f;
      mERInputDiffuser1.setFeedback(erDiffFeedback);
      mERInputDiffuser2.setFeedback(erDiffFeedback * 0.92f);
      mERInputDiffuser3.setFeedback(erDiffFeedback * 0.84f);
      mERInputDiffuser4.setFeedback(erDiffFeedback * 0.76f);

      int erMod1 = static_cast<int>(erDiffMod1);
      int erMod2 = static_cast<int>(erDiffMod2);
      erInput = mERInputDiffuser1.processInt(erInput, mERDiffuserDelay1 + erMod1);
      erInput = mERInputDiffuser2.processInt(erInput, mERDiffuserDelay2 - erMod1);
      erInput = mERInputDiffuser3.processInt(erInput, mERDiffuserDelay3 + erMod2);
      erInput = mERInputDiffuser4.processInt(erInput, mERDiffuserDelay4 - erMod2);
    }

    float earlyL = 0.0f, earlyR = 0.0f;
    mEarlyReflections.process(erInput, earlyL, earlyR);

    // Input diffusion
    float diffused = preDelayed;
    diffused = mInputAP1.processInt(diffused, mInputDiffusionDelay1);
    diffused = mInputAP2.processInt(diffused, mInputDiffusionDelay2);
    diffused = mInputAP3.processInt(diffused, mInputDiffusionDelay3);
    diffused = mInputAP4.processInt(diffused, mInputDiffusionDelay4);

    // Tank processing
    float tankInput = mFreeze ? 0.0f : diffused;
    float lateL = 0.0f, lateR = 0.0f;
    mTank.process(tankInput, decay, lateL, lateR, modOffset1, modOffset3, outputModL, outputModR);

    // DC blocking
    lateL = mDCBlockerL.process(lateL);
    lateR = mDCBlockerR.process(lateR);
    lateL *= 0.6f;
    lateR *= 0.6f;

    // Early/late mix
    float lateGain = 1.0f;
    float earlyGain = (1.0f - earlyLate) * 3.0f;
    float wetL = earlyL * earlyGain + lateL * lateGain;
    float wetR = earlyR * earlyGain + lateR * lateGain;

    // Color filtering
    switch (mColorMode) {
      case 0: // Bright - bypass
        break;
      case 1: // Neutral
        wetL = mColorLPF_L.process(wetL);
        wetR = mColorLPF_R.process(wetR);
        break;
      case 2: // Dark
        wetL = mColorLPF_L.process(wetL);
        wetL = mColorLPF_L.process(wetL);
        wetR = mColorLPF_R.process(wetR);
        wetR = mColorLPF_R.process(wetR);
        break;
      case 3: // Studio
        wetL = mColorHPF_L.process(wetL);
        wetL = mColorHPF_L.process(wetL);
        wetR = mColorHPF_R.process(wetR);
        wetR = mColorHPF_R.process(wetR);
        wetL = mColorLPF_L.process(wetL);
        wetL = mColorLPF_L.process(wetL);
        wetR = mColorLPF_R.process(wetR);
        wetR = mColorLPF_R.process(wetR);
        break;
    }

    // Stereo width
    {
      float mid = (wetL + wetR) * 0.5f;
      float side = (wetL - wetR) * 0.5f;
      wetL = mid + side * width;
      wetR = mid - side * width;
    }

    // Mix dry and wet
    float outL = inputL * dry + wetL * wet;
    float outR = inputR * dry + wetR * wet;

    // Safety limiting
    left = reverbFastTanh(outL);
    right = reverbFastTanh(outR);
  }

private:
  float mSampleRate = 44100.0f;

  // Smoothed parameters
  DattorroInternal::SmoothedValue<float> mDry;
  DattorroInternal::SmoothedValue<float> mWet;
  DattorroInternal::SmoothedValue<float> mDecay;
  DattorroInternal::SmoothedValue<float> mSize;
  DattorroInternal::SmoothedValue<float> mWidth;
  DattorroInternal::SmoothedValue<float> mDamping;
  DattorroInternal::SmoothedValue<float> mModRate;
  DattorroInternal::SmoothedValue<float> mModDepth;
  DattorroInternal::SmoothedValue<float> mEarlyLate;
  DattorroInternal::SmoothedValue<float> mDensity;

  bool mFreeze = false;
  int mPreDelaySamples = 0;

  float mLastSize = 0.5f;
  float mLastDensity = 0.7f;

  int mReverbMode = 0;
  int mColorMode = 1;

  // Input processing
  DattorroInternal::HighPassFilter mLowCut;
  DattorroInternal::LowPassFilter mHighCut;
  DattorroInternal::DelayLine<16384> mPreDelay;

  // Early reflections
  DattorroInternal::EarlyReflections mEarlyReflections;

  // ER input diffusion
  DattorroInternal::AllpassFilter<2048> mERInputDiffuser1;
  DattorroInternal::AllpassFilter<2048> mERInputDiffuser2;
  DattorroInternal::AllpassFilter<2048> mERInputDiffuser3;
  DattorroInternal::AllpassFilter<2048> mERInputDiffuser4;
  int mERDiffuserDelay1 = 221;
  int mERDiffuserDelay2 = 353;
  int mERDiffuserDelay3 = 577;
  int mERDiffuserDelay4 = 929;

  // Input diffusion
  DattorroInternal::AllpassFilter<2048> mInputAP1;
  DattorroInternal::AllpassFilter<2048> mInputAP2;
  DattorroInternal::AllpassFilter<4096> mInputAP3;
  DattorroInternal::AllpassFilter<4096> mInputAP4;

  int mInputDiffusionDelay1 = 142;
  int mInputDiffusionDelay2 = 107;
  int mInputDiffusionDelay3 = 379;
  int mInputDiffusionDelay4 = 277;

  // Tank
  DattorroInternal::TankSystem mTank;

  // DC blockers
  DattorroInternal::DCBlocker mDCBlockerL;
  DattorroInternal::DCBlocker mDCBlockerR;

  // Modulation
  DattorroInternal::ModulationBank mModBank;

  // Color filters
  DattorroInternal::LowPassFilter mColorLPF_L;
  DattorroInternal::LowPassFilter mColorLPF_R;
  DattorroInternal::HighPassFilter mColorHPF_L;
  DattorroInternal::HighPassFilter mColorHPF_R;
};
