#pragma once

#include <cmath>
#include <array>
#include <algorithm>
#include <memory>

// Q DSP Library includes
#include <q/support/literals.hpp>
#include <q/fx/delay.hpp>
#include <q/fx/allpass.hpp>
#include <q/fx/lowpass.hpp>

using namespace iplug;
using namespace cycfi::q::literals;

// =============================================================================
// Freeverb-style Reverb using Q library allpass filters
// =============================================================================
class QReverb
{
public:
  QReverb() = default;

  void init(float sampleRate)
  {
    mSampleRate = sampleRate;

    // Initialize allpass filters at different frequencies for diffusion
    // Left channel
    mAllpassL[0] = cycfi::q::one_pole_allpass(100_Hz, sampleRate);
    mAllpassL[1] = cycfi::q::one_pole_allpass(230_Hz, sampleRate);
    mAllpassL[2] = cycfi::q::one_pole_allpass(370_Hz, sampleRate);
    mAllpassL[3] = cycfi::q::one_pole_allpass(520_Hz, sampleRate);

    // Right channel (slightly different frequencies for stereo)
    mAllpassR[0] = cycfi::q::one_pole_allpass(110_Hz, sampleRate);
    mAllpassR[1] = cycfi::q::one_pole_allpass(240_Hz, sampleRate);
    mAllpassR[2] = cycfi::q::one_pole_allpass(380_Hz, sampleRate);
    mAllpassR[3] = cycfi::q::one_pole_allpass(530_Hz, sampleRate);

    // Lowpass filters for damping
    updateDamping();

    // Initialize delay lines for comb filter simulation
    float maxDelayTime = 0.1f;  // 100ms max
    mCombDelayL.reset(new cycfi::q::delay(cycfi::q::duration(maxDelayTime), sampleRate));
    mCombDelayR.reset(new cycfi::q::delay(cycfi::q::duration(maxDelayTime), sampleRate));
  }

  void setRoomSize(float size)
  {
    mRoomSize = std::clamp(size, 0.0f, 1.0f);
    mFeedback = 0.28f + (mRoomSize * 0.7f);
  }

  void setDamping(float damp)
  {
    mDamping = std::clamp(damp, 0.0f, 1.0f);
    updateDamping();
  }

  void setWidth(float width)
  {
    mWidth = std::clamp(width, 0.0f, 1.0f);
    mWet1 = (1.0f + mWidth) / 2.0f;
    mWet2 = (1.0f - mWidth) / 2.0f;
  }

  void process(float inputL, float inputR, float& outL, float& outR)
  {
    // Mix to mono and scale
    float input = (inputL + inputR) * 0.015f;

    // Comb filter simulation using delay with feedback
    float delaySamples = mRoomSize * 0.08f * mSampleRate;  // Up to 80ms based on room size
    if (delaySamples < 1.0f) delaySamples = 1.0f;

    float combOutL = 0.0f;
    float combOutR = 0.0f;

    if (mCombDelayL && mCombDelayR)
    {
      // Left comb
      float delayedL = (*mCombDelayL)(delaySamples);
      float filteredL = mDampLpL(delayedL);
      mCombDelayL->push(input + filteredL * mFeedback);
      combOutL = delayedL;

      // Right comb (slightly different delay for stereo)
      float delayedR = (*mCombDelayR)(delaySamples * 1.03f);
      float filteredR = mDampLpR(delayedR);
      mCombDelayR->push(input + filteredR * mFeedback);
      combOutR = delayedR;
    }

    // Process through allpass chain for diffusion
    for (int i = 0; i < 4; ++i)
    {
      combOutL = mAllpassL[i](combOutL);
      combOutR = mAllpassR[i](combOutR);
    }

    // Final lowpass
    combOutL = mOutputLpL(combOutL);
    combOutR = mOutputLpR(combOutR);

    // Stereo width mixing
    outL = combOutL * mWet1 + combOutR * mWet2;
    outR = combOutR * mWet1 + combOutL * mWet2;
  }

private:
  void updateDamping()
  {
    // Higher damping = lower cutoff frequency
    float cutoffHz = 1000.0f + (1.0f - mDamping) * 7000.0f;  // 1kHz to 8kHz
    if (mSampleRate > 0)
    {
      mDampLpL = cycfi::q::one_pole_lowpass(cycfi::q::frequency(cutoffHz), mSampleRate);
      mDampLpR = cycfi::q::one_pole_lowpass(cycfi::q::frequency(cutoffHz), mSampleRate);
      mOutputLpL = cycfi::q::one_pole_lowpass(cycfi::q::frequency(cutoffHz * 0.8f), mSampleRate);
      mOutputLpR = cycfi::q::one_pole_lowpass(cycfi::q::frequency(cutoffHz * 0.8f), mSampleRate);
    }
  }

  float mSampleRate = 44100.0f;
  float mRoomSize = 0.5f;
  float mDamping = 0.5f;
  float mWidth = 1.0f;
  float mFeedback = 0.63f;
  float mWet1 = 1.0f;
  float mWet2 = 0.0f;

  // Q allpass filters
  std::array<cycfi::q::one_pole_allpass, 4> mAllpassL = {{
    cycfi::q::one_pole_allpass(0.5f),
    cycfi::q::one_pole_allpass(0.5f),
    cycfi::q::one_pole_allpass(0.5f),
    cycfi::q::one_pole_allpass(0.5f)
  }};
  std::array<cycfi::q::one_pole_allpass, 4> mAllpassR = {{
    cycfi::q::one_pole_allpass(0.5f),
    cycfi::q::one_pole_allpass(0.5f),
    cycfi::q::one_pole_allpass(0.5f),
    cycfi::q::one_pole_allpass(0.5f)
  }};

  // Q lowpass filters for damping
  cycfi::q::one_pole_lowpass mDampLpL{0.5f};
  cycfi::q::one_pole_lowpass mDampLpR{0.5f};
  cycfi::q::one_pole_lowpass mOutputLpL{0.5f};
  cycfi::q::one_pole_lowpass mOutputLpR{0.5f};

  // Q delay lines for comb simulation
  std::unique_ptr<cycfi::q::delay> mCombDelayL;
  std::unique_ptr<cycfi::q::delay> mCombDelayR;
};

// =============================================================================
// Stereo Delay using Q library
// =============================================================================
class QStereoDelay
{
public:
  QStereoDelay() = default;

  void init(float sampleRate)
  {
    mSampleRate = sampleRate;
    // Max 1 second delay
    mDelayL.reset(new cycfi::q::delay(1.0_s, sampleRate));
    mDelayR.reset(new cycfi::q::delay(1.0_s, sampleRate));
  }

  void setDelayTime(float delayMs, double sampleRate)
  {
    mDelaySamples = std::min(delayMs * static_cast<float>(sampleRate) / 1000.0f,
                              static_cast<float>(sampleRate) - 1.0f);
  }

  void setFeedback(float feedback)
  {
    mFeedback = std::clamp(feedback, 0.0f, 0.95f);
  }

  void process(float inputL, float inputR, float& outL, float& outR)
  {
    if (!mDelayL || !mDelayR)
    {
      outL = 0.0f;
      outR = 0.0f;
      return;
    }

    // Read delayed samples
    float delayedL = (*mDelayL)(mDelaySamples);
    float delayedR = (*mDelayR)(mDelaySamples);

    // Write input + feedback to delay lines
    mDelayL->push(inputL + delayedL * mFeedback);
    mDelayR->push(inputR + delayedR * mFeedback);

    outL = delayedL;
    outR = delayedR;
  }

private:
  std::unique_ptr<cycfi::q::delay> mDelayL;
  std::unique_ptr<cycfi::q::delay> mDelayR;
  float mSampleRate = 44100.0f;
  float mDelaySamples = 0.0f;
  float mFeedback = 0.0f;
};

// =============================================================================
// Main Effect DSP using Q library
// =============================================================================
template<typename T>
class EffectTemplateDSP
{
public:
  EffectTemplateDSP() = default;

  void ProcessBlock(T** inputs, T** outputs, int nInputs, int nOutputs, int nFrames)
  {
    if (!inputs || !outputs || nOutputs < 1 || nInputs < 1)
      return;

    if (mBypass)
    {
      // Bypass: pass through with gain only
      for (int ch = 0; ch < nOutputs; ++ch)
      {
        const T* in = (ch < nInputs && inputs[ch]) ? inputs[ch] : inputs[0];
        T* out = outputs[ch];
        if (!in || !out) continue;
        for (int s = 0; s < nFrames; ++s)
          out[s] = in[s] * mGain;
      }
      return;
    }

    const T* inL = inputs[0];
    const T* inR = (nInputs > 1 && inputs[1]) ? inputs[1] : inputs[0];
    T* outL = outputs[0];
    T* outR = (nOutputs > 1 && outputs[1]) ? outputs[1] : outputs[0];

    if (!inL || !outL) return;

    for (int s = 0; s < nFrames; ++s)
    {
      float dryL = static_cast<float>(inL[s]);
      float dryR = static_cast<float>(inR[s]);

      // Process delay
      float delayL = 0.0f, delayR = 0.0f;
      mDelay.process(dryL, dryR, delayL, delayR);

      // Process reverb (on delayed signal for serial processing)
      float reverbL = 0.0f, reverbR = 0.0f;
      mReverb.process(dryL + delayL, dryR + delayR, reverbL, reverbR);

      // Mix: dry + delay + reverb based on wet/dry mix
      float wetL = delayL + reverbL;
      float wetR = delayR + reverbR;

      float outSampleL = dryL * (1.0f - mMix) + wetL * mMix;
      float outSampleR = dryR * (1.0f - mMix) + wetR * mMix;

      // Apply output gain
      outL[s] = static_cast<T>(outSampleL * mGain);
      if (outR != outL)
        outR[s] = static_cast<T>(outSampleR * mGain);
    }
  }

  void Reset(double sampleRate, int /*blockSize*/)
  {
    mSampleRate = sampleRate;
    const float sps = static_cast<float>(sampleRate);

    // Initialize Q-based effects
    mDelay.init(sps);
    mReverb.init(sps);

    // Re-apply delay time with new sample rate
    mDelay.setDelayTime(mDelayTimeMs, sampleRate);
  }

  void SetParam(int paramIdx, double value)
  {
    switch (paramIdx)
    {
      case kParamGain:
        mGain = static_cast<T>(value / 100.0);
        break;
      case kParamMix:
        mMix = static_cast<float>(value / 100.0);
        break;
      case kParamDelayTime:
        mDelayTimeMs = static_cast<float>(value);
        mDelay.setDelayTime(mDelayTimeMs, mSampleRate);
        break;
      case kParamDelayFeedback:
        mDelay.setFeedback(static_cast<float>(value / 100.0));
        break;
      case kParamReverbSize:
        mReverb.setRoomSize(static_cast<float>(value / 100.0));
        break;
      case kParamReverbDamping:
        mReverb.setDamping(static_cast<float>(value / 100.0));
        break;
      case kParamReverbWidth:
        mReverb.setWidth(static_cast<float>(value / 100.0));
        break;
      case kParamBypass:
        mBypass = value > 0.5;
        break;
    }
  }

private:
  QStereoDelay mDelay;
  QReverb mReverb;

  double mSampleRate = 44100.0;
  bool mBypass = false;
  T mGain = static_cast<T>(1.0);
  float mMix = 0.5f;
  float mDelayTimeMs = 250.0f;
};
