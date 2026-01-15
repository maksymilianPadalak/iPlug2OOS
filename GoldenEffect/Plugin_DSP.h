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
 * - LFO 0-1: Input diffusion allpasses (subtle, ±2 samples)
 * - LFO 2-3: Left tank allpass + delay (medium, ±8 samples)
 * - LFO 4-5: Right tank allpass + delay (medium, ±8 samples)
 * - LFO 6-7: Secondary tank delays (subtle, ±4 samples)
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
 * Simple 1-pole lowpass filter for damping.
 *
 * WHY DAMPING:
 * Real acoustic spaces absorb high frequencies faster than low frequencies.
 * This filter in the feedback path makes the reverb tail progressively
 * darker over time - like a real room.
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

  // Tap times in milliseconds (at size=1.0)
  // Logarithmically spaced to match real room behavior
  static constexpr float kBaseTapTimes[NUM_TAPS] = {
    3.1f, 5.2f, 7.8f, 11.3f, 15.7f, 19.2f,
    24.1f, 31.5f, 38.2f, 47.6f, 58.3f, 72.1f
  };

  // Tap gains (decaying with time, some negative for phase variation)
  // Negative gains reduce comb filtering artifacts
  static constexpr float kTapGains[NUM_TAPS] = {
    0.85f, -0.72f, 0.68f, -0.55f, 0.48f, 0.42f,
    -0.35f, 0.30f, -0.25f, 0.20f, 0.16f, -0.12f
  };

  // Tap pans (-1 = full left, +1 = full right)
  // Spread across stereo field for natural imaging
  static constexpr float kTapPans[NUM_TAPS] = {
    -0.3f, 0.5f, -0.7f, 0.2f, -0.4f, 0.8f,
    -0.6f, 0.1f, 0.9f, -0.8f, 0.4f, -0.2f
  };

  std::array<int, NUM_TAPS> tapSamples{};  // Tap times in samples
  float mSampleRate = 44100.0f;

  void clear()
  {
    delay.clear();
    std::fill(tapSamples.begin(), tapSamples.end(), 0);
  }

  void reset(float sampleRate)
  {
    mSampleRate = sampleRate;
    delay.clear();
    updateTapTimes(0.5f);  // Default size
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
      float timeMs = kBaseTapTimes[i] * sizeScale;
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
      float tap = delay.readInt(tapSamples[i]) * kTapGains[i];
      float pan = kTapPans[i];

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
    // Update tank parameters that don't need per-sample smoothing
    // Size is smoothed per-sample, but we update tank once per block

    // Check if size changed significantly (for freeze-and-blend)
    float currentSize = mSize.current;
    float sizeGap = std::abs(mSize.target - currentSize);
    constexpr float kGapThreshold = 0.001f;
    constexpr float kFadeOutSpeed = 0.1f;
    constexpr float kFadeInSpeed = 0.06f;
    constexpr int kSettleBlocks = 15;

    if (sizeGap > kGapThreshold) {
      mWetGain = std::max(0.0f, mWetGain - kFadeOutSpeed);
      mSettleCounter = kSettleBlocks;
    } else if (mSettleCounter > 0) {
      mSettleCounter--;
    } else {
      mWetGain = std::min(1.0f, mWetGain + kFadeInSpeed);
    }

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

      // Update tank when size changes
      if (std::abs(size - mLastSize) > 0.0001f) {
        mTankA.updateDelayTimes(size, scale);
        mEarlyReflections.updateTapTimes(size);
        mLastSize = size;
      }

      // Update damping when it changes (independent of size)
      if (std::abs(damping - mLastDamping) > 0.0001f) {
        mTankA.setDamping(damping);
        mLastDamping = damping;
      }

      // Update density when it changes (controls tank allpass diffusion)
      if (std::abs(density - mLastDensity) > 0.0001f) {
        mTankA.setDensity(density);
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
      // STEP 4: EARLY REFLECTIONS (Room Character)
      // =======================================================================
      // Process early reflections in parallel with late reverb
      float earlyL = 0.0f, earlyR = 0.0f;
      mEarlyReflections.process(preDelayed, earlyL, earlyR);

      // =======================================================================
      // STEP 5: Input Diffusion (4 allpass filters in series)
      // =======================================================================
      float diffused = preDelayed;
      diffused = mInputAP1.processInt(diffused, mInputDiffusionDelay1);
      diffused = mInputAP2.processInt(diffused, mInputDiffusionDelay2);
      diffused = mInputAP3.processInt(diffused, mInputDiffusionDelay3);
      diffused = mInputAP4.processInt(diffused, mInputDiffusionDelay4);

      // =======================================================================
      // STEP 6: MODULATION BANK (8 Decorrelated LFOs)
      // =======================================================================
      // Process all 8 LFOs at once
      mModBank.process(modRate, mSampleRate);

      // Get modulation offsets for different delay lines
      // Each delay line gets its own LFO for complex, organic movement
      //
      // EXCURSION CALCULATION:
      // Professional reverbs use aggressive modulation for "lush" sound.
      // At 48kHz, 200 samples = ~4ms pitch variation = very audible chorus.
      float excursion = 50.0f + modDepth * 200.0f;  // 50 to 250 samples

      // LFO assignment (see ModulationBank for decorrelation details):
      // - LFO 0-1: Output tap modulation (for immediate stereo shimmer)
      // - LFO 2: Left tank allpass modulation
      // - LFO 4: Right tank allpass modulation
      float modOffset1 = mModBank.get(2) * excursion;
      float modOffset3 = mModBank.get(4) * excursion;

      // Output tap modulation - makes modulation immediately audible
      // This is the key to hearing chorus on first reflection, not just tail
      // ±32 samples at 48kHz = ~0.7ms = obvious pitch wobble
      float outputModL = mModBank.get(0) * modDepth * 32.0f;
      float outputModR = mModBank.get(1) * modDepth * 32.0f;

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

      float wetL = (earlyL * earlyGain + lateL * lateGain) * mWetGain;
      float wetR = (earlyR * earlyGain + lateR * lateGain) * mWetGain;

      // =======================================================================
      // STEP 10: Color Filtering (Output Tonal Character)
      // =======================================================================
      // Color modes apply output filtering AFTER the reverb for tonal character.
      // Bright = bypass, Neutral = 8kHz LPF (12dB/oct), Dark = 3kHz LPF (24dB/oct),
      // Studio = 600Hz HPF + 6kHz LPF (24dB/oct) - tight bandpass for mix clarity
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
    // Set smoothing times (in ms) - faster for modulation, slower for filters
    constexpr float kFastSmooth = 5.0f;   // 5ms for responsive params
    constexpr float kMediumSmooth = 10.0f; // 10ms for most params
    constexpr float kSlowSmooth = 20.0f;  // 20ms for size (avoid pitch artifacts)

    mDry.setTime(kFastSmooth, mSampleRate);
    mWet.setTime(kFastSmooth, mSampleRate);
    mDecay.setTime(kMediumSmooth, mSampleRate);
    mSize.setTime(kSlowSmooth, mSampleRate);
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
    mDamping.setTarget(0.5f); mDamping.snap();
    mDensity.setTarget(0.7f); mDensity.snap();
    mModRate.setTarget(0.5f); mModRate.snap();
    mModDepth.setTarget(0.5f); mModDepth.snap();
    mEarlyLate.setTarget(0.5f); mEarlyLate.snap();

    mLastSize = 0.7f;
    mLastDamping = 0.5f;
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

    // Set diffusion coefficients (from Dattorro's paper)
    mInputAP1.setFeedback(DattorroConstants::kInputDiffusionCoeff1);
    mInputAP2.setFeedback(DattorroConstants::kInputDiffusionCoeff1);
    mInputAP3.setFeedback(DattorroConstants::kInputDiffusionCoeff2);
    mInputAP4.setFeedback(DattorroConstants::kInputDiffusionCoeff2);

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

      case kParamDamping:
        // Damping controls high frequency absorption in the feedback loop
        // Higher damping = darker reverb tail (highs decay faster)
        mDamping.setTarget(static_cast<float>(value / 100.0));
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

      case kParamDiffusion:
        // Diffusion controls how much the input is "smeared" before entering the tank
        // Low diffusion = you hear distinct echoes/reflections (attack character)
        // High diffusion = smooth, washed-out, blended sound
        {
          float diff = static_cast<float>(value / 100.0);
          // Wide range for audible effect: 0.1 to 0.9
          float coeff1 = 0.1f + diff * 0.8f;   // 0.1 to 0.9
          float coeff2 = 0.1f + diff * 0.7f;   // 0.1 to 0.8
          mInputAP1.setFeedback(coeff1);
          mInputAP2.setFeedback(coeff1);
          mInputAP3.setFeedback(coeff2);
          mInputAP4.setFeedback(coeff2);
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
        // Color mode - output filtering for tonal character
        // Steep filters (24dB/oct via cascading) for clearly audible differences
        mColorMode = static_cast<int>(value);
        switch (mColorMode) {
          case kColorBright:
            // No filtering - full bandwidth, airy
            break;
          case kColorNeutral:
            // 8kHz lowpass (12dB/oct) - gentle rolloff, still open
            mColorLPF_L.setCutoff(8000.0f, mSampleRate);
            mColorLPF_R.setCutoff(8000.0f, mSampleRate);
            break;
          case kColorDark:
            // 3kHz lowpass (24dB/oct via cascade) - steep, clearly dark
            mColorLPF_L.setCutoff(3000.0f, mSampleRate);
            mColorLPF_R.setCutoff(3000.0f, mSampleRate);
            break;
          case kColorStudio:
            // 600Hz HPF + 6kHz LPF (both 24dB/oct) - tight bandpass
            mColorHPF_L.setCutoff(600.0f, mSampleRate);
            mColorHPF_R.setCutoff(600.0f, mSampleRate);
            mColorLPF_L.setCutoff(6000.0f, mSampleRate);
            mColorLPF_R.setCutoff(6000.0f, mSampleRate);
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
  SmoothedValue<float> mDamping;       // High-frequency damping (0-1)
  SmoothedValue<float> mModRate;       // LFO rate in Hz
  SmoothedValue<float> mModDepth;      // Modulation intensity (0-1)
  SmoothedValue<float> mEarlyLate;     // Early/Late balance (0=late only, 1=early only)
  SmoothedValue<float> mDensity;       // Tank diffusion - tail texture (0-1)

  int mPreDelaySamples = 0;

  // Freeze-and-blend state (for artifact-free size changes)
  // See ProcessBlock comments for detailed explanation of this technique.
  float mWetGain = 1.0f;      // 0.0 = wet muted, 1.0 = wet at full level
  int mSettleCounter = 0;     // Countdown: blocks to wait after size stabilizes
  float mLastSize = 0.5f;     // Track size changes for freeze-and-blend
  float mLastDamping = 0.5f;  // Track damping changes
  float mLastDensity = 0.7f;  // Track density changes

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
  // COLOR OUTPUT FILTERS - Tonal character applied after reverb
  // ===========================================================================
  // Color modes apply output filtering AFTER the tank for tonal character.
  // This doesn't affect the reverb decay - just the final output EQ.
  // Bright = bypass, Neutral = 8kHz LPF (12dB/oct), Dark = 3kHz LPF (24dB/oct),
  // Studio = 600Hz HPF + 6kHz LPF (24dB/oct) - tight bandpass for mix clarity
  int mColorMode = kColorNeutral;
  LowPassFilter mColorLPF_L;   // Color lowpass filter (left)
  LowPassFilter mColorLPF_R;   // Color lowpass filter (right)
  HighPassFilter mColorHPF_L;  // Color highpass filter (left) - Studio mode only
  HighPassFilter mColorHPF_R;  // Color highpass filter (right) - Studio mode only
};
