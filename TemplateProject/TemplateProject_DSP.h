#pragma once

#include <cmath>
#include <cstring>

#include "Oscillator.h"

using namespace iplug;

template<typename T>
class TemplateProjectDSP
{
public:
  TemplateProjectDSP() = default;

  void ProcessBlock(T** /*inputs*/, T** outputs, int nOutputs, int nFrames)
  {
    for (int i = 0; i < nOutputs; ++i)
    {
      std::memset(outputs[i], 0, nFrames * sizeof(T));
    }

    if (!mActive)
      return;

    const double frequency = mFrequency;
    const T gain = mGain;
    const T level = mLevel;

    for (int s = 0; s < nFrames; ++s)
    {
      const T sample = static_cast<T>(mOsc.Process(frequency)) * gain * level;

      for (int ch = 0; ch < nOutputs; ++ch)
      {
        outputs[ch][s] += sample;
      }
    }
  }

  void Reset(double sampleRate, int /*blockSize*/)
  {
    mOsc.SetSampleRate(sampleRate);
    mOsc.Reset();

    mGain = static_cast<T>(0.8);
    mLevel = static_cast<T>(1.0);
    mFrequency = 440.0;
    mActive = false;
  }

  void ProcessMidiMsg(const IMidiMsg& msg)
  {
    switch (msg.StatusMsg())
    {
      case IMidiMsg::kNoteOn:
      {
        const int note = msg.NoteNumber();
        const int velocity = msg.Velocity();
        if (velocity == 0)
        {
          mActive = false;
          break;
        }
        mFrequency = 440.0 * std::pow(2.0, (note - 69) / 12.0);
        mLevel = static_cast<T>(velocity / 127.0);
        mActive = true;
        break;
      }
      case IMidiMsg::kNoteOff:
        mActive = false;
        break;
      default:
        break;
    }
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
  FastSinOscillator<T> mOsc;
  T mGain = static_cast<T>(0.8);
  T mLevel = static_cast<T>(1.0);
  double mFrequency = 440.0;
  bool mActive = false;
};
