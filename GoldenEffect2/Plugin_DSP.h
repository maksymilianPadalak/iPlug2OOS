#pragma once

/**
 * =============================================================================
 * SIMPLE GAIN CONTROLLER - Minimal Effect Template
 * =============================================================================
 *
 * This is a minimal DSP implementation for a gain controller.
 * Use this as a starting point for building more complex effects.
 *
 * SIGNAL FLOW:
 * -----------
 *   Input → Gain (dB to linear) → Output
 *
 * =============================================================================
 */

#include <cmath>
#include <algorithm>

using namespace iplug;

// =============================================================================
// MAIN DSP CLASS
// =============================================================================
template<typename T>
class PluginInstanceDSP
{
public:
  PluginInstanceDSP() = default;

  /**
   * Process a block of audio samples.
   * Applies gain to input signal.
   */
  void ProcessBlock(T** inputs, T** outputs, int nInputs, int nOutputs, int nFrames)
  {
    if (!inputs || !outputs || nOutputs < 1 || nInputs < 1)
      return;

    T* inL = inputs[0];
    T* inR = (nInputs > 1 && inputs[1]) ? inputs[1] : inputs[0];

    for (int s = 0; s < nFrames; ++s)
    {
      // Apply gain
      outputs[0][s] = static_cast<T>(inL[s] * mGainLinear);
      if (nOutputs > 1)
        outputs[1][s] = static_cast<T>(inR[s] * mGainLinear);
    }
  }

  /**
   * Reset DSP state. Called when sample rate changes.
   */
  void Reset(double sampleRate, int /*blockSize*/)
  {
    mSampleRate = static_cast<float>(sampleRate);
  }

  /**
   * Handle parameter changes from the UI.
   */
  void SetParam(int paramIdx, double value)
  {
    switch (paramIdx)
    {
      case kParamGain:
        // Convert dB to linear gain
        // Formula: linear = 10^(dB/20)
        mGainLinear = static_cast<float>(std::pow(10.0, value / 20.0));
        break;

      default:
        break;
    }
  }

private:
  float mSampleRate = 44100.0f;
  float mGainLinear = 1.0f;  // Linear gain (1.0 = 0 dB)
};
