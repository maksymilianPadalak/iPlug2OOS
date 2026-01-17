#pragma once

#include <cmath>

// iPlug2 Synth Infrastructure
#include "MidiSynth.h"

// ═══════════════════════════════════════════════════════════════════════════════
// DSP MODULES - Modular, self-contained DSP components
// ═══════════════════════════════════════════════════════════════════════════════
// These modules can be used independently or combined to build synthesizers.
// Each module is fully documented and includes all necessary utilities.
// ═══════════════════════════════════════════════════════════════════════════════

// DSP Utilities (DenormalGuard, math constants, smoothing, saturation)
#include "dsp_utilities.h"

// Wavetable Oscillator (WavetableOscillator, WavetableGenerator)
#include "wavetable.h"

// Resonant Filter (Cytomic SVF - stable under audio-rate modulation)
#include "resonant_filter.h"

// Stereo Delay (Hermite interpolation, DC blocking, tempo sync)
#include "stereo_delay.h"

// LFO (Low Frequency Oscillator - Q library based, tempo sync)
#include "lfo.h"

// Dattorro Reverb (Lexicon 224-style, modes: Plate/Chamber/Hall/Cathedral)
// See GoldenSynth/Plugin_DSP.h for usage example
#include "dattorro_reverb.h"

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
    }

    // Parameter setters called from DSP class
    void SetWaveform(int waveform) { mWaveform = waveform; }

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
    for (int i = 0; i < nVoices; i++)
    {
      mSynth.AddVoice(new Voice(), 0);
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
  float mGain = 0.8f;
  float mGainSmoothed = 0.8f;
};
