#pragma once

#include <array>
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

    if (!HasActiveVoices())
      return;

    const T gain = mGain;

    for (int s = 0; s < nFrames; ++s)
    {
      T sample = static_cast<T>(0.0);

      for (auto& voice : mVoices)
      {
        if (!voice.active)
          continue;

        sample += static_cast<T>(voice.osc.Process(voice.frequency)) * voice.level;
      }

      sample *= gain;

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

    mGain = static_cast<T>(0.8);
    mNextVoice = 0;
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
      default:
        break;
    }
  }

private:
  static constexpr int kMaxVoices = 8;

  struct Voice
  {
    FastSinOscillator<T> osc;
    T level = static_cast<T>(0.0);
    double frequency = 0.0;
    bool active = false;
    int noteNumber = -1;

    void Reset(double sampleRate)
    {
      osc.SetSampleRate(sampleRate);
      osc.Reset();
      level = static_cast<T>(0.0);
      frequency = 0.0;
      active = false;
      noteNumber = -1;
    }
  };

  std::array<Voice, kMaxVoices> mVoices;
  T mGain = static_cast<T>(0.8);
  int mNextVoice = 0;

  bool HasActiveVoices() const
  {
    for (const auto& voice : mVoices)
    {
      if (voice.active)
        return true;
    }

    return false;
  }

  int FindVoiceByNote(int noteNumber) const
  {
    for (int i = 0; i < kMaxVoices; ++i)
    {
      if (mVoices[i].active && mVoices[i].noteNumber == noteNumber)
        return i;
    }

    return -1;
  }

  int AllocateVoice()
  {
    for (int i = 0; i < kMaxVoices; ++i)
    {
      if (!mVoices[i].active)
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
    voice.frequency = frequency;
    voice.level = level;
    voice.noteNumber = noteNumber;
    voice.active = true;
    voice.osc.Reset();
  }

  void ReleaseVoice(int noteNumber)
  {
    const int voiceIndex = FindVoiceByNote(noteNumber);

    if (voiceIndex < 0)
      return;

    auto& voice = mVoices[voiceIndex];
    voice.active = false;
    voice.level = static_cast<T>(0.0);
    voice.noteNumber = -1;
  }
};
