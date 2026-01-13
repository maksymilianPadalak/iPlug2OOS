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
 *   Input → Bandwidth Filter → Pre-Delay → Input Diffusion (4 allpasses)
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

  // LFO modulation parameters
  constexpr float kModExcursion = 16.0f;  // Samples of delay modulation
  constexpr float kDefaultModRate = 1.0f; // Hz
}

// =============================================================================
// DELAY LINE
// =============================================================================
/**
 * Simple delay line with interpolated read.
 *
 * WHY INTERPOLATION:
 * When modulating delay time (for chorus-like effects in the tank),
 * we need fractional sample delays. Linear interpolation gives smooth
 * results without audible artifacts.
 */
template<int MAX_SIZE>
struct DelayLine
{
  std::array<float, MAX_SIZE> buffer{};
  int writePos = 0;

  void clear()
  {
    std::fill(buffer.begin(), buffer.end(), 0.0f);
    writePos = 0;
  }

  void write(float sample)
  {
    buffer[writePos] = sample;
    writePos = (writePos + 1) % MAX_SIZE;
  }

  /**
   * Read with linear interpolation for fractional delays.
   * This is essential for smooth modulation without zipper noise.
   */
  float read(float delaySamples) const
  {
    float readPos = static_cast<float>(writePos) - delaySamples;
    while (readPos < 0.0f) readPos += MAX_SIZE;

    int index0 = static_cast<int>(readPos) % MAX_SIZE;
    int index1 = (index0 + 1) % MAX_SIZE;
    float frac = readPos - std::floor(readPos);

    // Linear interpolation: y = a + (b - a) * t
    return buffer[index0] + (buffer[index1] - buffer[index0]) * frac;
  }

  /**
   * Read at integer sample position (faster, no interpolation)
   */
  float readInt(int delaySamples) const
  {
    int readPos = writePos - delaySamples;
    while (readPos < 0) readPos += MAX_SIZE;
    return buffer[readPos % MAX_SIZE];
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
// BANDWIDTH FILTER (Input Lowpass)
// =============================================================================
/**
 * Input bandwidth control.
 *
 * WHY:
 * Controls the brightness of the signal entering the reverb.
 * Lower bandwidth = darker input = darker reverb overall.
 * This is different from damping which only affects the decay.
 */
struct BandwidthFilter
{
  float state = 0.0f;
  float bandwidth = 0.9995f;  // Very gentle filtering by default

  void clear()
  {
    state = 0.0f;
  }

  void setBandwidth(float bw)
  {
    bandwidth = std::max(0.0f, std::min(1.0f, bw));
  }

  float process(float input)
  {
    state = input * bandwidth + state * (1.0f - bandwidth);
    return state;
  }
};

// =============================================================================
// LFO (Low Frequency Oscillator)
// =============================================================================
/**
 * Simple sine LFO for delay modulation.
 *
 * WHY MODULATION:
 * Without modulation, the fixed delay times can create resonant "ringing"
 * at specific frequencies. Slowly varying the delay times (1-2 Hz) creates
 * slight chorus-like movement that masks these artifacts and adds life.
 */
struct LFO
{
  float phase = 0.0f;
  float phaseIncrement = 0.0f;

  void setRate(float hz, float sampleRate)
  {
    phaseIncrement = hz / sampleRate;
  }

  void reset()
  {
    phase = 0.0f;
  }

  float process()
  {
    float output = std::sin(phase * 2.0f * 3.14159265359f);
    phase += phaseIncrement;
    if (phase >= 1.0f) phase -= 1.0f;
    return output;
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
   * 1. Input bandwidth filtering (optional brightness control)
   * 2. Pre-delay (space before reverb)
   * 3. Input diffusion (4 allpasses to smear transients)
   * 4. Tank processing (figure-8 with cross-feedback)
   * 5. Output taps (multiple points for stereo width)
   * 6. Mix dry and wet
   */
  void ProcessBlock(T** inputs, T** outputs, int nInputs, int nOutputs, int nFrames)
  {
    if (!inputs || !outputs || nOutputs < 1 || nInputs < 1)
      return;

    T* inL = inputs[0];
    T* inR = (nInputs > 1 && inputs[1]) ? inputs[1] : inputs[0];

    for (int s = 0; s < nFrames; ++s)
    {
      float inputL = static_cast<float>(inL[s]);
      float inputR = static_cast<float>(inR[s]);

      // =========================================================================
      // STEP 1: Create mono input with stereo preservation option
      // =========================================================================
      // For simplicity, we sum to mono. A more advanced version could
      // process stereo separately or use mid-side encoding.
      float monoIn = (inputL + inputR) * 0.5f;

      // =========================================================================
      // STEP 2: Bandwidth filter (input brightness)
      // =========================================================================
      float filtered = mBandwidthFilter.process(monoIn);

      // =========================================================================
      // STEP 3: Pre-delay
      // =========================================================================
      // Pre-delay creates space between dry sound and reverb tail.
      // Essential for clarity in vocals and percussion.
      // NOTE: Read BEFORE write to get correct delay behavior.
      float preDelayed = (mPreDelaySamples > 0)
        ? mPreDelay.readInt(mPreDelaySamples)
        : filtered;  // Bypass if no pre-delay
      mPreDelay.write(filtered);

      // =========================================================================
      // STEP 4: Input Diffusion (4 allpass filters in series)
      // =========================================================================
      // These allpasses "smear" the input signal, converting sharp transients
      // into dense, smooth textures before entering the tank.
      // Without this, you'd hear distinct echoes instead of smooth reverb.
      float diffused = preDelayed;
      diffused = mInputAP1.processInt(diffused, mInputDiffusionDelay1);
      diffused = mInputAP2.processInt(diffused, mInputDiffusionDelay2);
      diffused = mInputAP3.processInt(diffused, mInputDiffusionDelay3);
      diffused = mInputAP4.processInt(diffused, mInputDiffusionDelay4);

      // =========================================================================
      // STEP 5: Tank Processing (Figure-8 Topology)
      // =========================================================================
      // This is where the magic happens. The two tanks cross-feed into each
      // other, creating echo density that builds over time.
      //
      // CRITICAL: We must save BOTH previous outputs BEFORE processing either
      // tank. Otherwise we get a one-sample feedback loop that causes instability.

      // Save previous frame's outputs for cross-feedback
      // This is ESSENTIAL - using current values causes runaway feedback!
      float prevLeftOut = mLeftTankOut;
      float prevRightOut = mRightTankOut;

      // ----- LEFT TANK -----
      // Input: diffused signal + feedback from RIGHT tank (cross-coupling!)
      // Using prevRightOut (previous sample) NOT current!
      float leftTankIn = diffused + prevRightOut * mDecay;

      // Soft limit the tank input to prevent runaway (critical for stability!)
      leftTankIn = std::tanh(leftTankIn);

      // First allpass
      float leftAP1Out = mTankAP1.process(leftTankIn, mTankAllpassDelay1);

      // First delay
      mTankDelay1.write(leftAP1Out);
      float leftDelay1Out = mTankDelay1.read(mTankDelayTime1);

      // Damping filter (high frequencies decay faster)
      float leftDamped = mDampingL.process(leftDelay1Out);

      // Second allpass
      float leftAP2Out = mTankAP2.processInt(leftDamped, mTankAllpassDelay2);

      // Second delay
      mTankDelay2.write(leftAP2Out);
      float leftDelay2Out = mTankDelay2.read(mTankDelayTime2);

      // ----- RIGHT TANK -----
      // Input: diffused signal + feedback from LEFT tank (cross-coupling!)
      // Using prevLeftOut (previous sample) NOT current!
      float rightTankIn = diffused + prevLeftOut * mDecay;

      // Soft limit the tank input to prevent runaway (critical for stability!)
      rightTankIn = std::tanh(rightTankIn);

      // First allpass
      float rightAP1Out = mTankAP3.process(rightTankIn, mTankAllpassDelay3);

      // First delay
      mTankDelay3.write(rightAP1Out);
      float rightDelay1Out = mTankDelay3.read(mTankDelayTime3);

      // Damping filter
      float rightDamped = mDampingR.process(rightDelay1Out);

      // Second allpass
      float rightAP2Out = mTankAP4.processInt(rightDamped, mTankAllpassDelay4);

      // Second delay
      mTankDelay4.write(rightAP2Out);
      float rightDelay2Out = mTankDelay4.read(mTankDelayTime4);

      // Update tank outputs for NEXT sample's cross-feedback
      mLeftTankOut = leftDelay2Out;
      mRightTankOut = rightDelay2Out;

      // =========================================================================
      // STEP 6: Output Taps (Stereo Decorrelation)
      // =========================================================================
      // We tap from multiple points in BOTH tanks to create a wide stereo image.
      // Left output takes from right tank (and vice versa) for maximum decorrelation.

      float wetL = 0.0f;
      float wetR = 0.0f;

      // Left output: taps from right tank delays
      wetL += mTankDelay3.read(mOutputTapL1);
      wetL += mTankDelay3.read(mOutputTapL2);
      wetL -= mTankAP4.delay.read(mOutputTapL3);
      wetL += mTankDelay4.read(mOutputTapL4);
      wetL -= mTankDelay1.read(mOutputTapL5);
      wetL -= mTankAP2.delay.read(mOutputTapL6);
      wetL -= mTankDelay2.read(mOutputTapL7);

      // Right output: taps from left tank delays
      wetR += mTankDelay1.read(mOutputTapR1);
      wetR += mTankDelay1.read(mOutputTapR2);
      wetR -= mTankAP2.delay.read(mOutputTapR3);
      wetR += mTankDelay2.read(mOutputTapR4);
      wetR -= mTankDelay3.read(mOutputTapR5);
      wetR -= mTankAP4.delay.read(mOutputTapR6);
      wetR -= mTankDelay4.read(mOutputTapR7);

      // Scale wet signal
      wetL *= 0.6f;
      wetR *= 0.6f;

      // =========================================================================
      // STEP 7: Mix Dry and Wet
      // =========================================================================
      float outL = inputL * mDry + wetL * mWet;
      float outR = inputR * mDry + wetR * mWet;

      // Safety limiting (soft clip instead of hard clip for smoother sound)
      outL = std::tanh(outL);
      outR = std::tanh(outR);

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

    // Clear all delay lines and filters
    mPreDelay.clear();
    mBandwidthFilter.clear();
    mInputAP1.clear();
    mInputAP2.clear();
    mInputAP3.clear();
    mInputAP4.clear();
    mTankAP1.clear();
    mTankAP2.clear();
    mTankAP3.clear();
    mTankAP4.clear();
    mTankDelay1.clear();
    mTankDelay2.clear();
    mTankDelay3.clear();
    mTankDelay4.clear();
    mDampingL.clear();
    mDampingR.clear();
    mLFO1.reset();
    mLFO2.reset();

    // Reset tank feedback state
    mLeftTankOut = 0.0f;
    mRightTankOut = 0.0f;

    // Set scaled delay times (multiply original times by sample rate ratio)
    mInputDiffusionDelay1 = static_cast<int>(DattorroConstants::kInputDiffusion1 * scale);
    mInputDiffusionDelay2 = static_cast<int>(DattorroConstants::kInputDiffusion2 * scale);
    mInputDiffusionDelay3 = static_cast<int>(DattorroConstants::kInputDiffusion3 * scale);
    mInputDiffusionDelay4 = static_cast<int>(DattorroConstants::kInputDiffusion4 * scale);

    // Scale tank delay times by size parameter
    updateTankDelays(scale);
    updateOutputTaps(scale);

    // Set diffusion coefficients (from Dattorro's paper)
    mInputAP1.setFeedback(DattorroConstants::kInputDiffusionCoeff1);
    mInputAP2.setFeedback(DattorroConstants::kInputDiffusionCoeff1);
    mInputAP3.setFeedback(DattorroConstants::kInputDiffusionCoeff2);
    mInputAP4.setFeedback(DattorroConstants::kInputDiffusionCoeff2);

    // Tank allpass coefficients
    mTankAP1.setFeedback(DattorroConstants::kDecayDiffusionCoeff1);
    mTankAP2.setFeedback(DattorroConstants::kDecayDiffusionCoeff2);
    mTankAP3.setFeedback(DattorroConstants::kDecayDiffusionCoeff1);
    mTankAP4.setFeedback(DattorroConstants::kDecayDiffusionCoeff2);

    // Set LFO rates (slightly different for L/R to avoid correlation)
    mLFO1.setRate(DattorroConstants::kDefaultModRate, mSampleRate);
    mLFO2.setRate(DattorroConstants::kDefaultModRate * 0.97f, mSampleRate);
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
        // Damping controls high frequency absorption
        // Higher damping = darker reverb tail
        {
          float damping = static_cast<float>(value / 100.0);
          mDampingL.setCoeff(damping * 0.7f);
          mDampingR.setCoeff(damping * 0.7f);
        }
        break;

      case kParamSize:
        // Size scales all tank delay times
        // Larger size = longer delays = bigger sounding space
        mSize = static_cast<float>(value / 100.0);
        updateTankDelays(scale);
        updateOutputTaps(scale);
        break;

      case kParamPreDelay:
        // Pre-delay in milliseconds
        mPreDelaySamples = static_cast<int>((value / 1000.0) * mSampleRate);
        mPreDelaySamples = std::min(mPreDelaySamples, DattorroConstants::kMaxPreDelay - 1);
        break;

      case kParamDiffusion:
        // Diffusion controls input allpass feedback
        // Higher diffusion = smoother, more washed out sound
        {
          float diff = static_cast<float>(value / 100.0);
          float coeff1 = 0.5f + diff * 0.25f;   // 0.5 to 0.75
          float coeff2 = 0.375f + diff * 0.25f; // 0.375 to 0.625
          mInputAP1.setFeedback(coeff1);
          mInputAP2.setFeedback(coeff1);
          mInputAP3.setFeedback(coeff2);
          mInputAP4.setFeedback(coeff2);
        }
        break;

      case kParamModRate:
        // Modulation rate in Hz
        mLFO1.setRate(static_cast<float>(value), mSampleRate);
        mLFO2.setRate(static_cast<float>(value) * 0.97f, mSampleRate);
        break;

      case kParamModDepth:
        // Modulation depth in samples
        mModDepth = static_cast<float>(value / 100.0) * DattorroConstants::kModExcursion;
        break;

      default:
        break;
    }
  }

private:
  /**
   * Update tank delay times based on size parameter.
   */
  void updateTankDelays(float scale)
  {
    // Size range: 0.5 to 2.0 (50% to 200%)
    float sizeScale = 0.5f + mSize * 1.5f;

    mTankAllpassDelay1 = DattorroConstants::kTankAllpass1 * scale * sizeScale;
    mTankDelayTime1 = DattorroConstants::kTankDelay1 * scale * sizeScale;
    mTankAllpassDelay2 = static_cast<int>(DattorroConstants::kTankAllpass2 * scale * sizeScale);
    mTankDelayTime2 = DattorroConstants::kTankDelay2 * scale * sizeScale;

    mTankAllpassDelay3 = DattorroConstants::kTankAllpass3 * scale * sizeScale;
    mTankDelayTime3 = DattorroConstants::kTankDelay3 * scale * sizeScale;
    mTankAllpassDelay4 = static_cast<int>(DattorroConstants::kTankAllpass4 * scale * sizeScale);
    mTankDelayTime4 = DattorroConstants::kTankDelay4 * scale * sizeScale;
  }

  /**
   * Update output tap positions based on size parameter.
   */
  void updateOutputTaps(float scale)
  {
    float sizeScale = 0.5f + mSize * 1.5f;

    // Left output taps
    mOutputTapL1 = DattorroConstants::kLeftTap1 * scale * sizeScale;
    mOutputTapL2 = DattorroConstants::kLeftTap2 * scale * sizeScale;
    mOutputTapL3 = DattorroConstants::kLeftTap3 * scale * sizeScale;
    mOutputTapL4 = DattorroConstants::kLeftTap4 * scale * sizeScale;
    mOutputTapL5 = DattorroConstants::kLeftTap5 * scale * sizeScale;
    mOutputTapL6 = DattorroConstants::kLeftTap6 * scale * sizeScale;
    mOutputTapL7 = DattorroConstants::kLeftTap7 * scale * sizeScale;

    // Right output taps
    mOutputTapR1 = DattorroConstants::kRightTap1 * scale * sizeScale;
    mOutputTapR2 = DattorroConstants::kRightTap2 * scale * sizeScale;
    mOutputTapR3 = DattorroConstants::kRightTap3 * scale * sizeScale;
    mOutputTapR4 = DattorroConstants::kRightTap4 * scale * sizeScale;
    mOutputTapR5 = DattorroConstants::kRightTap5 * scale * sizeScale;
    mOutputTapR6 = DattorroConstants::kRightTap6 * scale * sizeScale;
    mOutputTapR7 = DattorroConstants::kRightTap7 * scale * sizeScale;
  }

  // ===========================================================================
  // MEMBER VARIABLES
  // ===========================================================================

  // Sample rate
  float mSampleRate = 44100.0f;

  // User parameters
  float mDry = 1.0f;
  float mWet = 0.3f;
  float mDecay = 0.5f;  // Safe default - max allowed is 0.85
  float mSize = 0.5f;
  float mModDepth = 8.0f;
  int mPreDelaySamples = 0;

  // Input processing
  BandwidthFilter mBandwidthFilter;
  DelayLine<16384> mPreDelay;  // Up to ~340ms at 48kHz

  // Input diffusion allpasses (smear input before tank)
  AllpassFilter<2048> mInputAP1;
  AllpassFilter<2048> mInputAP2;
  AllpassFilter<4096> mInputAP3;
  AllpassFilter<4096> mInputAP4;

  int mInputDiffusionDelay1 = 142;
  int mInputDiffusionDelay2 = 107;
  int mInputDiffusionDelay3 = 379;
  int mInputDiffusionDelay4 = 277;

  // Tank components - Left
  AllpassFilter<8192> mTankAP1;   // Modulated
  DelayLine<32768> mTankDelay1;
  AllpassFilter<8192> mTankAP2;
  DelayLine<32768> mTankDelay2;
  OnePoleLowpass mDampingL;

  // Tank components - Right
  AllpassFilter<8192> mTankAP3;   // Modulated
  DelayLine<32768> mTankDelay3;
  AllpassFilter<8192> mTankAP4;
  DelayLine<32768> mTankDelay4;
  OnePoleLowpass mDampingR;

  // Tank delay times (floats for modulation)
  float mTankAllpassDelay1 = 672.0f;
  float mTankDelayTime1 = 4453.0f;
  int mTankAllpassDelay2 = 1800;
  float mTankDelayTime2 = 3720.0f;

  float mTankAllpassDelay3 = 908.0f;
  float mTankDelayTime3 = 4217.0f;
  int mTankAllpassDelay4 = 2656;
  float mTankDelayTime4 = 3163.0f;

  // Output tap positions
  float mOutputTapL1, mOutputTapL2, mOutputTapL3, mOutputTapL4;
  float mOutputTapL5, mOutputTapL6, mOutputTapL7;
  float mOutputTapR1, mOutputTapR2, mOutputTapR3, mOutputTapR4;
  float mOutputTapR5, mOutputTapR6, mOutputTapR7;

  // Cross-feedback storage
  // CRITICAL: These store the PREVIOUS frame's tank outputs
  // Reading from these (not from current delays) prevents infinite feedback
  float mLeftTankOut = 0.0f;
  float mRightTankOut = 0.0f;

  // Modulation LFOs
  LFO mLFO1;
  LFO mLFO2;
};
