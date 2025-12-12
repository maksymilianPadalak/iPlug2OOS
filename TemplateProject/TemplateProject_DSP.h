#pragma once

#include <array>
#include <cmath>

// Q DSP Library includes
#include <q/support/literals.hpp>
#include <q/synth/sin_osc.hpp>

using namespace iplug;
using namespace cycfi::q::literals;

template<typename T>
class TemplateProjectDSP
{
public:
  TemplateProjectDSP() = default;

  void ProcessBlock(T** /*inputs*/, T** outputs, int nOutputs, int nFrames)
  {
    const T gain = mGain;

    for (int s = 0; s < nFrames; ++s)
    {
      T sample = static_cast<T>(0.0);

      // Process all active voices
      for (auto& voice : mVoices)
      {
        if (!voice.active)
          continue;

        // Q sine oscillator
        const float oscValue = cycfi::q::sin(voice.phaseIter);
        ++voice.phaseIter;
        sample += static_cast<T>(oscValue) * voice.level;
      }

      // Scale for polyphony
      constexpr T kPolyScale = static_cast<T>(1.0 / 2.83);
      sample *= kPolyScale * gain;

      for (int ch = 0; ch < nOutputs; ++ch)
      {
        outputs[ch][s] = sample;
      }
    }
  }

  void Reset(double sampleRate, int /*blockSize*/)
  {
    mSampleRate = sampleRate;
    const float sps = static_cast<float>(sampleRate);

    for (auto& voice : mVoices)
    {
      voice.Reset(sps);
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
    cycfi::q::phase_iterator phaseIter;
    int noteNumber = -1;
    T level = static_cast<T>(0.0);
    bool active = false;
    float sampleRate = 44100.0f;

    void Reset(float sps)
    {
      phaseIter = cycfi::q::phase_iterator(440_Hz, sps);
      noteNumber = -1;
      level = static_cast<T>(0.0);
      active = false;
      sampleRate = sps;
    }

    void SetFrequency(double freq)
    {
      phaseIter.set(cycfi::q::frequency{freq}, sampleRate);
      phaseIter._phase = cycfi::q::phase::begin();
    }
  };

  std::array<Voice, kMaxVoices> mVoices;
  T mGain = static_cast<T>(0.8);
  double mSampleRate = 44100.0;
  int mNextVoice = 0;

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
    voice.sampleRate = static_cast<float>(mSampleRate);
    voice.SetFrequency(frequency);
    voice.noteNumber = noteNumber;
    voice.level = level;
    voice.active = true;
  }

  void ReleaseVoice(int noteNumber)
  {
    const int voiceIndex = FindVoiceByNote(noteNumber);
    if (voiceIndex < 0)
      return;
    mVoices[voiceIndex].active = false;
  }
};
