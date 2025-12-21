#pragma once

#include <cmath>
#include <cstring>

// iPlug2 MIDI Infrastructure
#include "IPlugMidi.h"

// Q DSP Library
#include <q/support/literals.hpp>
#include <q/support/phase.hpp>
#include <q/synth/sin_osc.hpp>

using namespace iplug;
namespace q = cycfi::q;
using namespace q::literals;

// VOICE_CONFIG_START (do not remove this marker)
// LLM: Add, modify, or remove voice definitions here. Keep them between the markers.
// Format: constexpr int kMidiNote<Name> = <midiNote>;
constexpr int kMidiNoteKick = 48;
// VOICE_CONFIG_END (do not remove this marker)

//==============================================================================
// Kick Voice - Sine oscillator with pitch envelope
//==============================================================================
class KickVoice
{
public:
  void Reset(float sampleRate)
  {
    mSampleRate = sampleRate;
    mActive = false;
  }

  void Trigger(float velocity)
  {
    mVelocity = velocity;
    mActive = true;
    mPhase = q::phase_iterator{};
    mAmpEnvValue = 1.0f;
    mPitchEnvValue = 1.0f;
  }

  bool IsActive() const { return mActive; }

  float Process()
  {
    if (!mActive) return 0.0f;

    // Calculate pitch envelope (exponential decay)
    float pitchDecaySamples = mPitchDecayMs * mSampleRate * 0.001f;
    float pitchDecayCoeff = std::exp(-1.0f / pitchDecaySamples);
    mPitchEnvValue *= pitchDecayCoeff;

    // Calculate current frequency
    float freq = mPitchEnd + (mPitchStart - mPitchEnd) * mPitchEnvValue;
    mPhase.set(q::frequency{freq}, mSampleRate);

    // Generate sine
    float osc = q::sin(mPhase++);

    // Amplitude envelope (exponential decay)
    float ampDecaySamples = mAmpDecayMs * mSampleRate * 0.001f;
    float ampDecayCoeff = std::exp(-1.0f / ampDecaySamples);
    mAmpEnvValue *= ampDecayCoeff;

    // Check if voice is done
    if (mAmpEnvValue < 0.0001f)
    {
      mActive = false;
      return 0.0f;
    }

    return osc * mAmpEnvValue * mVelocity;
  }

  void SetPitchStart(float hz) { mPitchStart = hz; }
  void SetPitchEnd(float hz) { mPitchEnd = hz; }
  void SetPitchDecay(float ms) { mPitchDecayMs = ms; }
  void SetAmpDecay(float ms) { mAmpDecayMs = ms; }

private:
  float mSampleRate = 44100.0f;
  bool mActive = false;
  float mVelocity = 0.0f;

  q::phase_iterator mPhase;
  float mAmpEnvValue = 0.0f;
  float mPitchEnvValue = 0.0f;

  // Parameters
  float mPitchStart = 300.0f;
  float mPitchEnd = 50.0f;
  float mPitchDecayMs = 50.0f;
  float mAmpDecayMs = 300.0f;
};

//==============================================================================
// Main DrumMachine DSP Class
//==============================================================================
template<typename T>
class DrumMachineDSP
{
public:
  DrumMachineDSP() = default;

  void Reset(double sampleRate, int blockSize)
  {
    mSampleRate = static_cast<float>(sampleRate);
    mMidiQueue.Resize(blockSize);
    mKick.Reset(mSampleRate);
  }

  void ProcessBlock(T** inputs, T** outputs, int nOutputs, int nFrames)
  {
    // Clear outputs
    for (int c = 0; c < nOutputs; c++)
    {
      std::memset(outputs[c], 0, nFrames * sizeof(T));
    }

    // Process sample by sample
    for (int s = 0; s < nFrames; s++)
    {
      // Process MIDI queue
      while (!mMidiQueue.Empty())
      {
        IMidiMsg& msg = mMidiQueue.Peek();
        if (msg.mOffset > s) break;

        if (msg.StatusMsg() == IMidiMsg::kNoteOn && msg.Velocity() > 0)
        {
          float velocity = msg.Velocity() / 127.0f;
          int note = msg.NoteNumber();

          if (note == kMidiNoteKick)
          {
            mKick.Trigger(velocity);
          }
        }

        mMidiQueue.Remove();
      }

      // Process kick
      float sample = 0.0f;
      if (mKick.IsActive()) sample += mKick.Process();

      // Apply master gain with smoothing
      constexpr float kSmoothCoeff = 0.0005f;
      mGainSmoothed += kSmoothCoeff * (mGain - mGainSmoothed);

      sample *= mGainSmoothed;

      // Soft clipping
      if (sample > 1.0f) sample = 1.0f;
      if (sample < -1.0f) sample = -1.0f;

      // Write to stereo outputs
      outputs[0][s] = static_cast<T>(sample);
      if (nOutputs > 1)
        outputs[1][s] = static_cast<T>(sample);
    }

    mMidiQueue.Flush(nFrames);
  }

  void ProcessMidiMsg(const IMidiMsg& msg)
  {
    mMidiQueue.Add(msg);
  }

  void SetParam(int paramIdx, double value)
  {
    switch (paramIdx)
    {
      // Master
      case kParamGain:
        mGain = static_cast<float>(value / 100.0);
        break;

      // Kick
      case kParamKickPitchStart:
        mKick.SetPitchStart(static_cast<float>(value));
        break;
      case kParamKickPitchEnd:
        mKick.SetPitchEnd(static_cast<float>(value));
        break;
      case kParamKickPitchDecay:
        mKick.SetPitchDecay(static_cast<float>(value));
        break;
      case kParamKickAmpDecay:
        mKick.SetAmpDecay(static_cast<float>(value));
        break;

      default:
        break;
    }
  }

private:
  float mSampleRate = 44100.0f;
  float mGain = 0.8f;
  float mGainSmoothed = 0.8f;

  IMidiQueue mMidiQueue;

  // Drum voices
  KickVoice mKick;
};
