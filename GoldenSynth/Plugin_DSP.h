#pragma once

#include <cmath>
#include <array>
#include <algorithm>

// iPlug2 Synth Infrastructure
#include "MidiSynth.h"

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

// ═══════════════════════════════════════════════════════════════════════════════
// WAVETABLE OSCILLATOR - Simple & Working
// ═══════════════════════════════════════════════════════════════════════════════

constexpr int kWavetableSize = 256;       // Samples per waveform
constexpr int kWavetableFrames = 16;      // Waveforms per table
constexpr float kWavetableSizeF = static_cast<float>(kWavetableSize);

class WavetableOscillator
{
public:
  // Simple wavetable: [frame][sample]
  using WavetableData = std::array<std::array<float, kWavetableSize>, kWavetableFrames>;

  void SetWavetable(const WavetableData* table) { mTable = table; }

  void SetSampleRate(float sampleRate)
  {
    // Sample-rate aware smoothing: ~10ms
    mSmoothCoeff = 1.0f - std::exp(-1.0f / (0.01f * sampleRate));
  }

  void SetFrequency(float freq, float sampleRate)
  {
    mPhaseInc = freq / sampleRate;
  }

  void SetPosition(float pos)
  {
    mTargetPosition = std::max(0.0f, std::min(1.0f, pos)) * (kWavetableFrames - 1);
  }

  void Reset() { mPhase = 0.0f; }

  float Process()
  {
    if (!mTable) return 0.0f;

    // Smooth position
    mPosition += mSmoothCoeff * (mTargetPosition - mPosition);

    // Frame interpolation
    int frame0 = static_cast<int>(mPosition);
    int frame1 = frame0 + 1;
    if (frame1 >= kWavetableFrames) frame1 = kWavetableFrames - 1;
    float frameFrac = mPosition - frame0;

    // Sample interpolation (linear)
    float samplePos = mPhase * kWavetableSizeF;
    int idx0 = static_cast<int>(samplePos);
    int idx1 = idx0 + 1;
    if (idx0 >= kWavetableSize) idx0 = 0;
    if (idx1 >= kWavetableSize) idx1 = 0;
    float sampleFrac = samplePos - static_cast<int>(samplePos);

    // Bilinear interpolation
    const auto& t0 = (*mTable)[frame0];
    const auto& t1 = (*mTable)[frame1];
    float s0 = t0[idx0] + sampleFrac * (t0[idx1] - t0[idx0]);
    float s1 = t1[idx0] + sampleFrac * (t1[idx1] - t1[idx0]);
    float sample = s0 + frameFrac * (s1 - s0);

    // Advance phase
    mPhase += mPhaseInc;
    while (mPhase >= 1.0f) mPhase -= 1.0f;

    return sample;
  }

private:
  const WavetableData* mTable = nullptr;
  float mSmoothCoeff = 0.01f;
  float mPhase = 0.0f;
  float mPhaseInc = 0.0f;
  float mPosition = 0.0f;
  float mTargetPosition = 0.0f;
};

// ═══════════════════════════════════════════════════════════════════════════════
// WAVETABLE GENERATOR - Simple waveform morphing
// ═══════════════════════════════════════════════════════════════════════════════

class WavetableGenerator
{
public:
  using WavetableData = WavetableOscillator::WavetableData;
  static constexpr float kPi = 3.14159265359f;
  static constexpr float k2Pi = 2.0f * kPi;

  // Generate "Basic Shapes": Sine → Triangle → Saw → Square
  static WavetableData GenerateBasicShapes()
  {
    WavetableData table{};

    for (int frame = 0; frame < kWavetableFrames; frame++)
    {
      float t = static_cast<float>(frame) / (kWavetableFrames - 1);

      for (int i = 0; i < kWavetableSize; i++)
      {
        float phase = static_cast<float>(i) / kWavetableSize;

        // Simple waveforms (no additive - fast!)
        float sine = std::sin(phase * k2Pi);
        float triangle = 4.0f * std::abs(phase - 0.5f) - 1.0f;
        float saw = 2.0f * phase - 1.0f;
        float square = phase < 0.5f ? 1.0f : -1.0f;

        // Morph between shapes
        float sample;
        if (t < 0.333f)
        {
          float blend = t / 0.333f;
          sample = sine + blend * (triangle - sine);
        }
        else if (t < 0.666f)
        {
          float blend = (t - 0.333f) / 0.333f;
          sample = triangle + blend * (saw - triangle);
        }
        else
        {
          float blend = (t - 0.666f) / 0.334f;
          sample = saw + blend * (square - saw);
        }

        table[frame][i] = sample;
      }
    }
    return table;
  }
};

enum EWaveform
{
  kWaveformSine = 0,
  kWaveformSaw,
  kWaveformSquare,
  kWaveformTriangle,
  kWaveformWavetable,  // NEW: Wavetable mode
  kNumWaveforms
};

template<typename T>
class PluginInstanceDSP
{
public:
#pragma mark - Voice
  class Voice : public SynthVoice
  {
  public:
    Voice()
    {
      mEnvConfig = q::adsr_envelope_gen::config{
        q::duration{0.01f},   // 10ms attack
        q::duration{0.1f},    // 100ms decay
        q::lin_to_db(0.7f),   // 70% sustain
        50_s,                 // sustain hold
        q::duration{0.2f}     // 200ms release
      };
    }

    // Set shared wavetable data (called once from DSP class)
    void SetWavetable(const WavetableOscillator::WavetableData* table)
    {
      mWavetableOsc.SetWavetable(table);
    }

    bool GetBusy() const override
    {
      return mActive && !mEnv.in_idle_phase();
    }

    void Trigger(double level, bool isRetrigger) override
    {
      // Capture current envelope level BEFORE creating new envelope
      float currentLevel = mActive ? mEnv.current() : 0.0f;

      mActive = true;
      mVelocity = static_cast<float>(level);

      if (!isRetrigger)
      {
        // New note - reset phase
        mPhase = q::phase_iterator{};
        mWavetableOsc.Reset();
      }

      // Create fresh envelope
      mEnv = q::adsr_envelope_gen{mEnvConfig, static_cast<float>(mSampleRate)};
      mEnv.attack();

      // Retrigger smoothing - prevents clicks when retriggering from non-zero level
      if (currentLevel > 0.01f)
      {
        mRetriggerOffset = currentLevel;
        mRetriggerDecay = 0.999f;  // ~7ms decay at 44.1kHz
      }
      else
      {
        mRetriggerOffset = 0.0f;
        mRetriggerDecay = 1.0f;
      }
    }

    void Release() override
    {
      mEnv.release();
    }

    void ProcessSamplesAccumulating(T** inputs, T** outputs,
                                     int nInputs, int nOutputs,
                                     int startIdx, int nFrames) override
    {
      // Get pitch from voice allocator (1V/octave format)
      double pitch = mInputs[kVoiceControlPitch].endValue;
      double pitchBend = mInputs[kVoiceControlPitchBend].endValue;

      // Convert to frequency: 440 * 2^pitch
      double freq = 440.0 * std::pow(2.0, pitch + pitchBend);
      mPhase.set(q::frequency{static_cast<float>(freq)}, static_cast<float>(mSampleRate));

      // Update wavetable oscillator frequency
      mWavetableOsc.SetFrequency(static_cast<float>(freq), static_cast<float>(mSampleRate));

      for (int i = startIdx; i < startIdx + nFrames; i++)
      {
        // Get envelope amplitude
        float envAmp = mEnv();

        // Apply retrigger offset for smooth transitions
        if (mRetriggerOffset > 0.001f)
        {
          if (envAmp < mRetriggerOffset)
            envAmp = mRetriggerOffset;
          mRetriggerOffset *= mRetriggerDecay;
        }
        else
        {
          mRetriggerOffset = 0.0f;
        }

        // Clamp envelope
        if (envAmp < 0.0f) envAmp = 0.0f;
        if (envAmp > 1.0f) envAmp = 1.0f;

        // Check if voice should deactivate
        if (envAmp < 0.0001f && mEnv.in_idle_phase())
        {
          mActive = false;
          return;
        }

        // Generate oscillator sample
        float oscValue = 0.0f;
        switch (mWaveform)
        {
          case kWaveformSine:
            oscValue = q::sin(mPhase++);
            break;
          case kWaveformSaw:
            oscValue = q::saw(mPhase++);
            break;
          case kWaveformSquare:
            oscValue = q::square(mPhase++);
            break;
          case kWaveformTriangle:
            oscValue = q::triangle(mPhase++);
            break;
          case kWaveformWavetable:
            oscValue = mWavetableOsc.Process();
            break;
          default:
            oscValue = q::sin(mPhase++);
            break;
        }

        // Apply envelope and velocity
        float sample = oscValue * envAmp * mVelocity;

        // Accumulate to outputs (don't overwrite - other voices add here too)
        outputs[0][i] += sample;
        if (nOutputs > 1)
          outputs[1][i] += sample;
      }
    }

    void SetSampleRateAndBlockSize(double sampleRate, int blockSize) override
    {
      mSampleRate = sampleRate;
      mEnv = q::adsr_envelope_gen{mEnvConfig, static_cast<float>(sampleRate)};
      mWavetableOsc.SetSampleRate(static_cast<float>(sampleRate));
    }

    // Parameter setters called from DSP class
    void SetWaveform(int waveform) { mWaveform = waveform; }
    void SetWavetablePosition(float pos) { mWavetableOsc.SetPosition(pos); }

    void SetAttack(float ms)
    {
      mEnvConfig.attack_rate = q::duration{ms * 0.001f};
    }

    void SetDecay(float ms)
    {
      mEnvConfig.decay_rate = q::duration{ms * 0.001f};
    }

    void SetSustain(float level)
    {
      mEnvConfig.sustain_level = q::lin_to_db(level);
    }

    void SetRelease(float ms)
    {
      mEnvConfig.release_rate = q::duration{ms * 0.001f};
    }

  private:
    q::phase_iterator mPhase;
    q::adsr_envelope_gen mEnv{q::adsr_envelope_gen::config{}, 44100.0f};
    q::adsr_envelope_gen::config mEnvConfig;
    WavetableOscillator mWavetableOsc;  // Wavetable oscillator
    double mSampleRate = 44100.0;
    float mVelocity = 0.0f;
    bool mActive = false;
    int mWaveform = kWaveformSine;

    // Retrigger smoothing
    float mRetriggerOffset = 0.0f;
    float mRetriggerDecay = 1.0f;
  };

public:
#pragma mark - DSP
  PluginInstanceDSP(int nVoices = 8)
  {
    // Generate wavetable at startup
    mWavetable = WavetableGenerator::GenerateBasicShapes();

    for (int i = 0; i < nVoices; i++)
    {
      Voice* voice = new Voice();
      voice->SetWavetable(&mWavetable);  // Share wavetable data
      mSynth.AddVoice(voice, 0);
    }
  }

  void ProcessBlock(T** inputs, T** outputs, int nOutputs, int nFrames)
  {
    // Clear outputs first
    for (int c = 0; c < nOutputs; c++)
    {
      memset(outputs[c], 0, nFrames * sizeof(T));
    }

    // MidiSynth processes MIDI queue and calls voice ProcessSamplesAccumulating
    mSynth.ProcessBlock(inputs, outputs, 0, nOutputs, nFrames);

    // Apply master gain with smoothing
    for (int s = 0; s < nFrames; s++)
    {
      constexpr float kSmoothCoeff = 0.0005f;
      mGainSmoothed += kSmoothCoeff * (mGain - mGainSmoothed);

      // Fixed poly scale for 8 voices
      constexpr float kPolyScale = 0.35f;
      float gainScale = kPolyScale * mGainSmoothed;

      for (int c = 0; c < nOutputs; c++)
      {
        outputs[c][s] *= gainScale;

        // Safety clamp
        if (outputs[c][s] > 1.0f) outputs[c][s] = 1.0f;
        if (outputs[c][s] < -1.0f) outputs[c][s] = -1.0f;
      }
    }
  }

  void Reset(double sampleRate, int blockSize)
  {
    mSynth.SetSampleRateAndBlockSize(sampleRate, blockSize);
    mSynth.Reset();
    mGainSmoothed = mGain;
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
        mGain = static_cast<float>(value / 100.0);
        break;

      case kParamWaveform:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          dynamic_cast<Voice&>(voice).SetWaveform(static_cast<int>(value));
        });
        break;

      case kParamWavetablePosition:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          dynamic_cast<Voice&>(voice).SetWavetablePosition(static_cast<float>(value / 100.0));
        });
        break;

      case kParamAttack:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          dynamic_cast<Voice&>(voice).SetAttack(static_cast<float>(value));
        });
        break;

      case kParamDecay:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          dynamic_cast<Voice&>(voice).SetDecay(static_cast<float>(value));
        });
        break;

      case kParamSustain:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          dynamic_cast<Voice&>(voice).SetSustain(static_cast<float>(value / 100.0));
        });
        break;

      case kParamRelease:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          dynamic_cast<Voice&>(voice).SetRelease(static_cast<float>(value));
        });
        break;

      default:
        break;
    }
  }

private:
  MidiSynth mSynth{VoiceAllocator::kPolyModePoly};
  WavetableOscillator::WavetableData mWavetable;  // Shared wavetable data
  float mGain = 0.8f;
  float mGainSmoothed = 0.8f;
};
