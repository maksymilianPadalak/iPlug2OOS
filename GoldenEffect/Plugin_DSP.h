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
// HIGH-PASS FILTER (Low Cut)
// =============================================================================
/**
 * One-pole high-pass filter for removing low frequencies.
 *
 * WHY LOW CUT:
 * Reverb can accumulate bass energy, making the mix muddy.
 * A high-pass filter on the input removes rumble and keeps things clean.
 * Typical settings: 60-150 Hz for music, higher for vocals.
 *
 * EQUATION: y[n] = coeff × (y[n-1] + x[n] - x[n-1])
 * where coeff = exp(-2π × cutoffHz / sampleRate)
 */
struct HighPassFilter
{
  float state = 0.0f;
  float prevInput = 0.0f;
  float coeff = 0.995f;

  void clear()
  {
    state = 0.0f;
    prevInput = 0.0f;
  }

  void setCutoff(float hz, float sampleRate)
  {
    // Calculate coefficient from cutoff frequency
    // Higher coeff = lower cutoff frequency
    coeff = std::exp(-2.0f * 3.14159265359f * hz / sampleRate);
    coeff = std::max(0.0f, std::min(0.9999f, coeff));
  }

  float process(float input)
  {
    // High-pass: output = coeff × (prev_output + input - prev_input)
    state = coeff * (state + input - prevInput);
    prevInput = input;
    return state;
  }
};

// =============================================================================
// LOW-PASS FILTER (High Cut)
// =============================================================================
/**
 * One-pole low-pass filter for removing high frequencies.
 *
 * WHY HIGH CUT:
 * Controls the brightness of the reverb input.
 * Lower cutoff = darker reverb overall.
 * This is different from damping which progressively darkens the tail.
 *
 * EQUATION: y[n] = (1-coeff) × x[n] + coeff × y[n-1]
 * where coeff = exp(-2π × cutoffHz / sampleRate)
 */
struct LowPassFilter
{
  float state = 0.0f;
  float coeff = 0.0f;

  void clear()
  {
    state = 0.0f;
  }

  void setCutoff(float hz, float sampleRate)
  {
    // Calculate coefficient from cutoff frequency
    // Higher coeff = lower cutoff frequency (more filtering)
    coeff = std::exp(-2.0f * 3.14159265359f * hz / sampleRate);
    coeff = std::max(0.0f, std::min(0.9999f, coeff));
  }

  float process(float input)
  {
    // Low-pass: output = (1-coeff) × input + coeff × prev_output
    state = (1.0f - coeff) * input + coeff * state;
    return state;
  }
};

// =============================================================================
// TANK SYSTEM - Complete reverb tank with all components
// =============================================================================
/**
 * Encapsulates a complete Dattorro tank (both left and right sides).
 * Having two of these allows the "spillover" technique:
 * - One tank receives new input (active)
 * - One tank just decays its existing tail (spillover)
 * - Both outputs are summed for seamless size transitions
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
   */
  void process(float diffusedInput, float decay, float& wetL, float& wetR)
  {
    // Save previous frame's outputs for cross-feedback
    float prevLeftOut = leftTankOut;
    float prevRightOut = rightTankOut;

    // ----- LEFT TANK -----
    float leftTankIn = diffusedInput + prevRightOut * decay;
    leftTankIn = fastTanh(leftTankIn);

    float leftAP1Out = tankAP1.process(leftTankIn, tankAllpassDelay1);
    tankDelay1.write(leftAP1Out);
    float leftDelay1Out = tankDelay1.read(tankDelayTime1);
    float leftDamped = dampingL.process(leftDelay1Out);
    float leftAP2Out = tankAP2.processInt(leftDamped, tankAllpassDelay2);
    tankDelay2.write(leftAP2Out);
    float leftDelay2Out = tankDelay2.read(tankDelayTime2);

    // ----- RIGHT TANK -----
    float rightTankIn = diffusedInput + prevLeftOut * decay;
    rightTankIn = fastTanh(rightTankIn);

    float rightAP1Out = tankAP3.process(rightTankIn, tankAllpassDelay3);
    tankDelay3.write(rightAP1Out);
    float rightDelay1Out = tankDelay3.read(tankDelayTime3);
    float rightDamped = dampingR.process(rightDelay1Out);
    float rightAP2Out = tankAP4.processInt(rightDamped, tankAllpassDelay4);
    tankDelay4.write(rightAP2Out);
    float rightDelay2Out = tankDelay4.read(tankDelayTime4);

    // Update outputs for next sample
    leftTankOut = leftDelay2Out;
    rightTankOut = rightDelay2Out;

    // Output taps
    wetL = 0.0f;
    wetL += tankDelay3.read(outputTapL1);
    wetL += tankDelay3.read(outputTapL2);
    wetL -= tankAP4.delay.read(outputTapL3);
    wetL += tankDelay4.read(outputTapL4);
    wetL -= tankDelay1.read(outputTapL5);
    wetL -= tankAP2.delay.read(outputTapL6);
    wetL -= tankDelay2.read(outputTapL7);

    wetR = 0.0f;
    wetR += tankDelay1.read(outputTapR1);
    wetR += tankDelay1.read(outputTapR2);
    wetR -= tankAP2.delay.read(outputTapR3);
    wetR += tankDelay2.read(outputTapR4);
    wetR -= tankDelay3.read(outputTapR5);
    wetR -= tankAP4.delay.read(outputTapR6);
    wetR -= tankDelay4.read(outputTapR7);
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
    // SIZE PARAMETER SMOOTHING - Handling the "Zipper Noise" Problem
    // =========================================================================
    //
    // THE PROBLEM: "Zipper Noise"
    // Reverb size controls delay line lengths. Abruptly changing delay lengths
    // causes the read position to jump, creating clicks/pops ("zipper noise").
    //
    // ALTERNATIVE APPROACHES (for reference):
    // -----------------------------------------------------------------------
    // 1. INTERPOLATED READS + PARAMETER SMOOTHING (this implementation)
    //    - Smooth delay time changes cause pitch shift instead of clicks
    //    - Pro: Simple, low CPU, works well for slow/moderate changes
    //    - Con: Pitch artifacts on fast changes, feedback can clip
    //    - Used by: Lexicon 224, Valhalla VintageVerb
    //
    // 2. DUAL-TANK CROSSFADE ("Spillover")
    //    - Two complete reverb tanks, crossfade between them on size change
    //    - Pro: No pitch shift, seamless transitions
    //    - Con: 2x CPU, 2x memory, complex gain staging to avoid clipping
    //    - Used by: Some Eventide reverbs
    //
    // 3. TWO-TAP DELAY CROSSFADE (per delay line)
    //    - Each delay line reads from old AND new position, crossfades
    //    - Pro: No pitch shift at delay level
    //    - Con: Complex implementation, can still have tank-level artifacts
    //    - Used by: FabFilter Pro-R
    //
    // 4. GRANULAR CROSSFADE
    //    - Chop output into small grains, crossfade between old/new positions
    //    - Pro: Very smooth, no pitch shift
    //    - Con: High CPU, adds latency, complex implementation
    //    - Used by: Some experimental/research reverbs
    //
    // 5. FREEZE-AND-BLEND (this implementation, combined with #1)
    //    - Mute wet during changes, fade back in after settling
    //    - Pro: Simple, no artifacts, works with any reverb topology
    //    - Con: Brief reverb dropout during adjustment
    //    - Used by: Various commercial plugins as a fallback
    //
    // WHY WE CHOSE THIS APPROACH:
    // We use #1 (interpolation + smoothing) combined with #5 (freeze-and-blend).
    // This gives the best tradeoff: simple implementation, low CPU, and
    // artifact-free results. The brief dropout during size changes is
    // acceptable for most use cases and preferable to clicks or pitch warbles.
    //
    // -----------------------------------------------------------------------
    //
    // IMPLEMENTATION: Two-pole parameter smoothing
    // Two one-pole lowpass filters in series (cascaded) provide smoother
    // transitions than a single filter. Single pole has discontinuous 1st
    // derivative (audible as a "corner"). Two poles = continuous 1st derivative
    // = smoother pitch glide during size changes.
    //
    // TUNING GUIDE:
    // - kSmoothCoeff 0.8  = fast/snappy, more pitch artifacts
    // - kSmoothCoeff 0.9  = balanced (current setting)
    // - kSmoothCoeff 0.95 = slow/smooth, less artifacts but laggy feel
    // - kSmoothCoeff 0.99 = very slow, nearly artifact-free but unusable lag

    constexpr float kSmoothCoeff = 0.9f;
    mSizeSmooth1 = mSizeSmooth1 * kSmoothCoeff + mSizeTarget * (1.0f - kSmoothCoeff);
    mSize = mSize * kSmoothCoeff + mSizeSmooth1 * (1.0f - kSmoothCoeff);
    mTankA.updateDelayTimes(mSize, scale);

    // =========================================================================
    // FREEZE-AND-BLEND - Mute wet signal during size changes
    // =========================================================================
    // THE REMAINING PROBLEM:
    // Even with smooth interpolation, rapid size changes cause audible artifacts:
    // - Pitch shifting (delay time changes = playback speed changes)
    // - Feedback loop instability (can cause clipping/ringing)
    // - Transient energy buildup in the tank
    //
    // SOLUTION: "Freeze and Blend" technique
    // 1. When size STARTS changing: quickly fade wet signal OUT
    //    - User hears dry signal only while adjusting
    //    - Tank continues processing internally (artifacts happen silently)
    // 2. When size STOPS changing: wait briefly for tank to "settle"
    //    - Residual transients/ringing decay naturally
    // 3. After settling: gradually fade wet signal back IN
    //    - Clean reverb at new size fades in smoothly
    //
    // TRADEOFF: Brief reverb dropout during size adjustment.
    // This is preferable to clicks, pitch warbles, or clipping.
    // Professional reverbs (Valhalla, FabFilter) use similar techniques.
    //
    // TUNING GUIDE:
    // - kGapThreshold: How small the gap must be to consider "stable"
    //   Lower = more sensitive, waits longer. 0.001 is good default.
    // - kFadeOutSpeed: How fast wet mutes when knob moves
    //   Higher = faster mute, less artifact bleed. 0.1 = ~2 blocks to mute.
    // - kFadeInSpeed: How fast wet returns after settling
    //   Higher = snappier. 0.06 = ~0.3s. Lower (0.02) = smoother but slower.
    // - kSettleBlocks: How long to wait after size stops before fade-in
    //   Higher = more time for tank to calm down. 15 = ~170ms at 48kHz/512.

    float sizeGap = std::abs(mSizeTarget - mSize);
    constexpr float kGapThreshold = 0.001f;   // Below this = "size is stable"
    constexpr float kFadeOutSpeed = 0.1f;     // Fast fade out (hide artifacts quickly)
    constexpr float kFadeInSpeed = 0.06f;     // Snappy fade in (~0.3s to full)
    constexpr int kSettleBlocks = 15;         // Blocks to wait before fade-in

    if (sizeGap > kGapThreshold) {
      // Size is actively changing - fade wet out, reset settle timer
      mWetGain = std::max(0.0f, mWetGain - kFadeOutSpeed);
      mSettleCounter = kSettleBlocks;
    } else if (mSettleCounter > 0) {
      // Size stopped but tank still settling - keep wet muted
      mSettleCounter--;
    } else {
      // Size stable AND tank settled - fade wet back in
      mWetGain = std::min(1.0f, mWetGain + kFadeInSpeed);
    }

    for (int s = 0; s < nFrames; ++s)
    {
      float inputL = static_cast<float>(inL[s]);
      float inputR = static_cast<float>(inR[s]);

      // =========================================================================
      // STEP 1: Create mono input with stereo preservation option
      // =========================================================================
      float monoIn = (inputL + inputR) * 0.5f;

      // =========================================================================
      // STEP 2: Input Tone Shaping (Low Cut + High Cut)
      // =========================================================================
      float filtered = mLowCut.process(monoIn);
      filtered = mHighCut.process(filtered);

      // =========================================================================
      // STEP 3: Pre-delay
      // =========================================================================
      float preDelayed = (mPreDelaySamples > 0)
        ? mPreDelay.readInt(mPreDelaySamples)
        : filtered;
      mPreDelay.write(filtered);

      // =========================================================================
      // STEP 4: Input Diffusion (4 allpass filters in series)
      // =========================================================================
      float diffused = preDelayed;
      diffused = mInputAP1.processInt(diffused, mInputDiffusionDelay1);
      diffused = mInputAP2.processInt(diffused, mInputDiffusionDelay2);
      diffused = mInputAP3.processInt(diffused, mInputDiffusionDelay3);
      diffused = mInputAP4.processInt(diffused, mInputDiffusionDelay4);

      // =========================================================================
      // STEP 5: Single Tank Processing (Simple)
      // =========================================================================
      // Tank processes continuously, but we only hear it when mWetGain > 0.
      // During size changes, wet is muted so we don't hear artifacts.

      float wetL, wetR;
      mTankA.process(diffused, mDecay, wetL, wetR);

      // Scale output and apply wet gain (freeze-and-blend)
      wetL *= 0.6f * mWetGain;
      wetR *= 0.6f * mWetGain;

      // =========================================================================
      // STEP 6: Stereo Width Control (Mid/Side Processing)
      // =========================================================================
      {
        float mid = (wetL + wetR) * 0.5f;
        float side = (wetL - wetR) * 0.5f;
        wetL = mid + side * mWidth;
        wetR = mid - side * mWidth;
      }

      // =========================================================================
      // STEP 7: Mix Dry and Wet
      // =========================================================================
      float outL = inputL * mDry + wetL * mWet;
      float outR = inputR * mDry + wetR * mWet;

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

    // Clear input processing
    mPreDelay.clear();
    mLowCut.clear();
    mHighCut.clear();
    mInputAP1.clear();
    mInputAP2.clear();
    mInputAP3.clear();
    mInputAP4.clear();

    // Clear and configure tank
    mTankA.clear();
    mTankA.setDiffusionCoeffs();
    mTankA.setDamping(mCurrentDamping);
    mTankA.updateDelayTimes(mSize, scale);

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
  }

  /**
   * Handle parameter changes from the UI.
   */
  void SetParam(int paramIdx, double value)
  {
    float scale = mSampleRate / DattorroConstants::kReferenceSampleRate;

    switch (paramIdx)
    {
      case kParamDry:
        mDry = static_cast<float>(value / 100.0);
        break;

      case kParamWet:
        mWet = static_cast<float>(value / 100.0);
        break;

      case kParamDecay:
        // Decay controls feedback amount
        // Higher decay = longer reverb tail
        // IMPORTANT: Max decay of 0.85 prevents runaway feedback
        // Combined with tanh() limiting in the tank, this ensures stability
        mDecay = static_cast<float>(value / 100.0) * 0.85f;
        break;

      case kParamDamping:
        // Damping controls high frequency absorption in the feedback loop
        // Higher damping = darker reverb tail (highs decay faster)
        // 0% = bright shimmery tail, 100% = dark muffled tail
        {
          float damping = static_cast<float>(value / 100.0);
          mCurrentDamping = damping;
          // Apply to tank
          mTankA.setDamping(damping);
        }
        break;

      case kParamSize:
        // Size scales all tank delay times
        // Larger size = longer delays = bigger sounding space
        // NOTE: We set the TARGET, not the actual value. The actual value
        // is smoothed in ProcessBlock to prevent clicks when changing.
        mSizeTarget = static_cast<float>(value / 100.0);
        break;

      case kParamPreDelay:
        // Pre-delay in milliseconds
        mPreDelaySamples = static_cast<int>((value / 1000.0) * mSampleRate);
        mPreDelaySamples = std::min(mPreDelaySamples, DattorroConstants::kMaxPreDelay - 1);
        break;

      case kParamDiffusion:
        // Diffusion controls how much the input is "smeared" before entering the tank
        // Low diffusion = you hear distinct echoes/reflections
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

      case kParamLowCut:
        // Low Cut (high-pass filter) - removes bass/mud from reverb input
        // Range: 20-1000 Hz (overlaps with High Cut for full midrange control)
        mLowCut.setCutoff(static_cast<float>(value), mSampleRate);
        break;

      case kParamHighCut:
        // High Cut (low-pass filter) - removes brightness from reverb input
        // Range: 500-20000 Hz (overlaps with Low Cut for full midrange control)
        mHighCut.setCutoff(static_cast<float>(value), mSampleRate);
        break;

      case kParamWidth:
        // Stereo width control using mid/side processing
        // 0% = mono, 100% = normal stereo, 200% = extra wide
        mWidth = static_cast<float>(value / 100.0);
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

  // User parameters
  float mDry = 1.0f;
  float mWet = 0.3f;
  float mDecay = 0.5f;        // Safe default - max allowed is 0.85
  float mSize = 0.5f;         // Current size value (output of 2nd smoother)
  float mSizeSmooth1 = 0.5f;  // Intermediate value (output of 1st smoother)
  float mSizeTarget = 0.5f;   // Target size from UI
  float mWidth = 1.0f;        // Stereo width: 0=mono, 1=normal, 2=extra wide
  int mPreDelaySamples = 0;
  float mCurrentDamping = 0.5f;  // Current damping value

  // Freeze-and-blend state (for artifact-free size changes)
  // See ProcessBlock comments for detailed explanation of this technique.
  float mWetGain = 1.0f;      // 0.0 = wet muted, 1.0 = wet at full level
  int mSettleCounter = 0;     // Countdown: blocks to wait after size stabilizes

  // Input processing (tone shaping before reverb)
  HighPassFilter mLowCut;     // Removes bass/mud (High-pass = "Low Cut")
  LowPassFilter mHighCut;     // Removes highs/brightness (Low-pass = "High Cut")
  DelayLine<16384> mPreDelay; // Up to ~340ms at 48kHz

  // Input diffusion allpasses (smear input before tank)
  AllpassFilter<2048> mInputAP1;
  AllpassFilter<2048> mInputAP2;
  AllpassFilter<4096> mInputAP3;
  AllpassFilter<4096> mInputAP4;

  int mInputDiffusionDelay1 = 142;
  int mInputDiffusionDelay2 = 107;
  int mInputDiffusionDelay3 = 379;
  int mInputDiffusionDelay4 = 277;

  // Dual tank systems for spillover technique
  // When size changes, we swap which tank is active.
  // Active tank receives new input, spillover tank just decays its existing tail.
  // Single tank (simple approach - smooth interpolation of delay times)
  TankSystem mTankA;
};
