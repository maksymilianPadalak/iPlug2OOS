#pragma once

#include <array>
#include <cmath>
#include <cstring>

#include "ADSREnvelope.h"
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

    if (!HasActiveVoices())
      return;

    const T gain = mGain;
    const T sustainLevel = mSustain;

    for (int s = 0; s < nFrames; ++s)
    {
      T sample = static_cast<T>(0.0);

      for (auto& voice : mVoices)
      {
        if (!voice.env.GetBusy())
          continue;

        const T envValue = voice.env.Process(sustainLevel);
        const T oscValue = static_cast<T>(voice.osc.Process(voice.frequency));
        sample += oscValue * envValue;
      }

      // Scale down for polyphony (1/sqrt(maxVoices) preserves perceived loudness)
      constexpr T kPolyScale = static_cast<T>(1.0 / 2.83); // ~1/sqrt(8)
      sample *= gain * kPolyScale;

      for (int ch = 0; ch < nOutputs; ++ch)
      {
        outputs[ch][s] += sample;
      }
    }
  }

  void Reset(double sampleRate, int /*blockSize*/)
  {
    for (auto& voice : mVoices)
    {
      voice.Reset(sampleRate);
    }

    mSampleRate = sampleRate;
    mGain = static_cast<T>(0.8);
    mNextVoice = 0;

    UpdateEnvelopeTimes();
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
          ReleaseVoice(note);
          break;
        }

        const double frequency = 440.0 * std::pow(2.0, (note - 69) / 12.0);
        const T level = static_cast<T>(static_cast<double>(velocity) / 127.0);
        ActivateVoice(note, frequency, level);
        break;
      }
      case IMidiMsg::kNoteOff:
        ReleaseVoice(msg.NoteNumber());
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
      case kParamAttack:
        mAttackMs = value;
        UpdateEnvelopeTimes();
        break;
      case kParamDecay:
        mDecayMs = value;
        UpdateEnvelopeTimes();
        break;
      case kParamSustain:
        mSustain = static_cast<T>(value / 100.0);
        break;
      case kParamRelease:
        mReleaseMs = value;
        UpdateEnvelopeTimes();
        break;
      default:
        break;
    }
  }

private:
  static constexpr int kMaxVoices = 8;

  struct Voice
  {
    ADSREnvelope<T> env;
    FastSinOscillator<T> osc;
    double frequency = 0.0;
    int noteNumber = -1;

    void Reset(double sampleRate)
    {
      env.SetSampleRate(sampleRate);
      osc.SetSampleRate(sampleRate);
      osc.Reset();
      frequency = 0.0;
      noteNumber = -1;
    }
  };

  std::array<Voice, kMaxVoices> mVoices;
  T mGain = static_cast<T>(0.8);
  T mSustain = static_cast<T>(0.7);
  double mAttackMs = 10.0;
  double mDecayMs = 100.0;
  double mReleaseMs = 200.0;
  double mSampleRate = 44100.0;
  int mNextVoice = 0;

  void UpdateEnvelopeTimes()
  {
    for (auto& voice : mVoices)
    {
      voice.env.SetStageTime(ADSREnvelope<T>::kAttack, mAttackMs);
      voice.env.SetStageTime(ADSREnvelope<T>::kDecay, mDecayMs);
      voice.env.SetStageTime(ADSREnvelope<T>::kRelease, mReleaseMs);
    }
  }

  bool HasActiveVoices() const
  {
    for (const auto& voice : mVoices)
    {
      if (voice.env.GetBusy())
        return true;
    }

    return false;
  }

  int FindVoiceByNote(int noteNumber) const
  {
    for (int i = 0; i < kMaxVoices; ++i)
    {
      if (mVoices[i].env.GetBusy() && mVoices[i].noteNumber == noteNumber)
        return i;
    }

    return -1;
  }

  int AllocateVoice()
  {
    for (int i = 0; i < kMaxVoices; ++i)
    {
      if (!mVoices[i].env.GetBusy())
        return i;
    }

    const int stolenIndex = mNextVoice;
    mNextVoice = (mNextVoice + 1) % kMaxVoices;
    return stolenIndex;
  }

  void ActivateVoice(int noteNumber, double frequency, T level)
  {
    int voiceIndex = FindVoiceByNote(noteNumber);

    if (voiceIndex < 0)
    {
      voiceIndex = AllocateVoice();
    }

    auto& voice = mVoices[voiceIndex];

    if (voice.env.GetBusy())
    {
      voice.env.Retrigger(level);
    }
    else
    {
      voice.env.Start(level);
    }

    voice.frequency = frequency;
    voice.noteNumber = noteNumber;
    voice.osc.Reset();
  }

  void ReleaseVoice(int noteNumber)
  {
    const int voiceIndex = FindVoiceByNote(noteNumber);

    if (voiceIndex < 0)
      return;

    mVoices[voiceIndex].env.Release();
  }
};
