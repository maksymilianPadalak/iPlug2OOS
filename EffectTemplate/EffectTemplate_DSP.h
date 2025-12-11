#pragma once

#include <cmath>

using namespace iplug;

template<typename T>
class EffectTemplateDSP
{
public:
  EffectTemplateDSP() = default;

  void ProcessBlock(T** inputs, T** outputs, int nInputs, int nOutputs, int nFrames)
  {
    if (!inputs || !outputs || nOutputs < 1 || nInputs < 1)
      return;

    const T gain = mGain;

    for (int ch = 0; ch < nOutputs; ++ch)
    {
      const T* in = (ch < nInputs && inputs[ch]) ? inputs[ch] : inputs[0];
      T* out = outputs[ch];

      if (!in || !out)
        continue;

      for (int s = 0; s < nFrames; ++s)
      {
        out[s] = in[s] * gain;
      }
    }
  }

  void Reset(double sampleRate, int /*blockSize*/)
  {
    mSampleRate = sampleRate;
  }

  void SetParam(int paramIdx, double value)
  {
    switch (paramIdx)
    {
      case kParamGain:
        mGain = static_cast<T>(value / 100.0);
        break;
      default:
        break;
    }
  }

private:
  T mGain = static_cast<T>(1.0);
  double mSampleRate = 44100.0;
};
