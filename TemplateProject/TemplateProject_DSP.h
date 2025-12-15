#pragma once

#include <array>
#include <cmath>

// Q DSP Library includes
#include <q/support/literals.hpp>
#include <q/fx/delay.hpp>

using namespace iplug;
using namespace cycfi::q::literals;

template<typename T>
class TemplateProjectDSP
{
public:
  TemplateProjectDSP()
    : mDelayL{2_s, 44100.0f}
    , mDelayR{2_s, 44100.0f}
  {
  }

          void ProcessBlock(T** inputs, T** outputs, int nOutputs, int nFrames)
  {
    const bool stereo = (nOutputs > 1);
    
    for (int s = 0; s < nFrames; ++s)
    {
      // Get input samples
      const float inputL = static_cast<float>(inputs[0][s]);
      const float inputR = stereo ? static_cast<float>(inputs[1][s]) : inputL;
      
      // Read delayed samples from delay lines
      const float delayedL = mDelayL[mDelayTimeSamples];
      const float delayedR = mDelayR[mDelayTimeSamples];
      
      // Calculate wet signal with feedback (add DC offset to prevent denormals)
      const float wetL = inputL + delayedL * mFeedback + 1e-15f;
      const float wetR = inputR + delayedR * mFeedback + 1e-15f;
      
      // Push wet signal to delay lines
      mDelayL.push(wetL);
      mDelayR.push(wetR);
      
      // Mix dry and wet signals
      outputs[0][s] = static_cast<T>(inputL * (1.0f - mMix) + delayedL * mMix);
      if (stereo)
      {
        outputs[1][s] = static_cast<T>(inputR * (1.0f - mMix) + delayedR * mMix);
      }
    }
  }

        void Reset(double sampleRate, int /*blockSize*/)
  {
    mSampleRate = static_cast<float>(sampleRate);
    
    // Recreate delay lines with new sample rate
    mDelayL = cycfi::q::delay(2_s, mSampleRate);
    mDelayR = cycfi::q::delay(2_s, mSampleRate);
    
    // Recalculate delay time in samples from stored ms value
    mDelayTimeSamples = (mDelayTimeMs / 1000.0f) * mSampleRate;
  }

        void SetParam(int paramIdx, double value)
  {
    switch (paramIdx)
    {
      case kParamDelayTime:
        mDelayTimeMs = static_cast<float>(value);
        mDelayTimeSamples = (mDelayTimeMs / 1000.0f) * mSampleRate;
        break;
      case kParamDelayFeedback:
        mFeedback = static_cast<float>(value) / 100.0f;
        break;
      case kParamDelayMix:
        mMix = static_cast<float>(value) / 100.0f;
        break;
      default:
        break;
    }
  }

private:
  float mSampleRate = 44100.0f;
  cycfi::q::delay mDelayL{2_s, 44100.0f};
  cycfi::q::delay mDelayR{2_s, 44100.0f};
  float mDelayTimeMs = 300.0f;         // Store delay time in ms for recalculation
  float mDelayTimeSamples = 13230.0f;  // 300ms * 44100
  float mFeedback = 0.4f;              // 40% default
  float mMix = 0.5f;                   // 50% default
};
