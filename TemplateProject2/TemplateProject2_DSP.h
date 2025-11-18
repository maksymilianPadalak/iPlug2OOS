#pragma once

#include "Smoothers.h"

using namespace iplug;

enum EModulations
{
  kModGainSmoother = 0,
  kNumModulations,
};

template<typename T>
class TemplateProject2DSP
{
public:
  TemplateProject2DSP()
  {
  }

  void ProcessBlock(T** inputs, T** outputs, int nOutputs, int nFrames)
  {
    // Process parameter smoothers
    mParamSmoother.ProcessBlock(mParamsToSmooth, mModulations.GetList(), nFrames);

    // Process audio: apply gain to input and copy to output
    for(int s = 0; s < nFrames; s++)
    {
      T smoothedGain = mModulations.GetList()[kModGainSmoother][s];
      
      // Process stereo channels
      if(nOutputs >= 2 && inputs && inputs[0] && inputs[1])
      {
        outputs[0][s] = inputs[0][s] * smoothedGain;
        outputs[1][s] = inputs[1][s] * smoothedGain;
      }
      else if(nOutputs >= 1 && inputs && inputs[0])
      {
        outputs[0][s] = inputs[0][s] * smoothedGain;
        if(nOutputs >= 2)
        {
          outputs[1][s] = inputs[0][s] * smoothedGain; // Mono to stereo
        }
      }
      else
      {
        // No input - output silence
        outputs[0][s] = 0.0;
        if(nOutputs >= 2)
        {
          outputs[1][s] = 0.0;
        }
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
  }

  void SetParam(int paramIdx, double value)
  {
    switch (paramIdx) {
      case kParamGain:
        mParamsToSmooth[kModGainSmoother] = (T) value / 100.; // Convert percentage to 0-1 range
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

  double mSampleRate = 44100.0;
  int mBlockSize = 64;
};
