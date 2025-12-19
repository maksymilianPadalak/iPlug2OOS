#pragma once

#include <array>
#include <cstring>

// Q DSP Library
#include <q/support/literals.hpp>
#include <q/support/phase.hpp>
#include <q/synth/sin_osc.hpp>
#include <q/synth/saw_osc.hpp>
#include <q/synth/square_osc.hpp>
#include <q/synth/triangle_osc.hpp>
#include <q/synth/envelope_gen.hpp>

using namespace iplug;
namespace q = cycfi::q;
using namespace q::literals;

enum EWaveform
{
  kWaveformSine = 0,
  kWaveformSaw,
  kWaveformSquare,
  kWaveformTriangle,
  kNumWaveforms
};

template<typename T>
class PluginInstanceDSP
{
public:
  PluginInstanceDSP() = default;

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

        // Get envelope amplitude (clamp to prevent overshoot from exponential curves)
        float envAmp = std::min(voice.env(), 1.0f);

        // Check if voice should be deactivated (envelope finished)
        if (voice.releasing && voice.env.in_idle_phase())
        {
          voice.active = false;
          continue;
        }

        // Generate oscillator sample using Q library
        float oscValue = 0.0f;
        switch (waveform)
        {
          case kWaveformSine:
            oscValue = q::sin(voice.phase++);
            break;
          case kWaveformSaw:
            oscValue = q::saw(voice.phase++);
            break;
          case kWaveformSquare:
            oscValue = q::square(voice.phase++);
            break;
          case kWaveformTriangle:
            oscValue = q::triangle(voice.phase++);
            break;
          default:
            oscValue = q::sin(voice.phase++);
            break;
        }

        sample += static_cast<T>(oscValue * envAmp * voice.velocity);
      }

      // Scale down for polyphony (1/sqrt(maxVoices) preserves perceived loudness)
      constexpr T kPolyScale = static_cast<T>(1.0 / 2.83); // ~1/sqrt(8)
      sample *= gain * kPolyScale;

      // Hard clip to prevent any remaining clipping (safety measure)
      sample = std::max(static_cast<T>(-1.0), std::min(sample, static_cast<T>(1.0)));

      for (int ch = 0; ch < nOutputs; ++ch)
      {
        outputs[ch][s] += sample;
      }
    }
  }

  void Reset(double sampleRate, int /*blockSize*/)
  {
    mSampleRate = static_cast<float>(sampleRate);

    // Create ADSR config
    mEnvConfig = q::adsr_envelope_gen::config{
      10_ms,      // Attack - fast for synth feel
      100_ms,     // Decay
      -6_dB,      // Sustain level
      50_s,       // Sustain rate (very slow decay during sustain)
      200_ms      // Release
    };

    for (auto& voice : mVoices)
    {
      voice.Reset(mEnvConfig, mSampleRate);
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

        const float frequency = static_cast<float>(440.0 * std::pow(2.0, (note - 69) / 12.0));
        const float level = static_cast<float>(velocity) / 127.0f;
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
      case kParamAttack:
        mEnvConfig.attack_rate = q::duration{value * 0.001}; // ms to seconds
        UpdateEnvelopeConfig();
        break;
      case kParamDecay:
        mEnvConfig.decay_rate = q::duration{value * 0.001};
        UpdateEnvelopeConfig();
        break;
      case kParamSustain:
        // Convert percentage to decibels (0% = -inf, 100% = 0dB)
        // Using a simple mapping: sustain% -> dB
        {
          double db = (value <= 0.0) ? -96.0 : 20.0 * std::log10(value / 100.0);
          mEnvConfig.sustain_level = q::decibel{db, q::direct_unit};
        }
        UpdateEnvelopeConfig();
        break;
      case kParamRelease:
        mEnvConfig.release_rate = q::duration{value * 0.001};
        UpdateEnvelopeConfig();
        break;
      default:
        break;
    }
  }

private:
  static constexpr int kMaxVoices = 8;

  struct Voice
  {
    q::phase_iterator phase;
    q::adsr_envelope_gen env{q::adsr_envelope_gen::config{}, 44100.0f};
    int noteNumber = -1;
    float velocity = 0.0f;
    bool active = false;
    bool releasing = false;

    void Reset(q::adsr_envelope_gen::config const& config, float sps)
    {
      phase = q::phase_iterator{};
      env = q::adsr_envelope_gen{config, sps};
      noteNumber = -1;
      velocity = 0.0f;
      active = false;
      releasing = false;
    }

    void Activate(int note, q::frequency freq, float vel, float sps)
    {
      phase = q::phase_iterator{freq, sps};
      noteNumber = note;
      velocity = vel;
      active = true;
      releasing = false;
      env.attack();
    }

    void Release()
    {
      releasing = true;
      env.release();
    }
  };

  std::array<Voice, kMaxVoices> mVoices;
  q::adsr_envelope_gen::config mEnvConfig{};
  T mGain = static_cast<T>(0.8);
  int mWaveform = kWaveformSine;
  float mSampleRate = 44100.0f;
  int mNextVoice = 0;

  void UpdateEnvelopeConfig()
  {
    // Update envelope config for all inactive voices
    // Active voices keep their current envelope until they're recycled
    for (auto& voice : mVoices)
    {
      if (!voice.active)
      {
        voice.env = q::adsr_envelope_gen{mEnvConfig, mSampleRate};
      }
    }
  }

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
    // First, try to find an inactive voice
    for (int i = 0; i < kMaxVoices; ++i)
    {
      if (!mVoices[i].active)
        return i;
    }

    // Then, try to find a releasing voice (steal it)
    for (int i = 0; i < kMaxVoices; ++i)
    {
      if (mVoices[i].releasing)
        return i;
    }

    // Finally, steal the oldest voice
    const int stolenIndex = mNextVoice;
    mNextVoice = (mNextVoice + 1) % kMaxVoices;
    return stolenIndex;
  }

  void ActivateVoice(int noteNumber, float frequency, float level)
  {
    int voiceIndex = FindVoiceByNote(noteNumber);

    if (voiceIndex < 0)
    {
      voiceIndex = AllocateVoice();
    }

    auto& voice = mVoices[voiceIndex];

    // Re-initialize envelope for this voice
    voice.env = q::adsr_envelope_gen{mEnvConfig, mSampleRate};
    voice.Activate(noteNumber, q::frequency{static_cast<double>(frequency)}, level, mSampleRate);
  }

  void ReleaseVoice(int noteNumber)
  {
    const int voiceIndex = FindVoiceByNote(noteNumber);

    if (voiceIndex < 0)
      return;

    mVoices[voiceIndex].Release();
  }
};
