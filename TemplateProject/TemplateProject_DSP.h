#pragma once

#include <cmath>
#include <cstring>

#include "MidiSynth.h"
#include "Oscillator.h"

using namespace iplug;

template<typename T>
class TemplateProjectDSP
{
public:
  class Voice : public SynthVoice
  {
  public:
    Voice() = default;

    bool GetBusy() const override
    {
      return mActive;
    }

    void Trigger(double level, bool /*isRetrigger*/) override
    {
      mOSC.Reset();
      mLevel = static_cast<T>(level);
      mActive = true;
    }

    void Release() override
    {
      mActive = false;
    }

    void ProcessSamplesAccumulating(T** /*inputs*/, T** outputs, int /*nInputs*/, int nOutputs, int startIdx, int nFrames) override
    {
      if (!mActive)
        return;

      const double pitch = mInputs[kVoiceControlPitch].endValue;
      const double pitchBend = mInputs[kVoiceControlPitchBend].endValue;
      const double baseFreq = 440. * std::pow(2., pitch + pitchBend);

      for (int i = startIdx; i < startIdx + nFrames; ++i)
      {
        const T sample = mOSC.Process(baseFreq) * mLevel;

        for (int ch = 0; ch < nOutputs; ++ch)
        {
          outputs[ch][i] += sample;
        }
      }
    }

    void SetSampleRateAndBlockSize(double sampleRate, int /*blockSize*/) override
    {
      mOSC.SetSampleRate(sampleRate);
    }

    void SetProgramNumber(int) override {}
    void SetControl(int, float) override {}

  private:
    FastSinOscillator<T> mOSC;
    T mLevel = static_cast<T>(1.0);
    bool mActive = false;
  };

public:
  TemplateProjectDSP(int nVoices)
  {
    for (int i = 0; i < nVoices; ++i)
    {
      mSynth.AddVoice(new Voice(), 0);
    }
  }

  void ProcessBlock(T** /*inputs*/, T** outputs, int nOutputs, int nFrames, double /*qnPos*/ = 0., bool /*transportIsRunning*/ = false, double /*tempo*/ = 120.)
  {
    for (int i = 0; i < nOutputs; ++i)
    {
      std::memset(outputs[i], 0, nFrames * sizeof(T));
    }

    mSynth.ProcessBlock(nullptr, outputs, 0, nOutputs, nFrames);

    const T gain = mGain;

    for (int s = 0; s < nFrames; ++s)
    {
      for (int ch = 0; ch < nOutputs; ++ch)
      {
        outputs[ch][s] *= gain;
      }
    }
  }

  void Reset(double sampleRate, int blockSize)
  {
    mSynth.SetSampleRateAndBlockSize(sampleRate, blockSize);
    mSynth.Reset();

    mGain = static_cast<T>(0.8);
  }

  void ProcessMidiMsg(const IMidiMsg& msg)
  {
    mSynth.AddMidiMsgToQueue(msg);
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
  MidiSynth mSynth { VoiceAllocator::kPolyModePoly, MidiSynth::kDefaultBlockSize };
  T mGain = static_cast<T>(0.8);
};
