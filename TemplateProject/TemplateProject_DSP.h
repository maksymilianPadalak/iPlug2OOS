#pragma once

#include <array>
#include <cmath>
#include <cstring>

using namespace iplug;

enum EWaveform
{
  kWaveformSine = 0,
  kWaveformSaw,
  kWaveformSquare,
  kWaveformTriangle,
  kNumWaveforms
};

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
    const int waveform = mWaveform;

    for (int s = 0; s < nFrames; ++s)
    {
      T sample = static_cast<T>(0.0);

      for (auto& voice : mVoices)
      {
        if (!voice.active)
          continue;

        const T oscValue = ProcessOscillator(voice, waveform);
        sample += oscValue * voice.level;
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
      voice.Reset();
    }

    mSampleRate = sampleRate;
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
      case kParamWaveform:
        mWaveform = static_cast<int>(value);
        break;
      default:
        break;
    }
  }

private:
  static constexpr int kMaxVoices = 8;

  struct Voice
  {
    double phase = 0.0;
    double frequency = 0.0;
    int noteNumber = -1;
    T level = static_cast<T>(0.0);
    bool active = false;

    void Reset()
    {
      phase = 0.0;
      frequency = 0.0;
      noteNumber = -1;
      level = static_cast<T>(0.0);
      active = false;
    }
  };

  T ProcessOscillator(Voice& voice, int waveform)
  {
    const double phaseIncr = voice.frequency / mSampleRate;
    voice.phase += phaseIncr;
    if (voice.phase >= 1.0)
      voice.phase -= 1.0;

    const double phase = voice.phase;
    T output = static_cast<T>(0.0);

    switch (waveform)
    {
      case kWaveformSine:
        output = static_cast<T>(std::sin(phase * 2.0 * 3.14159265358979323846));
        break;
      case kWaveformSaw:
        output = static_cast<T>(2.0 * phase - 1.0);
        break;
      case kWaveformSquare:
        output = static_cast<T>(phase < 0.5 ? 1.0 : -1.0);
        break;
      case kWaveformTriangle:
        output = static_cast<T>(phase < 0.5 ? (4.0 * phase - 1.0) : (3.0 - 4.0 * phase));
        break;
      default:
        break;
    }

    return output;
  }

  std::array<Voice, kMaxVoices> mVoices;
  T mGain = static_cast<T>(0.8);
  int mWaveform = kWaveformSine;
  double mSampleRate = 44100.0;
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
    voice.phase = 0.0;
    voice.frequency = frequency;
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
