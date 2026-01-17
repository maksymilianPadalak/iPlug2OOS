#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// STEREO DELAY - Simple delay with separate dry/wet levels
// ═══════════════════════════════════════════════════════════════════════════════
// Standalone stereo delay implementation featuring Hermite interpolation,
// DC blocking, soft saturation, and tempo sync support.
// ═══════════════════════════════════════════════════════════════════════════════

#include <vector>
#include <cmath>
#include <algorithm>
#include "dsp_utilities.h"

// Q DSP Library - DC blocker
#include <q/support/literals.hpp>
#include <q/fx/dc_block.hpp>

namespace q = cycfi::q;
using namespace q::literals;

// ═══════════════════════════════════════════════════════════════════════════════
// DELAY MODE - Stereo behavior
// ═══════════════════════════════════════════════════════════════════════════════
enum class DelayMode
{
  Stereo = 0,   // Parallel L/R delays (standard stereo)
  PingPong,     // Alternating L/R delays (classic ping-pong effect)
};

// ═══════════════════════════════════════════════════════════════════════════════
// DELAY SYNC RATES - Musical tempo divisions
// ═══════════════════════════════════════════════════════════════════════════════
enum class DelaySyncRate
{
  Off = 0,            // Use manual ms rate
  Whole,              // 1/1 - whole note (4 beats)
  HalfDotted,         // 1/2D - dotted half (3 beats)
  Half,               // 1/2 - half note (2 beats)
  HalfTriplet,        // 1/2T - half triplet (4/3 beats)
  QuarterDotted,      // 1/4D - dotted quarter (1.5 beats)
  Quarter,            // 1/4 - quarter note (1 beat)
  QuarterTriplet,     // 1/4T - quarter triplet (2/3 beat)
  EighthDotted,       // 1/8D - dotted eighth (0.75 beats)
  Eighth,             // 1/8 - eighth note (0.5 beats)
  EighthTriplet,      // 1/8T - eighth triplet (1/3 beat)
  SixteenthDotted,    // 1/16D - dotted sixteenth (0.375 beats)
  Sixteenth,          // 1/16 - sixteenth (0.25 beats)
  SixteenthTriplet,   // 1/16T - sixteenth triplet (1/6 beat)
  ThirtySecond,       // 1/32 - thirty-second (0.125 beats)
  kNumSyncRates
};

// ═══════════════════════════════════════════════════════════════════════════════
// TEMPO SYNC HELPER - Convert delay sync rate to milliseconds
// ═══════════════════════════════════════════════════════════════════════════════
inline float DelaySyncRateToMs(DelaySyncRate syncRate, float bpm)
{
  if (syncRate == DelaySyncRate::Off || bpm <= 0.0f)
    return 0.0f;

  // Milliseconds per beat = 60000 / BPM
  float msPerBeat = 60000.0f / bpm;

  // Beats per note for each sync rate
  float beatsPerNote = 1.0f;

  switch (syncRate)
  {
    case DelaySyncRate::Whole:            beatsPerNote = 4.0f; break;
    case DelaySyncRate::HalfDotted:       beatsPerNote = 3.0f; break;
    case DelaySyncRate::Half:             beatsPerNote = 2.0f; break;
    case DelaySyncRate::HalfTriplet:      beatsPerNote = 4.0f / 3.0f; break;
    case DelaySyncRate::QuarterDotted:    beatsPerNote = 1.5f; break;
    case DelaySyncRate::Quarter:          beatsPerNote = 1.0f; break;
    case DelaySyncRate::QuarterTriplet:   beatsPerNote = 2.0f / 3.0f; break;
    case DelaySyncRate::EighthDotted:     beatsPerNote = 0.75f; break;
    case DelaySyncRate::Eighth:           beatsPerNote = 0.5f; break;
    case DelaySyncRate::EighthTriplet:    beatsPerNote = 1.0f / 3.0f; break;
    case DelaySyncRate::SixteenthDotted:  beatsPerNote = 0.375f; break;
    case DelaySyncRate::Sixteenth:        beatsPerNote = 0.25f; break;
    case DelaySyncRate::SixteenthTriplet: beatsPerNote = 1.0f / 6.0f; break;
    case DelaySyncRate::ThirtySecond:     beatsPerNote = 0.125f; break;
    default: return 0.0f;
  }

  return msPerBeat * beatsPerNote;
}

// ═══════════════════════════════════════════════════════════════════════════════
// STEREO DELAY - Simple delay with separate dry/wet levels
// ═══════════════════════════════════════════════════════════════════════════════
//
// FEATURES:
// - Hermite interpolation: 4-point cubic interpolation for smooth delay time
//   modulation without artifacts (superior to linear interpolation)
// - DC blocking: Prevents DC offset buildup in feedback loop
// - Soft saturation: Prevents runaway feedback while adding warmth
// - Tempo sync: Lock delay time to musical divisions
// - Separate dry/wet levels: Independent control of original and delayed signal
//
// HERMITE INTERPOLATION:
// Uses 4-point, 3rd-order Hermite polynomial for reading fractional delay times.
// This preserves high frequencies better than linear interpolation and reduces
// aliasing artifacts when delay time is modulated.
//
// ═══════════════════════════════════════════════════════════════════════════════
class StereoDelay
{
public:
  // Maximum delay time in seconds (2 seconds = 2000ms)
  static constexpr float kMaxDelaySeconds = 2.0f;

  // Maximum feedback to prevent runaway (90% is safe with saturation)
  static constexpr float kMaxFeedback = 0.90f;

  // ─────────────────────────────────────────────────────────────────────────────
  // INITIALIZATION
  // ─────────────────────────────────────────────────────────────────────────────
  void SetSampleRate(float sampleRate)
  {
    mSampleRate = sampleRate;

    // Smoothing coefficient for delay time changes (~10ms)
    mSmoothCoeff = calcSmoothingCoeff(0.010f, sampleRate);

    // Resize delay buffers (+4 for hermite interpolation safety margin)
    int maxSamples = static_cast<int>(kMaxDelaySeconds * sampleRate) + 4;
    mDelayBufferL.resize(maxSamples, 0.0f);
    mDelayBufferR.resize(maxSamples, 0.0f);

    // Initialize DC blockers (10Hz cutoff for minimal audible effect)
    mDCBlockL = q::dc_block{10_Hz, sampleRate};
    mDCBlockR = q::dc_block{10_Hz, sampleRate};

    Reset();
  }

  void Reset()
  {
    // Clear delay buffers
    std::fill(mDelayBufferL.begin(), mDelayBufferL.end(), 0.0f);
    std::fill(mDelayBufferR.begin(), mDelayBufferR.end(), 0.0f);

    // Reset write positions
    mWriteIndex = 0;

    // Snap current delay to target (no smoothing on reset)
    mDelayTimeSamplesCurrent = mDelayTimeSamplesTarget;

    // Snap dry/wet levels to targets (no smoothing on reset)
    mDryLevelSmoothed = mDryLevelTarget;
    mWetLevelSmoothed = mWetLevelTarget;

    // Reset DC blockers
    mDCBlockL = 0.0f;
    mDCBlockR = 0.0f;
  }

  // ─────────────────────────────────────────────────────────────────────────────
  // PARAMETER SETTERS
  // ─────────────────────────────────────────────────────────────────────────────
  void SetDelayTime(float timeMs)
  {
    // Convert ms to samples, clamp to valid range
    float delaySamples = timeMs * 0.001f * mSampleRate;
    float maxDelay = static_cast<float>(mDelayBufferL.size() - 4);  // -4 for hermite
    mDelayTimeSamplesTarget = std::max(1.0f, std::min(maxDelay, delaySamples));
  }

  void SetFeedback(float feedback)
  {
    // Clamp to safe range (0-90%)
    mFeedback = std::max(0.0f, std::min(kMaxFeedback, feedback));
  }

  void SetDryLevel(float level)
  {
    mDryLevelTarget = std::max(0.0f, std::min(1.0f, level));
  }

  void SetWetLevel(float level)
  {
    mWetLevelTarget = std::max(0.0f, std::min(1.0f, level));
  }

  void SetMode(DelayMode mode)
  {
    mMode = mode;
  }

  // ─────────────────────────────────────────────────────────────────────────────
  // PROCESSING
  // ─────────────────────────────────────────────────────────────────────────────
  void Process(float& left, float& right)
  {
    if (mDelayBufferL.empty() || mDelayBufferR.empty())
      return;

    int bufferSize = static_cast<int>(mDelayBufferL.size());

    // ─────────────────────────────────────────────────────────────────────────
    // SMOOTH PARAMETERS (always, even when bypassed - ensures smooth transitions)
    // ─────────────────────────────────────────────────────────────────────────
    mDryLevelSmoothed += mSmoothCoeff * (mDryLevelTarget - mDryLevelSmoothed);
    mWetLevelSmoothed += mSmoothCoeff * (mWetLevelTarget - mWetLevelSmoothed);

    float timeDiff = mDelayTimeSamplesTarget - mDelayTimeSamplesCurrent;
    if (std::abs(timeDiff) > 0.001f)
    {
      mDelayTimeSamplesCurrent += mSmoothCoeff * timeDiff;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // BYPASS OPTIMIZATION: Skip expensive processing when wet is inaudible
    // Threshold 0.0001 = -80dB, below audible range
    // Still write to buffer so delay is "primed" when wet fades back in
    // ─────────────────────────────────────────────────────────────────────────
    constexpr float kBypassThreshold = 0.0001f;  // -80dB
    if (mWetLevelSmoothed < kBypassThreshold && mWetLevelTarget < kBypassThreshold)
    {
      // Write input to buffer (no feedback when bypassed)
      if (mMode == DelayMode::PingPong)
      {
        float monoIn = (left + right) * 0.5f;
        mDelayBufferL[mWriteIndex] = monoIn;
        mDelayBufferR[mWriteIndex] = 0.0f;
      }
      else
      {
        mDelayBufferL[mWriteIndex] = left;
        mDelayBufferR[mWriteIndex] = right;
      }
      mWriteIndex = (mWriteIndex + 1) % bufferSize;

      // Output dry only (skip hermite interpolation, DC blocking, saturation)
      left *= mDryLevelSmoothed;
      right *= mDryLevelSmoothed;
      return;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // READ FROM DELAY BUFFER WITH HERMITE INTERPOLATION
    // ─────────────────────────────────────────────────────────────────────────
    float readPos = static_cast<float>(mWriteIndex) - mDelayTimeSamplesCurrent;
    if (readPos < 0.0f) readPos += static_cast<float>(bufferSize);

    float delayedL = HermiteInterpolate(mDelayBufferL, readPos, bufferSize);
    float delayedR = HermiteInterpolate(mDelayBufferR, readPos, bufferSize);

    // ─────────────────────────────────────────────────────────────────────────
    // FEEDBACK PATH: DC Block → Soft Saturation
    // ─────────────────────────────────────────────────────────────────────────
    float processedL = mDCBlockL(delayedL);
    float processedR = mDCBlockR(delayedR);

    // Soft saturation - prevents runaway while adding warmth
    if (std::abs(processedL) > 0.8f)
    {
      processedL = softSaturate(processedL);
    }
    if (std::abs(processedR) > 0.8f)
    {
      processedR = softSaturate(processedR);
    }

    // ─────────────────────────────────────────────────────────────────────────
    // WRITE TO DELAY BUFFER
    // ─────────────────────────────────────────────────────────────────────────
    if (mMode == DelayMode::PingPong)
    {
      // PING-PONG: Cross-feedback creates alternating L/R pattern
      float monoIn = (left + right) * 0.5f;
      mDelayBufferL[mWriteIndex] = monoIn + mFeedback * processedR;
      mDelayBufferR[mWriteIndex] = mFeedback * processedL;
    }
    else
    {
      // STEREO: Parallel L/R delays (standard behavior)
      mDelayBufferL[mWriteIndex] = left + mFeedback * processedL;
      mDelayBufferR[mWriteIndex] = right + mFeedback * processedR;
    }

    // Flush denormals in buffer
    mDelayBufferL[mWriteIndex] = flushDenormal(mDelayBufferL[mWriteIndex]);
    mDelayBufferR[mWriteIndex] = flushDenormal(mDelayBufferR[mWriteIndex]);

    // Advance write index with wrap
    mWriteIndex = (mWriteIndex + 1) % bufferSize;

    // ─────────────────────────────────────────────────────────────────────────
    // OUTPUT: Separate dry and wet levels
    // Use processedL/R (saturated) for wet output, not raw delayedL/R
    // This prevents accumulated buffer values (>1.0) from hitting master output
    // ─────────────────────────────────────────────────────────────────────────
    left = left * mDryLevelSmoothed + processedL * mWetLevelSmoothed;
    right = right * mDryLevelSmoothed + processedR * mWetLevelSmoothed;
  }

private:
  // ─────────────────────────────────────────────────────────────────────────────
  // HERMITE INTERPOLATION (4-point, 3rd-order)
  // ─────────────────────────────────────────────────────────────────────────────
  float HermiteInterpolate(const std::vector<float>& buffer, float pos, int size) const
  {
    int i = static_cast<int>(pos);
    float frac = pos - static_cast<float>(i);

    // Get 4 adjacent samples with wraparound
    float y0 = buffer[(i - 1 + size) % size];
    float y1 = buffer[i % size];
    float y2 = buffer[(i + 1) % size];
    float y3 = buffer[(i + 2) % size];

    // Hermite polynomial coefficients
    float c0 = y1;
    float c1 = 0.5f * (y2 - y0);
    float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);

    // Evaluate: c0 + c1*x + c2*x^2 + c3*x^3
    return ((c3 * frac + c2) * frac + c1) * frac + c0;
  }

  // ─────────────────────────────────────────────────────────────────────────────
  // MEMBER VARIABLES
  // ─────────────────────────────────────────────────────────────────────────────
  float mSampleRate = 48000.0f;
  float mDelayTimeSamplesTarget = 12000.0f;   // ~250ms at 48kHz
  float mDelayTimeSamplesCurrent = 12000.0f;
  float mFeedback = 0.0f;
  float mDryLevelTarget = 1.0f;    // Target: full dry signal
  float mWetLevelTarget = 0.0f;    // Target: no wet signal
  float mDryLevelSmoothed = 1.0f;  // Smoothed value (click-free)
  float mWetLevelSmoothed = 0.0f;  // Smoothed value (click-free)
  float mSmoothCoeff = 0.01f;
  DelayMode mMode = DelayMode::Stereo;

  // Delay buffers
  std::vector<float> mDelayBufferL;
  std::vector<float> mDelayBufferR;
  int mWriteIndex = 0;

  // DC blockers (Q library)
  q::dc_block mDCBlockL{10_Hz, 48000.0f};
  q::dc_block mDCBlockR{10_Hz, 48000.0f};
};
