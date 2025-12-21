#pragma once

#include <cmath>
#include <cstring>

// iPlug2 MIDI Infrastructure
#include "IPlugMidi.h"

// Q DSP Library
#include <q/support/literals.hpp>
#include <q/support/phase.hpp>
#include <q/synth/sin_osc.hpp>
#include <q/synth/noise_gen.hpp>
#include <q/fx/biquad.hpp>

using namespace iplug;
namespace q = cycfi::q;
using namespace q::literals;

// GM MIDI note numbers for drums
constexpr int kMidiNoteKick = 36;        // C1 - Bass Drum 1
constexpr int kMidiNoteRim = 37;         // C#1 - Side Stick
constexpr int kMidiNoteSnare = 38;       // D1 - Acoustic Snare
constexpr int kMidiNoteClap = 39;        // D#1 - Hand Clap
constexpr int kMidiNoteHiHatClosed = 42; // F#1 - Closed Hi-Hat
constexpr int kMidiNoteTom = 45;         // A1 - Low Tom
constexpr int kMidiNoteHiHatOpen = 46;   // A#1 - Open Hi-Hat

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
// Snare Voice - Noise + bandpass filter with optional body sine
//==============================================================================
class SnareVoice
{
public:
  void Reset(float sampleRate)
  {
    mSampleRate = sampleRate;
    mActive = false;
    UpdateFilter();
  }

  void Trigger(float velocity)
  {
    mVelocity = velocity;
    mActive = true;
    mAmpEnvValue = 1.0f;
    mBodyPhase = q::phase_iterator{};
    mBodyPhase.set(q::frequency{180.0f}, mSampleRate); // Body at 180Hz
  }

  bool IsActive() const { return mActive; }

  float Process()
  {
    if (!mActive) return 0.0f;

    // Noise through bandpass
    float noise = mNoise();
    float filtered = mFilter(noise);

    // Body sine component
    float body = q::sin(mBodyPhase++);

    // Mix noise and body
    float bodyMix = mBodyMixPercent * 0.01f;
    float sample = filtered * (1.0f - bodyMix) + body * bodyMix;

    // Amplitude envelope
    float decaySamples = mNoiseDecayMs * mSampleRate * 0.001f;
    float decayCoeff = std::exp(-1.0f / decaySamples);
    mAmpEnvValue *= decayCoeff;

    if (mAmpEnvValue < 0.0001f)
    {
      mActive = false;
      return 0.0f;
    }

    return sample * mAmpEnvValue * mVelocity;
  }

  void SetFilterFreq(float hz)
  {
    mFilterFreq = hz;
    UpdateFilter();
  }

  void SetFilterQ(float q)
  {
    mFilterQ = q;
    UpdateFilter();
  }

  void SetNoiseDecay(float ms) { mNoiseDecayMs = ms; }
  void SetBodyMix(float percent) { mBodyMixPercent = percent; }

private:
  void UpdateFilter()
  {
    mFilter = q::bandpass_csg{q::frequency{mFilterFreq}, mSampleRate, mFilterQ};
  }

  float mSampleRate = 44100.0f;
  bool mActive = false;
  float mVelocity = 0.0f;
  float mAmpEnvValue = 0.0f;

  q::white_noise_gen mNoise;
  q::bandpass_csg mFilter{q::frequency{2000.0f}, 44100.0f, 1.0f};
  q::phase_iterator mBodyPhase;

  // Parameters
  float mFilterFreq = 2000.0f;
  float mFilterQ = 1.0f;
  float mNoiseDecayMs = 150.0f;
  float mBodyMixPercent = 30.0f;
};

//==============================================================================
// HiHat Voice - Noise + highpass filter (closed or open mode)
//==============================================================================
class HiHatVoice
{
public:
  void Reset(float sampleRate)
  {
    mSampleRate = sampleRate;
    mActive = false;
    UpdateFilter();
  }

  void Trigger(float velocity, bool isOpen)
  {
    mVelocity = velocity;
    mIsOpen = isOpen;
    mActive = true;
    mAmpEnvValue = 1.0f;
  }

  bool IsActive() const { return mActive; }

  float Process()
  {
    if (!mActive) return 0.0f;

    // Noise through highpass
    float noise = mNoise();
    float filtered = mFilter(noise);

    // Select decay based on open/closed
    float decayMs = mIsOpen ? mOpenDecayMs : mClosedDecayMs;
    float decaySamples = decayMs * mSampleRate * 0.001f;
    float decayCoeff = std::exp(-1.0f / decaySamples);
    mAmpEnvValue *= decayCoeff;

    if (mAmpEnvValue < 0.0001f)
    {
      mActive = false;
      return 0.0f;
    }

    return filtered * mAmpEnvValue * mVelocity;
  }

  void SetFilterFreq(float hz)
  {
    mFilterFreq = hz;
    UpdateFilter();
  }

  void SetClosedDecay(float ms) { mClosedDecayMs = ms; }
  void SetOpenDecay(float ms) { mOpenDecayMs = ms; }

private:
  void UpdateFilter()
  {
    mFilter = q::highpass{q::frequency{mFilterFreq}, mSampleRate};
  }

  float mSampleRate = 44100.0f;
  bool mActive = false;
  bool mIsOpen = false;
  float mVelocity = 0.0f;
  float mAmpEnvValue = 0.0f;

  q::white_noise_gen mNoise;
  q::highpass mFilter{q::frequency{8000.0f}, 44100.0f};

  // Parameters
  float mFilterFreq = 8000.0f;
  float mClosedDecayMs = 30.0f;
  float mOpenDecayMs = 400.0f;
};

//==============================================================================
// Tom Voice - Similar to kick but different pitch range
//==============================================================================
class TomVoice
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

    // Pitch envelope
    float pitchDecaySamples = mPitchDecayMs * mSampleRate * 0.001f;
    float pitchDecayCoeff = std::exp(-1.0f / pitchDecaySamples);
    mPitchEnvValue *= pitchDecayCoeff;

    float freq = mPitchEnd + (mPitchStart - mPitchEnd) * mPitchEnvValue;
    mPhase.set(q::frequency{freq}, mSampleRate);

    float osc = q::sin(mPhase++);

    // Amplitude envelope
    float ampDecaySamples = mAmpDecayMs * mSampleRate * 0.001f;
    float ampDecayCoeff = std::exp(-1.0f / ampDecaySamples);
    mAmpEnvValue *= ampDecayCoeff;

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
  float mPitchStart = 200.0f;
  float mPitchEnd = 80.0f;
  float mPitchDecayMs = 60.0f;
  float mAmpDecayMs = 400.0f;
};

//==============================================================================
// Clap Voice - Multiple noise bursts with bandpass
//==============================================================================
class ClapVoice
{
public:
  void Reset(float sampleRate)
  {
    mSampleRate = sampleRate;
    mActive = false;
    UpdateFilter();
  }

  void Trigger(float velocity)
  {
    mVelocity = velocity;
    mActive = true;
    mAmpEnvValue = 1.0f;
    mBurstCounter = 0;
    mBurstSampleCounter = 0;
    mCurrentBurstAmp = 1.0f;
  }

  bool IsActive() const { return mActive; }

  float Process()
  {
    if (!mActive) return 0.0f;

    // Generate noise burst pattern (3-4 bursts)
    float noise = mNoise();
    float filtered = mFilter(noise);

    // Burst envelope - create multiple hits
    constexpr int kNumBursts = 4;
    float spreadSamples = mSpreadMs * mSampleRate * 0.001f;
    int samplesPerBurst = static_cast<int>(spreadSamples);
    if (samplesPerBurst < 1) samplesPerBurst = 1;

    float burstEnv = 0.0f;
    if (mBurstCounter < kNumBursts)
    {
      // Quick attack/decay per burst
      float burstProgress = static_cast<float>(mBurstSampleCounter) / static_cast<float>(samplesPerBurst);
      burstEnv = std::exp(-burstProgress * 5.0f) * mCurrentBurstAmp;

      mBurstSampleCounter++;
      if (mBurstSampleCounter >= samplesPerBurst)
      {
        mBurstSampleCounter = 0;
        mBurstCounter++;
        mCurrentBurstAmp *= 0.7f; // Each burst slightly quieter
      }
    }

    // Overall decay envelope
    float decaySamples = mDecayMs * mSampleRate * 0.001f;
    float decayCoeff = std::exp(-1.0f / decaySamples);
    mAmpEnvValue *= decayCoeff;

    if (mAmpEnvValue < 0.0001f && mBurstCounter >= kNumBursts)
    {
      mActive = false;
      return 0.0f;
    }

    return filtered * burstEnv * mAmpEnvValue * mVelocity;
  }

  void SetFilterFreq(float hz)
  {
    mFilterFreq = hz;
    UpdateFilter();
  }

  void SetDecay(float ms) { mDecayMs = ms; }
  void SetSpread(float ms) { mSpreadMs = ms; }

private:
  void UpdateFilter()
  {
    mFilter = q::bandpass_csg{q::frequency{mFilterFreq}, mSampleRate, 1.5f};
  }

  float mSampleRate = 44100.0f;
  bool mActive = false;
  float mVelocity = 0.0f;
  float mAmpEnvValue = 0.0f;

  q::white_noise_gen mNoise;
  q::bandpass_csg mFilter{q::frequency{1500.0f}, 44100.0f, 1.5f};

  int mBurstCounter = 0;
  int mBurstSampleCounter = 0;
  float mCurrentBurstAmp = 1.0f;

  // Parameters
  float mFilterFreq = 1500.0f;
  float mDecayMs = 200.0f;
  float mSpreadMs = 20.0f;
};

//==============================================================================
// Rim Voice - Short click + high sine
//==============================================================================
class RimVoice
{
public:
  void Reset(float sampleRate)
  {
    mSampleRate = sampleRate;
    mActive = false;
    UpdateFilter();
  }

  void Trigger(float velocity)
  {
    mVelocity = velocity;
    mActive = true;
    mAmpEnvValue = 1.0f;
    mPhase = q::phase_iterator{};
    mPhase.set(q::frequency{mPitch}, mSampleRate);
  }

  bool IsActive() const { return mActive; }

  float Process()
  {
    if (!mActive) return 0.0f;

    // Click noise component
    float noise = mNoise();
    float filteredNoise = mFilter(noise);

    // Sine component
    float sine = q::sin(mPhase++);

    // Mix based on click parameter
    float clickMix = mClickPercent * 0.01f;
    float sample = filteredNoise * clickMix + sine * (1.0f - clickMix);

    // Very fast decay
    float decaySamples = mDecayMs * mSampleRate * 0.001f;
    float decayCoeff = std::exp(-1.0f / decaySamples);
    mAmpEnvValue *= decayCoeff;

    if (mAmpEnvValue < 0.0001f)
    {
      mActive = false;
      return 0.0f;
    }

    return sample * mAmpEnvValue * mVelocity;
  }

  void SetPitch(float hz)
  {
    mPitch = hz;
    if (mActive)
    {
      mPhase.set(q::frequency{mPitch}, mSampleRate);
    }
  }

  void SetDecay(float ms) { mDecayMs = ms; }
  void SetClick(float percent) { mClickPercent = percent; }

private:
  void UpdateFilter()
  {
    mFilter = q::highpass{q::frequency{2000.0f}, mSampleRate};
  }

  float mSampleRate = 44100.0f;
  bool mActive = false;
  float mVelocity = 0.0f;
  float mAmpEnvValue = 0.0f;

  q::phase_iterator mPhase;
  q::white_noise_gen mNoise;
  q::highpass mFilter{q::frequency{2000.0f}, 44100.0f};

  // Parameters
  float mPitch = 800.0f;
  float mDecayMs = 20.0f;
  float mClickPercent = 50.0f;
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
    mSnare.Reset(mSampleRate);
    mHiHat.Reset(mSampleRate);
    mTom.Reset(mSampleRate);
    mClap.Reset(mSampleRate);
    mRim.Reset(mSampleRate);
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

          switch (note)
          {
            case kMidiNoteKick:
              mKick.Trigger(velocity);
              break;
            case kMidiNoteSnare:
              mSnare.Trigger(velocity);
              break;
            case kMidiNoteHiHatClosed:
              mHiHat.Trigger(velocity, false);
              break;
            case kMidiNoteHiHatOpen:
              mHiHat.Trigger(velocity, true);
              break;
            case kMidiNoteTom:
              mTom.Trigger(velocity);
              break;
            case kMidiNoteClap:
              mClap.Trigger(velocity);
              break;
            case kMidiNoteRim:
              mRim.Trigger(velocity);
              break;
            default:
              break;
          }
        }

        mMidiQueue.Remove();
      }

      // Sum all drum outputs
      float sample = 0.0f;

      if (mKick.IsActive()) sample += mKick.Process();
      if (mSnare.IsActive()) sample += mSnare.Process();
      if (mHiHat.IsActive()) sample += mHiHat.Process();
      if (mTom.IsActive()) sample += mTom.Process();
      if (mClap.IsActive()) sample += mClap.Process();
      if (mRim.IsActive()) sample += mRim.Process();

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

      // Snare
      case kParamSnareFilterFreq:
        mSnare.SetFilterFreq(static_cast<float>(value));
        break;
      case kParamSnareFilterQ:
        mSnare.SetFilterQ(static_cast<float>(value));
        break;
      case kParamSnareNoiseDecay:
        mSnare.SetNoiseDecay(static_cast<float>(value));
        break;
      case kParamSnareBodyMix:
        mSnare.SetBodyMix(static_cast<float>(value));
        break;

      // HiHat
      case kParamHiHatFilterFreq:
        mHiHat.SetFilterFreq(static_cast<float>(value));
        break;
      case kParamHiHatClosedDecay:
        mHiHat.SetClosedDecay(static_cast<float>(value));
        break;
      case kParamHiHatOpenDecay:
        mHiHat.SetOpenDecay(static_cast<float>(value));
        break;

      // Tom
      case kParamTomPitchStart:
        mTom.SetPitchStart(static_cast<float>(value));
        break;
      case kParamTomPitchEnd:
        mTom.SetPitchEnd(static_cast<float>(value));
        break;
      case kParamTomPitchDecay:
        mTom.SetPitchDecay(static_cast<float>(value));
        break;
      case kParamTomAmpDecay:
        mTom.SetAmpDecay(static_cast<float>(value));
        break;

      // Clap
      case kParamClapFilterFreq:
        mClap.SetFilterFreq(static_cast<float>(value));
        break;
      case kParamClapDecay:
        mClap.SetDecay(static_cast<float>(value));
        break;
      case kParamClapSpread:
        mClap.SetSpread(static_cast<float>(value));
        break;

      // Rim
      case kParamRimPitch:
        mRim.SetPitch(static_cast<float>(value));
        break;
      case kParamRimDecay:
        mRim.SetDecay(static_cast<float>(value));
        break;
      case kParamRimClick:
        mRim.SetClick(static_cast<float>(value));
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
  SnareVoice mSnare;
  HiHatVoice mHiHat;
  TomVoice mTom;
  ClapVoice mClap;
  RimVoice mRim;
};
