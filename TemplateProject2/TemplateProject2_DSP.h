#pragma once

#include "Smoothers.h"

using namespace iplug;

enum EModulations
{
  kModGainSmoother = 0,
  kModDelayTimeSmoother = 1,
  kModDelayFeedbackSmoother = 2,
  kModDelayDrySmoother = 3,
  kModDelayWetSmoother = 4,
  kNumModulations,
};

template<typename T>
class TemplateProject2DSP
{
public:
  TemplateProject2DSP()
  {
    // Delay buffers will be initialized in Reset() with proper sample rate
  }

  void ProcessBlock(T** inputs, T** outputs, int nOutputs, int nFrames)
  {
    // Process parameter smoothers
    mParamSmoother.ProcessBlock(mParamsToSmooth, mModulations.GetList(), nFrames);

    // Update delay line size based on current delay time
    // Use the first sample's delay time for the entire block (delay time changes smoothly)
    T currentDelayTime = mModulations.GetList()[kModDelayTimeSmoother][0];
    int delaySamples = static_cast<int>(currentDelayTime * mSampleRate / 1000.0); // Convert ms to samples
    delaySamples = std::max(1, std::min(delaySamples, static_cast<int>(mSampleRate * 2))); // Clamp to 1 sample - 2 seconds
    
    // Update delay buffer size if delay time changed
    if (delaySamples != mCurrentDelaySamples)
    {
      // Resize buffers and clear them
      mDelayBufferL.Resize(delaySamples);
      mDelayBufferR.Resize(delaySamples);
      memset(mDelayBufferL.Get(), 0, delaySamples * sizeof(T));
      memset(mDelayBufferR.Get(), 0, delaySamples * sizeof(T));
      mDelayWriteAddressL = 0;
      mDelayWriteAddressR = 0;
      mCurrentDelaySamples = delaySamples;
    }

    // Process audio sample-by-sample to implement delay with feedback
    T* delayBufferL = mDelayBufferL.Get();
    T* delayBufferR = mDelayBufferR.Get();
    
    for(int s = 0; s < nFrames; s++)
    {
      T smoothedGain = mModulations.GetList()[kModGainSmoother][s];
      T smoothedFeedback = mModulations.GetList()[kModDelayFeedbackSmoother][s];
      T smoothedDry = mModulations.GetList()[kModDelayDrySmoother][s];
      T smoothedWet = mModulations.GetList()[kModDelayWetSmoother][s];
      
      T inputL = 0.0;
      T inputR = 0.0;
      
      // Get input signal
      if(nOutputs >= 2 && inputs && inputs[0] && inputs[1])
      {
        inputL = inputs[0][s];
        inputR = inputs[1][s];
      }
      else if(nOutputs >= 1 && inputs && inputs[0])
      {
        inputL = inputs[0][s];
        inputR = inputs[0][s]; // Mono to stereo
      }
      
      // Apply gain
      T gainedL = inputL * smoothedGain;
      T gainedR = inputR * smoothedGain;
      
      // Get delayed signal from delay buffer
      T delayedL = 0.0;
      T delayedR = 0.0;
      
      if (mCurrentDelaySamples > 0 && delayBufferL && delayBufferR)
      {
        // Read delayed sample from buffer (this is what was written delaySamples ago)
        delayedL = delayBufferL[mDelayWriteAddressL];
        delayedR = delayBufferR[mDelayWriteAddressR];
        
        // Calculate delay input: current input + feedback from delayed signal
        T delayInputL = gainedL + delayedL * smoothedFeedback;
        T delayInputR = gainedR + delayedR * smoothedFeedback;
        
        // Write to delay buffer (will be read delaySamples later)
        delayBufferL[mDelayWriteAddressL] = delayInputL;
        delayBufferR[mDelayWriteAddressR] = delayInputR;
        
        // Increment and wrap write addresses
        mDelayWriteAddressL++;
        mDelayWriteAddressL %= mCurrentDelaySamples;
        mDelayWriteAddressR++;
        mDelayWriteAddressR %= mCurrentDelaySamples;
      }
      
      // Mix dry and wet signals separately
      T dryL = gainedL * smoothedDry;
      T wetL = delayedL * smoothedWet;
      outputs[0][s] = dryL + wetL;
      
      if(nOutputs >= 2)
      {
        T dryR = gainedR * smoothedDry;
        T wetR = delayedR * smoothedWet;
        outputs[1][s] = dryR + wetR;
      }
    }
  }

  void Reset(double sampleRate, int blockSize)
  {
    mModulationsData.Resize(blockSize * kNumModulations);
    mModulations.Empty();

    for(int i = 0; i < kNumModulations; i++)
    {
      mModulations.Add(mModulationsData.Get() + (blockSize * i));
    }

    mSampleRate = sampleRate;
    mBlockSize = blockSize;

    // Initialize parameter smoothers
    mParamsToSmooth[kModGainSmoother] = static_cast<T>(1.0); // 100% gain by default
    mParamsToSmooth[kModDelayTimeSmoother] = static_cast<T>(250.0); // 250ms delay by default
    mParamsToSmooth[kModDelayFeedbackSmoother] = static_cast<T>(0.3); // 30% feedback by default
    mParamsToSmooth[kModDelayDrySmoother] = static_cast<T>(0.5); // 50% dry by default
    mParamsToSmooth[kModDelayWetSmoother] = static_cast<T>(0.5); // 50% wet by default
    
    // Reset delay buffers (250ms default delay)
    int defaultDelaySamples = static_cast<int>(sampleRate * 0.25);
    mDelayBufferL.Resize(defaultDelaySamples);
    mDelayBufferR.Resize(defaultDelaySamples);
    memset(mDelayBufferL.Get(), 0, defaultDelaySamples * sizeof(T));
    memset(mDelayBufferR.Get(), 0, defaultDelaySamples * sizeof(T));
    mDelayWriteAddressL = 0;
    mDelayWriteAddressR = 0;
    mCurrentDelaySamples = defaultDelaySamples;
  }

  void SetParam(int paramIdx, double value)
  {
    switch (paramIdx) {
      case kParamGain:
        mParamsToSmooth[kModGainSmoother] = (T) value / 100.; // Convert percentage to 0-1 range
        break;
      case kParamDelayTime:
        mParamsToSmooth[kModDelayTimeSmoother] = (T) value; // Delay time in milliseconds
        break;
      case kParamDelayFeedback:
        mParamsToSmooth[kModDelayFeedbackSmoother] = (T) value / 100.; // Convert percentage to 0-1 range
        break;
      case kParamDelayDry:
        mParamsToSmooth[kModDelayDrySmoother] = (T) value / 100.; // Convert percentage to 0-1 range
        break;
      case kParamDelayWet:
        mParamsToSmooth[kModDelayWetSmoother] = (T) value / 100.; // Convert percentage to 0-1 range
        break;
      default:
        break;
    }
  }
  
public:
  WDL_TypedBuf<T> mModulationsData; // Sample data for global modulations
  WDL_PtrList<T> mModulations; // Ptrlist for global modulations
  LogParamSmooth<T, kNumModulations> mParamSmoother;
  sample mParamsToSmooth[kNumModulations];

  // Simple delay line implementation based on NChanDelayLine but with direct buffer access for feedback
  WDL_TypedBuf<T> mDelayBufferL; // Left channel delay buffer
  WDL_TypedBuf<T> mDelayBufferR; // Right channel delay buffer
  uint32_t mDelayWriteAddressL = 0; // Write address for left channel
  uint32_t mDelayWriteAddressR = 0; // Write address for right channel
  int mCurrentDelaySamples = 0; // Current delay length in samples

  double mSampleRate = 44100.0;
  int mBlockSize = 64;
};
