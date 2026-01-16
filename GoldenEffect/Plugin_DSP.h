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

using namespace iplug;

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
inline float fastTanh(float x)
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

namespace DattorroConstants
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
    tankAP1.setFeedback(DattorroConstants::kDecayDiffusionCoeff1);
    tankAP2.setFeedback(DattorroConstants::kDecayDiffusionCoeff2);
    tankAP3.setFeedback(DattorroConstants::kDecayDiffusionCoeff1);
    tankAP4.setFeedback(DattorroConstants::kDecayDiffusionCoeff2);
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

    tankAllpassDelay1 = DattorroConstants::kTankAllpass1 * scale * sizeScale;
    tankDelayTime1 = DattorroConstants::kTankDelay1 * scale * sizeScale;
    tankAllpassDelay2 = static_cast<int>(DattorroConstants::kTankAllpass2 * scale * sizeScale);
    tankDelayTime2 = DattorroConstants::kTankDelay2 * scale * sizeScale;

    tankAllpassDelay3 = DattorroConstants::kTankAllpass3 * scale * sizeScale;
    tankDelayTime3 = DattorroConstants::kTankDelay3 * scale * sizeScale;
    tankAllpassDelay4 = static_cast<int>(DattorroConstants::kTankAllpass4 * scale * sizeScale);
    tankDelayTime4 = DattorroConstants::kTankDelay4 * scale * sizeScale;

    // Output taps
    outputTapL1 = DattorroConstants::kLeftTap1 * scale * sizeScale;
    outputTapL2 = DattorroConstants::kLeftTap2 * scale * sizeScale;
    outputTapL3 = DattorroConstants::kLeftTap3 * scale * sizeScale;
    outputTapL4 = DattorroConstants::kLeftTap4 * scale * sizeScale;
    outputTapL5 = DattorroConstants::kLeftTap5 * scale * sizeScale;
    outputTapL6 = DattorroConstants::kLeftTap6 * scale * sizeScale;
    outputTapL7 = DattorroConstants::kLeftTap7 * scale * sizeScale;

    outputTapR1 = DattorroConstants::kRightTap1 * scale * sizeScale;
    outputTapR2 = DattorroConstants::kRightTap2 * scale * sizeScale;
    outputTapR3 = DattorroConstants::kRightTap3 * scale * sizeScale;
    outputTapR4 = DattorroConstants::kRightTap4 * scale * sizeScale;
    outputTapR5 = DattorroConstants::kRightTap5 * scale * sizeScale;
    outputTapR6 = DattorroConstants::kRightTap6 * scale * sizeScale;
    outputTapR7 = DattorroConstants::kRightTap7 * scale * sizeScale;
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
    leftTankIn = fastTanh(leftTankIn);

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
    rightTankIn = fastTanh(rightTankIn);

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

// =============================================================================
// MAIN DSP CLASS - DATTORRO PLATE REVERB
// =============================================================================
template<typename T>
class PluginInstanceDSP
{
public:
  PluginInstanceDSP() = default;

  /**
   * Process a block of audio samples.
   *
   * SIGNAL FLOW:
   * 1. Input tone shaping (Low Cut removes bass, High Cut removes brightness)
   * 2. Pre-delay (space before reverb)
   * 3. Input diffusion (4 allpasses to smear transients)
   * 4. Dual-tank processing (spillover technique for smooth size changes)
   * 5. Output taps (multiple points for stereo decorrelation)
   * 6. Stereo width control (mid/side processing)
   * 7. Mix dry and wet
   *
   * SPILLOVER TECHNIQUE:
   * We run two complete tank systems. When size changes:
   * - The currently active tank becomes "spillover" (receives no new input, just decays)
   * - The other tank becomes active (receives new input with new size settings)
   * - Both outputs are summed - the old tail fades naturally via decay
   * This avoids pitch shift AND volume dips during size automation.
   */
  void ProcessBlock(T** inputs, T** outputs, int nInputs, int nOutputs, int nFrames)
  {
    if (!inputs || !outputs || nOutputs < 1 || nInputs < 1)
      return;

    T* inL = inputs[0];
    T* inR = (nInputs > 1 && inputs[1]) ? inputs[1] : inputs[0];

    float scale = mSampleRate / DattorroConstants::kReferenceSampleRate;

    // =========================================================================
    // BLOCK-LEVEL UPDATES
    // =========================================================================
    // SIZE CHANGE STRATEGY (Valhalla-style):
    // We allow the natural pitch shift (Doppler effect) from delay time changes -
    // this is the "analog" behavior of tape machines and Valhalla reverbs.
    // 15ms smoothing prevents clicks while keeping the musical pitch bend.
    // NO clearing needed - old tail decays naturally through feedback.

    for (int s = 0; s < nFrames; ++s)
    {
      // =======================================================================
      // GET SMOOTHED PARAMETER VALUES (per-sample for click-free automation)
      // =======================================================================
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

      // Update tank when size changes (no clearing - see block comment above)
      if (std::abs(size - mLastSize) > 0.0001f) {
        mTankA.updateDelayTimes(size, scale);
        mEarlyReflections.updateTapTimes(size);
        mLastSize = size;
      }

      // Update tank damping per-sample (smoothed via mDamping)
      // Damping is set internally by Color mode, not exposed as a parameter
      mTankA.setDamping(damping);

      // Update density when it changes (controls tank allpass diffusion AND ER smearing)
      if (std::abs(density - mLastDensity) > 0.0001f) {
        mTankA.setDensity(density);

        // SOFT-CLEAR ER DIFFUSERS ON DENSITY CHANGE:
        // Without this, old audio (processed with old feedback) lingers in the
        // diffuser buffers. When you increase density, you'd still hear the old
        // "bouncy" echoes mixed with new smooth ones - sounds wrong.
        // -20dB attenuation fades old audio quickly without audible clicks.
        // Same technique as Valhalla-style mode changes.
        float clearFactor = 0.1f;  // -20dB = 10^(-20/20) ≈ 0.1
        mERInputDiffuser1.softClear(clearFactor);
        mERInputDiffuser2.softClear(clearFactor);
        mERInputDiffuser3.softClear(clearFactor);
        mERInputDiffuser4.softClear(clearFactor);

        mLastDensity = density;
      }

      float inputL = static_cast<float>(inL[s]);
      float inputR = static_cast<float>(inR[s]);

      // =======================================================================
      // STEP 1: Create mono input
      // =======================================================================
      float monoIn = (inputL + inputR) * 0.5f;

      // =======================================================================
      // STEP 2: Input Tone Shaping (Low Cut + High Cut)
      // =======================================================================
      float filtered = mLowCut.process(monoIn);
      filtered = mHighCut.process(filtered);

      // =======================================================================
      // STEP 3: Pre-delay
      // =======================================================================
      float preDelayed = (mPreDelaySamples > 0)
        ? mPreDelay.readInt(mPreDelaySamples)
        : filtered;
      mPreDelay.write(filtered);

      // =======================================================================
      // STEP 4: MODULATION BANK (8 Decorrelated LFOs)
      // =======================================================================
      // Process modulation FIRST so we can use it for ALL diffusers.
      // This is CRITICAL for preventing metallic "hitting a pipe" artifacts!
      //
      // WHY MODULATE ALL DIFFUSERS:
      // Static allpass delays create resonant peaks at specific frequencies.
      // When impulsive sounds (drums, consonants) pass through, these frequencies
      // ring out = metallic sound. Adding even subtle modulation (±4-8 samples)
      // breaks up resonances without audible pitch wobble.
      // See: https://valhalladsp.com/2011/01/21/reverbs-diffusion-allpass-delays-and-metallic-artifacts/
      mModBank.process(modRate, mSampleRate);

      // TANK MODULATION (user-controlled via Mod Depth knob)
      // Professional reverbs use aggressive modulation for "lush" sound.
      // At 48kHz, 250 samples = ~5ms pitch variation = very audible chorus.
      // Mod Depth = 0 means NO modulation (clean reverb).
      float excursion = modDepth * 250.0f;  // 0 to 250 samples
      float modOffset1 = mModBank.get(2) * excursion;  // Left tank
      float modOffset3 = mModBank.get(4) * excursion;  // Right tank

      // OUTPUT TAP MODULATION (user-controlled via Mod Depth knob)
      // Makes modulation immediately audible on first reflection, not just tail
      // ±32 samples at 48kHz = ~0.7ms = obvious pitch wobble
      float outputModL = mModBank.get(0) * modDepth * 32.0f;
      float outputModR = mModBank.get(1) * modDepth * 32.0f;

      // ER DIFFUSER MODULATION (user-controlled via Mod Depth knob)
      // Only ER diffusers (5-21ms) are modulated, NOT input diffusers (3-8ms).
      //
      // Per Sean Costello (Valhalla DSP): modulating SHORT allpasses causes
      // "too audible of a chorusing sound, or a sound similar to water sloshing"
      // Only LONG allpasses benefit from modulation.
      constexpr float kERDiffuserModAmt = 5.0f;  // ±5 samples at full Mod Depth
      float erDiffMod1 = mModBank.get(6) * kERDiffuserModAmt * modDepth;
      float erDiffMod2 = mModBank.get(7) * kERDiffuserModAmt * modDepth;

      // =======================================================================
      // STEP 5: EARLY REFLECTIONS (Room Character)
      // =======================================================================
      // Process early reflections in parallel with late reverb.
      //
      // DENSITY-CONTROLLED ER INPUT DIFFUSION:
      // At low density: raw input -> distinct ER taps (you hear bouncing echoes)
      // At high density: diffused input -> blended ER taps (smooth onset)
      //
      // MODULATED DELAYS: Each allpass gets subtle delay modulation to prevent
      // metallic ringing. Pairs share LFOs but with opposite polarity.
      //
      // REDUCED FEEDBACK: Max 0.60 (was 0.78) to further minimize resonances.
      // Combined with modulation, this eliminates the "pipe" sound.
      float erInput = preDelayed;
      {
        // Scale feedback with density: 0% = no diffusion, 100% = max 0.60
        // REDUCED from 0.78 to 0.60 - modulation handles the smearing now
        float erDiffFeedback = density * 0.60f;

        // Decreasing feedback per stage prevents resonance buildup
        mERInputDiffuser1.setFeedback(erDiffFeedback);
        mERInputDiffuser2.setFeedback(erDiffFeedback * 0.92f);
        mERInputDiffuser3.setFeedback(erDiffFeedback * 0.84f);
        mERInputDiffuser4.setFeedback(erDiffFeedback * 0.76f);

        // Process with MODULATED delay times (subtle ±5 samples)
        // Pairs share LFOs but with opposite polarity for decorrelation
        int erMod1 = static_cast<int>(erDiffMod1);
        int erMod2 = static_cast<int>(erDiffMod2);
        erInput = mERInputDiffuser1.processInt(erInput, mERDiffuserDelay1 + erMod1);
        erInput = mERInputDiffuser2.processInt(erInput, mERDiffuserDelay2 - erMod1);
        erInput = mERInputDiffuser3.processInt(erInput, mERDiffuserDelay3 + erMod2);
        erInput = mERInputDiffuser4.processInt(erInput, mERDiffuserDelay4 - erMod2);
      }

      float earlyL = 0.0f, earlyR = 0.0f;
      mEarlyReflections.process(erInput, earlyL, earlyR);

      // =======================================================================
      // STEP 6: Input Diffusion (4 allpass filters in series)
      // =======================================================================
      // NO MODULATION on input diffusers - they have SHORT delays (3-8ms).
      // Per Sean Costello (Valhalla DSP): modulating short allpasses causes
      // "too audible of a chorusing sound, or a sound similar to water sloshing"
      float diffused = preDelayed;
      diffused = mInputAP1.processInt(diffused, mInputDiffusionDelay1);
      diffused = mInputAP2.processInt(diffused, mInputDiffusionDelay2);
      diffused = mInputAP3.processInt(diffused, mInputDiffusionDelay3);
      diffused = mInputAP4.processInt(diffused, mInputDiffusionDelay4);

      // =======================================================================
      // STEP 7: Tank Processing (Late Reverb)
      // =======================================================================
      float lateL = 0.0f, lateR = 0.0f;
      mTankA.process(diffused, decay, lateL, lateR, modOffset1, modOffset3, outputModL, outputModR);

      // =======================================================================
      // STEP 8: DC BLOCKING (Prevent DC buildup in feedback)
      // =======================================================================
      lateL = mDCBlockerL.process(lateL);
      lateR = mDCBlockerR.process(lateR);

      // Scale late reverb output to prevent clipping
      // The tank accumulates energy; 0.6 keeps peaks around -4dB headroom
      lateL *= 0.6f;
      lateR *= 0.6f;

      // =======================================================================
      // STEP 9: EARLY/LATE MIX (Listener Position)
      // =======================================================================
      // Early/Late controls the amount of early reflections, NOT a crossfade!
      // - 0% (Early): Full early reflections = close to source = punchy attack
      // - 100% (Late): No early reflections = far from source = slow attack
      // Late reverb is ALWAYS at full volume - it's the main reverb tail.
      float lateGain = 1.0f;  // Always full
      // Boost early reflections significantly (3x) so they're clearly audible
      float earlyGain = (1.0f - earlyLate) * 3.0f;

      float wetL = earlyL * earlyGain + lateL * lateGain;
      float wetR = earlyR * earlyGain + lateR * lateGain;

      // =======================================================================
      // STEP 10: Color Filtering (Output EQ - part of Color mode system)
      // =======================================================================
      // Color mode controls BOTH this output EQ AND the feedback damping above.
      // The damping (mDamping) was already applied in the tank via setDamping().
      // This step applies the OUTPUT filter part of the Color mode:
      // - Bright: bypass (full bandwidth)
      // - Neutral: 8kHz LPF (12dB/oct)
      // - Dark: 3kHz LPF (24dB/oct)
      // - Studio: 600Hz HPF + 6kHz LPF (24dB/oct)
      switch (mColorMode) {
        case kColorBright:
          // No filtering - full bandwidth, airy
          break;
        case kColorNeutral:
          // 8kHz lowpass (2-pole, 12dB/oct) - gentle rolloff
          wetL = mColorLPF_L.process(wetL);
          wetR = mColorLPF_R.process(wetR);
          break;
        case kColorDark:
          // 3kHz lowpass (4-pole, 24dB/oct) - steep, clearly dark
          // Run twice for steeper slope
          wetL = mColorLPF_L.process(wetL);
          wetL = mColorLPF_L.process(wetL);
          wetR = mColorLPF_R.process(wetR);
          wetR = mColorLPF_R.process(wetR);
          break;
        case kColorStudio:
          // 600Hz HPF + 6kHz LPF (both 4-pole) - tight bandpass
          // Run twice for steeper slopes
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

      // =======================================================================
      // STEP 11: Stereo Width Control (Mid/Side Processing)
      // =======================================================================
      {
        float mid = (wetL + wetR) * 0.5f;
        float side = (wetL - wetR) * 0.5f;
        wetL = mid + side * width;
        wetR = mid - side * width;
      }

      // =======================================================================
      // STEP 12: Mix Dry and Wet
      // =======================================================================
      float outL = inputL * dry + wetL * wet;
      float outR = inputR * dry + wetR * wet;

      // Safety limiting
      outL = fastTanh(outL);
      outR = fastTanh(outR);

      outputs[0][s] = static_cast<T>(outL);
      if (nOutputs > 1)
        outputs[1][s] = static_cast<T>(outR);
    }
  }

  /**
   * Reset DSP state. Called when sample rate changes or plugin is reset.
   */
  void Reset(double sampleRate, int /*blockSize*/)
  {
    mSampleRate = static_cast<float>(sampleRate);

    // Calculate sample rate scaling factor
    // Dattorro's times were specified at 29761 Hz
    float scale = mSampleRate / DattorroConstants::kReferenceSampleRate;

    // ===========================================================================
    // INITIALIZE SMOOTHED PARAMETERS
    // ===========================================================================
    // Set smoothing times (in ms)
    constexpr float kFastSmooth = 5.0f;   // 5ms for responsive params
    constexpr float kMediumSmooth = 10.0f; // 10ms for most params

    // SIZE SMOOTHING (Valhalla-style "analog manner"):
    // Instead of muting reverb during size changes, we allow natural pitch shift
    // (Doppler effect) - sounds like tape speed change, musically pleasing.
    //
    // WHY 15ms:
    // - Below 10ms: Pitch shift too fast/harsh, sounds like a glitch
    // - Above 20ms: Pitch shift too slow, feels laggy/unresponsive
    // - 15ms: Sweet spot - audible pitch bend but smooth and musical
    constexpr float kSizeSmooth = 15.0f;

    mDry.setTime(kFastSmooth, mSampleRate);
    mWet.setTime(kFastSmooth, mSampleRate);
    mDecay.setTime(kMediumSmooth, mSampleRate);
    mSize.setTime(kSizeSmooth, mSampleRate);
    mWidth.setTime(kMediumSmooth, mSampleRate);
    mDamping.setTime(kMediumSmooth, mSampleRate);  // Internal, set by Color mode
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
    // Damping default: Neutral mode = 0.35 (see Color mode documentation)
    mDamping.setTarget(0.35f); mDamping.snap();
    mDensity.setTarget(0.7f); mDensity.snap();
    mModRate.setTarget(0.5f); mModRate.snap();
    mModDepth.setTarget(0.5f); mModDepth.snap();
    mEarlyLate.setTarget(0.5f); mEarlyLate.snap();

    mLastSize = 0.7f;
    mLastDensity = 0.7f;

    // ===========================================================================
    // CLEAR INPUT PROCESSING
    // ===========================================================================
    mPreDelay.clear();
    mLowCut.clear();
    mHighCut.clear();
    mInputAP1.clear();
    mInputAP2.clear();
    mInputAP3.clear();
    mInputAP4.clear();

    // ===========================================================================
    // INITIALIZE EARLY REFLECTIONS
    // ===========================================================================
    mEarlyReflections.reset(mSampleRate);

    // ===========================================================================
    // INITIALIZE ER INPUT DIFFUSERS
    // ===========================================================================
    // These smear the input BEFORE it hits the ER tap network.
    // Controlled by Density parameter - at 100% density, these provide heavy
    // diffusion that smooths out Cathedral's long ER taps (no more bouncing).
    //
    // Fibonacci-like delays (5, 8, 13, 21ms) are roughly coprime, avoiding
    // resonant buildup that occurs with evenly-spaced delays.
    mERInputDiffuser1.clear();
    mERInputDiffuser2.clear();
    mERInputDiffuser3.clear();
    mERInputDiffuser4.clear();
    mERDiffuserDelay1 = static_cast<int>(5.0f * 0.001f * mSampleRate);   // ~5ms
    mERDiffuserDelay2 = static_cast<int>(8.0f * 0.001f * mSampleRate);   // ~8ms
    mERDiffuserDelay3 = static_cast<int>(13.0f * 0.001f * mSampleRate);  // ~13ms
    mERDiffuserDelay4 = static_cast<int>(21.0f * 0.001f * mSampleRate);  // ~21ms

    // ===========================================================================
    // CLEAR AND CONFIGURE TANK
    // ===========================================================================
    mTankA.clear();
    mTankA.setDiffusionCoeffs();
    mTankA.setDamping(mDamping.current);
    mTankA.updateDelayTimes(mSize.current, scale);

    // ===========================================================================
    // INITIALIZE DC BLOCKERS
    // ===========================================================================
    mDCBlockerL.clear();
    mDCBlockerR.clear();
    mDCBlockerL.setCutoff(5.0f, mSampleRate);  // 5Hz cutoff
    mDCBlockerR.setCutoff(5.0f, mSampleRate);

    // ===========================================================================
    // INITIALIZE COLOR OUTPUT FILTERS
    // ===========================================================================
    // Color filters are applied after the reverb tank for tonal character.
    // Default to Neutral mode (8kHz lowpass 12dB/oct - takes off harshness).
    mColorMode = kColorNeutral;
    mColorLPF_L.clear();
    mColorLPF_R.clear();
    mColorHPF_L.clear();
    mColorHPF_R.clear();
    mColorLPF_L.setCutoff(8000.0f, mSampleRate);  // Neutral mode default
    mColorLPF_R.setCutoff(8000.0f, mSampleRate);
    mColorHPF_L.setCutoff(600.0f, mSampleRate);  // Pre-configured for Studio mode
    mColorHPF_R.setCutoff(600.0f, mSampleRate);

    // Set scaled input diffusion delay times
    mInputDiffusionDelay1 = static_cast<int>(DattorroConstants::kInputDiffusion1 * scale);
    mInputDiffusionDelay2 = static_cast<int>(DattorroConstants::kInputDiffusion2 * scale);
    mInputDiffusionDelay3 = static_cast<int>(DattorroConstants::kInputDiffusion3 * scale);
    mInputDiffusionDelay4 = static_cast<int>(DattorroConstants::kInputDiffusion4 * scale);

    // ===========================================================================
    // INITIALIZE REVERB MODE
    // ===========================================================================
    // Default to Plate mode - sets input diffusion coefficients and ER pattern.
    // Mode is controlled by user param, diffusion is internal (not user-exposed).
    mReverbMode = kModePlate;
    mEarlyReflections.setMode(mReverbMode);
    // Plate: Medium-high diffusion - REDUCED from 0.80/0.70 to prevent metallic ringing
    // Modulation now handles smearing, so we can use lower feedback safely
    mInputAP1.setFeedback(0.65f);
    mInputAP2.setFeedback(0.65f);
    mInputAP3.setFeedback(0.55f);
    mInputAP4.setFeedback(0.55f);

    // Set default input filter cutoffs
    mLowCut.setCutoff(80.0f, mSampleRate);
    mHighCut.setCutoff(8000.0f, mSampleRate);

    // ===========================================================================
    // INITIALIZE MODULATION BANK
    // ===========================================================================
    mModBank.reset();
  }

  /**
   * Handle parameter changes from the UI.
   * All parameters use SmoothedValue for click-free automation.
   */
  void SetParam(int paramIdx, double value)
  {
    switch (paramIdx)
    {
      case kParamDry:
        mDry.setTarget(static_cast<float>(value / 100.0));
        break;

      case kParamWet:
        mWet.setTarget(static_cast<float>(value / 100.0));
        break;

      case kParamDecay:
        // Decay controls feedback amount
        // Higher decay = longer reverb tail
        // IMPORTANT: Max decay of 0.85 prevents runaway feedback
        mDecay.setTarget(static_cast<float>(value / 100.0) * 0.85f);
        break;

      case kParamSize:
        // Size scales all tank delay times
        // Larger size = longer delays = bigger sounding space
        mSize.setTarget(static_cast<float>(value / 100.0));
        break;

      case kParamPreDelay:
        // Pre-delay in milliseconds
        mPreDelaySamples = static_cast<int>((value / 1000.0) * mSampleRate);
        mPreDelaySamples = std::min(mPreDelaySamples, DattorroConstants::kMaxPreDelay - 1);
        break;

      case kParamMode:
        // Mode selects reverb type - each mode sets internal diffusion and ER patterns
        // Plate: instant attack, high diffusion, minimal ERs
        // Chamber: fast attack, medium-high diffusion, dense ERs
        // Cathedral: very slow attack, low diffusion, sparse long ERs
        //
        // VALHALLA-STYLE MODE CHANGE:
        // When mode changes, we soft-clear the tank so the old tail fades quickly
        // and the new character builds fresh. This prevents the old tail (built with
        // old diffusion settings) from playing through the new algorithm weirdly.
        {
          int newMode = static_cast<int>(value);

          // Only clear if mode actually changed
          if (newMode != mReverbMode) {
            // Soft-clear tank and input diffusers (-40dB attenuation)
            // This fades the old tail quickly without a hard click
            mTankA.softClear(0.01f);
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
            case kModePlate:
              // Plate: Medium-high diffusion - REDUCED to prevent metallic ringing
              // Modulation now handles smearing, so we can use lower feedback safely
              mInputAP1.setFeedback(0.65f);
              mInputAP2.setFeedback(0.65f);
              mInputAP3.setFeedback(0.55f);
              mInputAP4.setFeedback(0.55f);
              break;
            case kModeChamber:
              // Chamber: Medium diffusion - warm onset, less metallic than before
              mInputAP1.setFeedback(0.55f);
              mInputAP2.setFeedback(0.55f);
              mInputAP3.setFeedback(0.48f);
              mInputAP4.setFeedback(0.48f);
              break;
            case kModeHall:
              // Hall: Medium diffusion - balanced attack, wide and smooth
              mInputAP1.setFeedback(0.48f);
              mInputAP2.setFeedback(0.48f);
              mInputAP3.setFeedback(0.42f);
              mInputAP4.setFeedback(0.42f);
              break;
            case kModeCathedral:
              // Cathedral: Low diffusion - transients pass through, then smear
              // Creates "ethereal" buildup: attack → echoes → dense tail
              mInputAP1.setFeedback(0.42f);
              mInputAP2.setFeedback(0.42f);
              mInputAP3.setFeedback(0.36f);
              mInputAP4.setFeedback(0.36f);
              break;
          }
        }
        break;

      case kParamDensity:
        // Density controls the tank allpass diffusion (tail texture)
        // Low density = grainy, you hear individual reflections
        // High density = smooth, continuous wash
        mDensity.setTarget(static_cast<float>(value / 100.0));
        break;

      case kParamLowCut:
        // Low Cut (high-pass filter) - removes bass/mud from reverb input
        mLowCut.setCutoff(static_cast<float>(value), mSampleRate);
        break;

      case kParamHighCut:
        // High Cut (low-pass filter) - removes brightness from reverb input
        mHighCut.setCutoff(static_cast<float>(value), mSampleRate);
        break;

      case kParamColor:
        // =======================================================================
        // COLOR MODE - Combined Output Filter + Feedback Damping (FutureVerb-style)
        // =======================================================================
        // Color controls BOTH the output EQ AND the feedback loop damping.
        // This is how FutureVerb works - one semantic "tonal character" control
        // instead of separate technical knobs.
        //
        // WHY COMBINE THEM:
        // - Output filter: Instant effect on brightness (post-reverb EQ)
        // - Damping: Evolving effect - highs decay faster in feedback loop
        // - Together they create cohesive character that users understand intuitively
        //
        // DAMPING VALUES (0.0 = no damping, 1.0 = full damping):
        // Values spread wide for OBVIOUS differences between modes:
        // - Bright (0.10): Minimal HF absorption, highs sustain, shimmering plate-like
        // - Neutral (0.35): Natural acoustic absorption, balanced decay
        // - Dark (0.75): Aggressive absorption, highs die fast, vintage warm tail
        // - Studio (0.50): Moderate control, sits in mix without buildup
        //
        // WHY SNAP (not smooth):
        // Mode changes are discrete selections - user expects immediate character change.
        // Smoothing would cause weird in-between states during transition.
        // =======================================================================
        mColorMode = static_cast<int>(value);
        switch (mColorMode) {
          case kColorBright:
            // No output filtering - full bandwidth, airy
            // Minimal damping (0.10) - highs sustain, shimmering tail
            mDamping.setTarget(0.10f);
            mDamping.snap();  // Instant change on mode switch
            break;
          case kColorNeutral:
            // 8kHz lowpass (12dB/oct) - gentle rolloff, still open
            // Medium damping (0.35) - natural acoustic decay
            mColorLPF_L.setCutoff(8000.0f, mSampleRate);
            mColorLPF_R.setCutoff(8000.0f, mSampleRate);
            mDamping.setTarget(0.35f);
            mDamping.snap();  // Instant change on mode switch
            break;
          case kColorDark:
            // 3kHz lowpass (24dB/oct via cascade) - steep, clearly dark
            // Aggressive damping (0.75) - highs decay fast, vintage character
            mColorLPF_L.setCutoff(3000.0f, mSampleRate);
            mColorLPF_R.setCutoff(3000.0f, mSampleRate);
            mDamping.setTarget(0.75f);
            mDamping.snap();  // Instant change on mode switch
            break;
          case kColorStudio:
            // 600Hz HPF + 6kHz LPF (both 24dB/oct) - tight bandpass
            // Moderate damping (0.50) - controlled, mix-friendly
            mColorHPF_L.setCutoff(600.0f, mSampleRate);
            mColorHPF_R.setCutoff(600.0f, mSampleRate);
            mColorLPF_L.setCutoff(6000.0f, mSampleRate);
            mColorLPF_R.setCutoff(6000.0f, mSampleRate);
            mDamping.setTarget(0.50f);
            mDamping.snap();  // Instant change on mode switch
            break;
        }
        break;

      case kParamWidth:
        // Stereo width control using mid/side processing
        // 0% = mono, 100% = normal stereo
        mWidth.setTarget(static_cast<float>(value / 100.0));
        break;

      case kParamModRate:
        // Modulation rate controls how fast the LFOs oscillate
        // 0.1 Hz = very slow shimmer, 2.0 Hz = fast chorus-like wobble
        mModRate.setTarget(static_cast<float>(value));
        break;

      case kParamModDepth:
        // Modulation depth controls the intensity of the pitch variation
        // 0% = no modulation (static reverb)
        // 100% = obvious chorus effect in the tail
        mModDepth.setTarget(static_cast<float>(value / 100.0));
        break;

      case kParamEarlyLate:
        // Early/Late - Listener position control
        // Controls the amount of early reflections (late reverb is always full)
        // 0% = Close to source - full early reflections, punchy attack
        // 100% = Far from source - no early reflections, slow diffuse attack
        mEarlyLate.setTarget(static_cast<float>(value / 100.0));
        break;

      default:
        break;
    }
  }

private:
  // ===========================================================================
  // MEMBER VARIABLES
  // ===========================================================================

  // Sample rate
  float mSampleRate = 44100.0f;

  // ===========================================================================
  // SMOOTHED PARAMETERS - All parameters use SmoothedValue for click-free automation
  // ===========================================================================
  SmoothedValue<float> mDry;           // Dry signal level (0-1)
  SmoothedValue<float> mWet;           // Wet signal level (0-1)
  SmoothedValue<float> mDecay;         // Feedback amount (0-0.85)
  SmoothedValue<float> mSize;          // Room size (0-1)
  SmoothedValue<float> mWidth;         // Stereo width (0-2)
  SmoothedValue<float> mDamping;       // HF damping (0-1) - INTERNAL, set by Color mode
  SmoothedValue<float> mModRate;       // LFO rate in Hz
  SmoothedValue<float> mModDepth;      // Modulation intensity (0-1)
  SmoothedValue<float> mEarlyLate;     // Early/Late balance (0=late only, 1=early only)
  SmoothedValue<float> mDensity;       // Tank diffusion - tail texture (0-1)

  int mPreDelaySamples = 0;

  // Parameter change tracking (for per-sample updates)
  float mLastSize = 0.5f;     // Track size changes
  float mLastDensity = 0.7f;  // Track density changes

  // ===========================================================================
  // REVERB MODE - Controls diffusion and ER character
  // ===========================================================================
  // Mode determines the overall character: Plate (instant/bright) vs Chamber (fast/warm)
  // Diffusion is controlled internally by mode, not exposed to user.
  int mReverbMode = 0;  // kModePlate (defined in Plugin.h)

  // ===========================================================================
  // INPUT PROCESSING
  // ===========================================================================
  HighPassFilter mLowCut;     // Removes bass/mud (High-pass = "Low Cut")
  LowPassFilter mHighCut;     // Removes highs/brightness (Low-pass = "High Cut")
  DelayLine<16384> mPreDelay; // Up to ~340ms at 48kHz

  // ===========================================================================
  // EARLY REFLECTIONS - Room character
  // ===========================================================================
  EarlyReflections mEarlyReflections;

  // ===========================================================================
  // ER INPUT DIFFUSION - Density-controlled smearing BEFORE ER taps
  // ===========================================================================
  // At low density: raw input -> distinct ER taps (you hear individual echoes)
  // At high density: diffused input -> blended ER taps (smooth onset)
  //
  // KEY INSIGHT FROM VALHALLA DSP RESEARCH:
  // - Allpasses at INPUT expand impulses into many echoes (correct)
  // - Allpasses at OUTPUT cause metallic coloration (wrong!)
  // This is because allpasses are "impulse expanders" - they work by smearing
  // discrete impulses. On output, this creates ringing on already-complex signal.
  //
  // DELAY TIME DESIGN:
  // Fibonacci-like spacing (5, 8, 13, 21ms) ensures delays are roughly coprime,
  // preventing resonant peaks that occur with evenly-spaced delays.
  // Total diffusion time: ~47ms - enough to smear Cathedral's long ER taps.
  //
  // WHY 4 DIFFUSERS:
  // Each allpass roughly doubles echo density. 4 stages = 16× density increase.
  // This matches professional reverbs like Valhalla which use 4-8 input diffusers.
  AllpassFilter<2048> mERInputDiffuser1;
  AllpassFilter<2048> mERInputDiffuser2;
  AllpassFilter<2048> mERInputDiffuser3;
  AllpassFilter<2048> mERInputDiffuser4;
  int mERDiffuserDelay1 = 221;   // ~5ms at 44.1kHz (prime)
  int mERDiffuserDelay2 = 353;   // ~8ms at 44.1kHz (prime)
  int mERDiffuserDelay3 = 577;   // ~13ms at 44.1kHz (prime)
  int mERDiffuserDelay4 = 929;   // ~21ms at 44.1kHz (prime)

  // ===========================================================================
  // INPUT DIFFUSION - Smear input before tank
  // ===========================================================================
  AllpassFilter<2048> mInputAP1;
  AllpassFilter<2048> mInputAP2;
  AllpassFilter<4096> mInputAP3;
  AllpassFilter<4096> mInputAP4;

  int mInputDiffusionDelay1 = 142;
  int mInputDiffusionDelay2 = 107;
  int mInputDiffusionDelay3 = 379;
  int mInputDiffusionDelay4 = 277;

  // ===========================================================================
  // TANK SYSTEM
  // ===========================================================================
  TankSystem mTankA;

  // ===========================================================================
  // DC BLOCKERS - Prevent DC buildup in feedback loops
  // ===========================================================================
  DCBlocker mDCBlockerL;
  DCBlocker mDCBlockerR;

  // ===========================================================================
  // MODULATION BANK - 8 Decorrelated LFOs for professional sound
  // ===========================================================================
  // Replaces the old 2-LFO system with 8 decorrelated modulators.
  // Each delay line gets its own LFO for complex, organic movement.
  ModulationBank mModBank;

  // ===========================================================================
  // COLOR SYSTEM - Combined Output Filtering + Feedback Damping
  // ===========================================================================
  // Color mode controls BOTH output EQ AND feedback damping (FutureVerb-style).
  // This provides one intuitive "tonal character" control instead of two technical knobs.
  //
  // OUTPUT FILTERS (applied after reverb):
  // - Bright: Bypass (full bandwidth)
  // - Neutral: 8kHz LPF (12dB/oct) - gentle rolloff
  // - Dark: 3kHz LPF (24dB/oct) - steep, clearly dark
  // - Studio: 600Hz HPF + 6kHz LPF (24dB/oct) - tight bandpass
  //
  // FEEDBACK DAMPING (snaps instantly on mode change):
  // - Bright: 0.10 (minimal - highs sustain, shimmering)
  // - Neutral: 0.35 (medium - natural acoustic decay)
  // - Dark: 0.75 (aggressive - highs die fast, vintage)
  // - Studio: 0.50 (moderate - controlled, mix-friendly)
  int mColorMode = 1;  // kColorNeutral (defined in Plugin.h)
  LowPassFilter mColorLPF_L;   // Color lowpass filter (left)
  LowPassFilter mColorLPF_R;   // Color lowpass filter (right)
  HighPassFilter mColorHPF_L;  // Color highpass filter (left) - Studio mode only
  HighPassFilter mColorHPF_R;  // Color highpass filter (right) - Studio mode only
};
