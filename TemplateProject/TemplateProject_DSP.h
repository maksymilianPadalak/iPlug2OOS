#pragma once

#include <array>
#include <cmath>

// Q DSP Library includes
#include <q/support/literals.hpp>
#include <q/support/pitch.hpp>
#include <q/synth/sin_osc.hpp>
#include <q/synth/envelope_gen.hpp>
#include <q/fx/lowpass.hpp>
#include <q/fx/clip.hpp>

using namespace iplug;
using namespace cycfi::q::literals;

template<typename T>
class TemplateProjectDSP
{
public:
  TemplateProjectDSP() = default;

  void ProcessBlock(T** /*inputs*/, T** outputs, int nOutputs, int nFrames)
  {
    // Polyphony headroom: 1/sqrt(kMaxVoices) prevents clipping when all voices play
    constexpr T kPolyScale = static_cast<T>(1.0 / 2.83); // ~1/sqrt(8)

    for (int s = 0; s < nFrames; ++s)
    {
      // Smooth gain changes using Q one_pole_lowpass
      const float smoothedGain = mGainSmoother(mTargetGain);

      T sample = static_cast<T>(0.0);

      // Process all active voices
      for (auto& voice : mVoices)
      {
        if (!voice.active)
          continue;

        // Get envelope value from Q ADSR
        const float envValue = voice.envelope();

        // Check if envelope finished (voice done)
        if (voice.envelope.in_idle_phase())
        {
          voice.active = false;
          continue;
        }

        // Q sine oscillator
        const float oscValue = cycfi::q::sin(voice.phaseIter);
        ++voice.phaseIter;

        // Apply envelope and velocity
        sample += static_cast<T>(oscValue) * static_cast<T>(envValue) * voice.velocity;
      }

      // Apply gain and polyphony scaling
      sample *= kPolyScale * static_cast<T>(smoothedGain);

      // Q soft_clip to prevent hard digital clipping
      const float clipped = mSoftClip(static_cast<float>(sample));

      for (int ch = 0; ch < nOutputs; ++ch)
      {
        outputs[ch][s] = static_cast<T>(clipped);
      }
    }
  }

  void Reset(double sampleRate, int /*blockSize*/)
  {
    mSampleRate = static_cast<float>(sampleRate);

    for (auto& voice : mVoices)
    {
      voice.Reset(mSampleRate);
    }

    // Initialize Q gain smoother with ~10ms response time (~100Hz cutoff)
    mGainSmoother = cycfi::q::one_pole_lowpass(100_Hz, mSampleRate);
    mGainSmoother = 0.8f; // Initialize to default gain

    mTargetGain = 0.8f;
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

        // Use Q pitch for MIDI note to frequency conversion
        const cycfi::q::pitch notePitch{static_cast<float>(note)};
        const cycfi::q::frequency freq = cycfi::q::as_frequency(notePitch);

        // Attempt to convert velocity to perceptual curve: x^2 for more dynamic range
        const float normalizedVel = static_cast<float>(velocity) / 127.0f;
        const T level = static_cast<T>(normalizedVel * normalizedVel);

        ActivateVoice(note, freq, level);
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
        mTargetGain = static_cast<float>(value / 100.0);
        break;
      default:
        break;
    }
  }

private:
  static constexpr int kMaxVoices = 8;

  // Q ADSR envelope configuration
  static cycfi::q::adsr_envelope_gen::config GetEnvelopeConfig()
  {
    return {
      .attack_rate = 5_ms,      // Fast attack for synth feel
      .decay_rate = 100_ms,     // Medium decay
      .sustain_level = -3_dB,   // Sustain at -3dB
      .sustain_rate = 50_s,     // Very slow sustain decay (essentially hold)
      .release_rate = 200_ms    // Smooth release
    };
  }

  struct Voice
  {
    cycfi::q::phase_iterator phaseIter;
    cycfi::q::adsr_envelope_gen envelope{GetEnvelopeConfig(), 44100.0f};
    int noteNumber = -1;
    T velocity = static_cast<T>(0.0);
    bool active = false;
    float sampleRate = 44100.0f;

    void Reset(float sps)
    {
      phaseIter = cycfi::q::phase_iterator(440_Hz, sps);
      envelope = cycfi::q::adsr_envelope_gen(GetEnvelopeConfig(), sps);
      noteNumber = -1;
      velocity = static_cast<T>(0.0);
      active = false;
      sampleRate = sps;
    }

    void SetFrequency(cycfi::q::frequency freq)
    {
      // Don't reset phase - prevents clicks on note change
      phaseIter.set(freq, sampleRate);
    }

    void TriggerAttack()
    {
      envelope.attack();
    }

    void TriggerRelease()
    {
      envelope.release();
    }
  };

  std::array<Voice, kMaxVoices> mVoices;
  cycfi::q::one_pole_lowpass mGainSmoother{100_Hz, 44100.0f};
  cycfi::q::soft_clip mSoftClip;
  float mTargetGain = 0.8f;
  float mSampleRate = 44100.0f;

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
    // Priority 1: Find an inactive voice
    for (int i = 0; i < kMaxVoices; ++i)
    {
      if (!mVoices[i].active)
        return i;
    }

    // Priority 2: Steal a voice in release phase
    for (int i = 0; i < kMaxVoices; ++i)
    {
      if (mVoices[i].envelope.in_release_phase())
        return i;
    }

    // Priority 3: Steal the voice with lowest envelope level
    int quietestIndex = 0;
    float quietestLevel = mVoices[0].envelope.current();
    for (int i = 1; i < kMaxVoices; ++i)
    {
      const float level = mVoices[i].envelope.current();
      if (level < quietestLevel)
      {
        quietestLevel = level;
        quietestIndex = i;
      }
    }
    return quietestIndex;
  }

  void ActivateVoice(int noteNumber, cycfi::q::frequency freq, T level)
  {
    int voiceIndex = FindVoiceByNote(noteNumber);
    if (voiceIndex < 0)
    {
      voiceIndex = AllocateVoice();
    }

    auto& voice = mVoices[voiceIndex];
    voice.sampleRate = mSampleRate;
    voice.SetFrequency(freq);
    voice.noteNumber = noteNumber;
    voice.velocity = level;
    voice.active = true;
    voice.envelope.reset();
    voice.TriggerAttack();
  }

  void ReleaseVoice(int noteNumber)
  {
    const int voiceIndex = FindVoiceByNote(noteNumber);
    if (voiceIndex < 0)
      return;
    // Trigger ADSR release - envelope will fade out smoothly
    mVoices[voiceIndex].TriggerRelease();
  }
};
