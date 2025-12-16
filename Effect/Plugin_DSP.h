#pragma once

#include <q/support/literals.hpp>
#include <q/fx/lowpass.hpp>

using namespace iplug;
using namespace cycfi::q::literals;

template<typename T>
class PluginInstanceDSP
{
public:
  PluginInstanceDSP() = default;

  void ProcessBlock(T** inputs, T** outputs, int nInputs, int nOutputs, int nFrames)
  {
    if (!inputs || !outputs || nOutputs < 1 || nInputs < 1 || !inputs[0])
      return;

    T* inL = inputs[0];
    T* inR = (nInputs > 1 && inputs[1]) ? inputs[1] : inputs[0];

    for (int s = 0; s < nFrames; ++s)
    {
      const float gain = mGainSmoother(mTargetGain);

      outputs[0][s] = inL[s] * static_cast<T>(gain);
      if (nOutputs > 1)
        outputs[1][s] = inR[s] * static_cast<T>(gain);
    }
  }

  void Reset(double sampleRate, int /*blockSize*/)
  {
    mGainSmoother = cycfi::q::one_pole_lowpass(100_Hz, static_cast<float>(sampleRate));
    mGainSmoother(mTargetGain);
  }

  void SetParam(int paramIdx, double value)
  {
    if (paramIdx == kParamGain)
      mTargetGain = static_cast<float>(value / 100.0);
  }

private:
  float mTargetGain = 1.0f;
  cycfi::q::one_pole_lowpass mGainSmoother{100_Hz, 44100.0f};
};
