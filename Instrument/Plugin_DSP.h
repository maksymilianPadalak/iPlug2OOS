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
#include <q/synth/linear_gen.hpp>
#include <q/fx/biquad.hpp>

#include <cmath>

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

static constexpr int kNumOscillators = 3;

struct OscillatorParams
{
  float volume = 0.8f;
  float detune = 0.0f;
  int octave = 0;
  float pan = 0.0f;
  int waveform = 0;
  float attackMs = 10.0f;
  float decayMs = 100.0f;
  float sustainLevel = 0.7f;
  float releaseMs = 200.0f;
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

    for (int s = 0; s < nFrames; ++s)
    {
      T sampleL = static_cast<T>(0.0);
      T sampleR = static_cast<T>(0.0);

      for (auto& voice : mVoices)
      {
        if (!voice.active)
          continue;

        // Check if voice should be deactivated (all envelopes finished)
        if (voice.AllEnvelopesIdle())
        {
          voice.active = false;
          continue;
        }

        float voiceSampleL = 0.0f;
        float voiceSampleR = 0.0f;
        float totalLevel = 0.0f;
        
        // Sum all oscillators with per-oscillator panning
        for (int osc = 0; osc < kNumOscillators; ++osc)
        {
          // Get envelope amplitude for this oscillator
          float envAmp = std::min(voice.envs[osc](), 1.0f);
          
          // Skip if this oscillator's envelope is idle and releasing
          if (voice.oscReleasing[osc] && voice.envs[osc].in_idle_phase())
            continue;

          // Generate oscillator sample using Q library
          float oscValue = 0.0f;
          switch (mOscParams[osc].waveform)
          {
            case kWaveformSine:
              oscValue = q::sin(voice.phases[osc]++);
              break;
            case kWaveformSaw:
              oscValue = q::saw(voice.phases[osc]++);
              break;
            case kWaveformSquare:
              oscValue = q::square(voice.phases[osc]++);
              break;
            case kWaveformTriangle:
              oscValue = q::triangle(voice.phases[osc]++);
              break;
            default:
              oscValue = q::sin(voice.phases[osc]++);
              break;
          }

          // Apply envelope and volume
          float oscSample = oscValue * envAmp * mOscParams[osc].volume;
          
          // Calculate equal-power pan gains
          // Pan is -100 to +100, normalize to -1 to +1
          float panNorm = mOscParams[osc].pan / 100.0f;
          float leftGain = std::sqrt((1.0f - panNorm) * 0.5f);
          float rightGain = std::sqrt((1.0f + panNorm) * 0.5f);
          
          // Accumulate to stereo channels
          voiceSampleL += oscSample * leftGain;
          voiceSampleR += oscSample * rightGain;
          totalLevel += mOscParams[osc].volume;
        }
        
        // Normalize oscillator mix to prevent level stacking
        if (totalLevel > 1.0f)
        {
          voiceSampleL /= totalLevel;
          voiceSampleR /= totalLevel;
        }
        
        // Apply filter to mono sum, then reconstruct stereo
        float monoSum = (voiceSampleL + voiceSampleR) * 0.5f;
        float filtered = voice.filter(monoSum);
        
        // Apply resonance compensation
        const float resonanceCompensation = 1.0f / std::max(0.707f, mFilterQ);
        filtered *= resonanceCompensation;
        
        // Reconstruct stereo from filtered mono (preserve original L/R ratio)
        float ratio = (monoSum != 0.0f) ? filtered / monoSum : 1.0f;
        voiceSampleL *= ratio;
        voiceSampleR *= ratio;

        // Apply voice velocity
        voiceSampleL *= voice.velocity;
        voiceSampleR *= voice.velocity;

        // Handle voice stealing crossfade
        if (voice.isBeingStolen)
        {
          float fadeGain = voice.stealFade();
          voiceSampleL *= fadeGain;
          voiceSampleR *= fadeGain;

          // Check if fade is complete
          if (fadeGain <= 0.001f)
          {
            // Fade complete - activate pending note
            voice.isBeingStolen = false;
            if (voice.pendingNote >= 0)
            {
              voice.Activate(voice.pendingNote, voice.pendingFrequency,
                             voice.pendingVelocity, mSampleRate, mOscParams,
                             mFilterCutoff, mFilterQ);
              voice.pendingNote = -1;
            }
            else
            {
              voice.active = false;
            }
          }
        }

        sampleL += static_cast<T>(voiceSampleL);
        sampleR += static_cast<T>(voiceSampleR);
      }

      // Scale down for polyphony (1/sqrt(maxVoices) preserves perceived loudness)
      constexpr T kPolyScale = static_cast<T>(1.0 / 2.83); // ~1/sqrt(8)
      sampleL *= gain * kPolyScale;
      sampleR *= gain * kPolyScale;

      // Hard clip to prevent any remaining clipping (safety measure)
      sampleL = std::max(static_cast<T>(-1.0), std::min(sampleL, static_cast<T>(1.0)));
      sampleR = std::max(static_cast<T>(-1.0), std::min(sampleR, static_cast<T>(1.0)));

      if (nOutputs >= 2)
      {
        outputs[0][s] += sampleL;
        outputs[1][s] += sampleR;
      }
      else if (nOutputs == 1)
      {
        outputs[0][s] += (sampleL + sampleR) * static_cast<T>(0.5);
      }
    }
  }

    void Reset(double sampleRate, int /*blockSize*/)
  {
    mSampleRate = static_cast<float>(sampleRate);

    // Initialize oscillator parameters with defaults matching Plugin.cpp
    // Oscillator 1: 80% volume, Sine wave
    mOscParams[0].volume = 0.8f;
    mOscParams[0].detune = 0.0f;
    mOscParams[0].octave = 0;
    mOscParams[0].pan = 0.0f;
    mOscParams[0].waveform = kWaveformSine;
    mOscParams[0].attackMs = 10.0f;
    mOscParams[0].decayMs = 100.0f;
    mOscParams[0].sustainLevel = 0.7f;
    mOscParams[0].releaseMs = 200.0f;

    // Oscillator 2: 0% volume, Saw wave
    mOscParams[1].volume = 0.0f;
    mOscParams[1].detune = 0.0f;
    mOscParams[1].octave = 0;
    mOscParams[1].pan = 0.0f;
    mOscParams[1].waveform = kWaveformSaw;
    mOscParams[1].attackMs = 10.0f;
    mOscParams[1].decayMs = 100.0f;
    mOscParams[1].sustainLevel = 0.7f;
    mOscParams[1].releaseMs = 200.0f;

    // Oscillator 3: 0% volume, Square wave
    mOscParams[2].volume = 0.0f;
    mOscParams[2].detune = 0.0f;
    mOscParams[2].octave = 0;
    mOscParams[2].pan = 0.0f;
    mOscParams[2].waveform = kWaveformSquare;
    mOscParams[2].attackMs = 10.0f;
    mOscParams[2].decayMs = 100.0f;
    mOscParams[2].sustainLevel = 0.7f;
    mOscParams[2].releaseMs = 200.0f;

    // Initialize filter defaults
    mFilterCutoff = 8000.0f;
    mFilterQ = 0.707f;

    // Create ADSR configs for each oscillator
    for (int i = 0; i < kNumOscillators; ++i)
    {
      mEnvConfigs[i] = q::adsr_envelope_gen::config{
        q::duration{mOscParams[i].attackMs * 0.001f},
        q::duration{mOscParams[i].decayMs * 0.001f},
        q::lin_to_db(mOscParams[i].sustainLevel),
        50_s,
        q::duration{mOscParams[i].releaseMs * 0.001f}
      };
    }

    for (auto& voice : mVoices)
    {
      voice.Reset(mEnvConfigs, mSampleRate, mFilterCutoff, mFilterQ);
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
      
      // Oscillator 1 parameters
      case kParamOsc1Volume:
        mOscParams[0].volume = static_cast<float>(value / 100.0);
        break;
      case kParamOsc1Detune:
        mOscParams[0].detune = static_cast<float>(value);
        for (auto& voice : mVoices)
        {
          if (voice.active && voice.baseFrequency > 0.0f)
          {
            float octaveMultiplier = std::pow(2.0f, static_cast<float>(mOscParams[0].octave));
            float detuneRatio = std::pow(2.0f, mOscParams[0].detune / 1200.0f);
            float oscFreq = voice.baseFrequency * octaveMultiplier * detuneRatio;
            voice.phases[0].set(q::frequency{oscFreq}, mSampleRate);
          }
        }
        break;
      case kParamOsc1Octave:
        mOscParams[0].octave = static_cast<int>(value);
        for (auto& voice : mVoices)
        {
          if (voice.active && voice.baseFrequency > 0.0f)
          {
            float octaveMultiplier = std::pow(2.0f, static_cast<float>(mOscParams[0].octave));
            float detuneRatio = std::pow(2.0f, mOscParams[0].detune / 1200.0f);
            float oscFreq = voice.baseFrequency * octaveMultiplier * detuneRatio;
            voice.phases[0].set(q::frequency{oscFreq}, mSampleRate);
          }
        }
        break;
      case kParamOsc1Pan:
        mOscParams[0].pan = static_cast<float>(value);
        break;
      case kParamOsc1Waveform:
        mOscParams[0].waveform = static_cast<int>(value);
        break;
      case kParamOsc1Attack:
        mOscParams[0].attackMs = static_cast<float>(value);
        mEnvConfigs[0].attack_rate = q::duration{value * 0.001};
        for (auto& voice : mVoices)
        {
          if (voice.active)
          {
            voice.envs[0].attack_rate(q::duration{value * 0.001}, mSampleRate);
          }
        }
        break;
      case kParamOsc1Decay:
        mOscParams[0].decayMs = static_cast<float>(value);
        mEnvConfigs[0].decay_rate = q::duration{value * 0.001};
        for (auto& voice : mVoices)
        {
          if (voice.active)
          {
            voice.envs[0].decay_rate(q::duration{value * 0.001}, mSampleRate);
          }
        }
        break;
      case kParamOsc1Sustain:
        mOscParams[0].sustainLevel = static_cast<float>(value / 100.0);
        mEnvConfigs[0].sustain_level = q::lin_to_db(static_cast<float>(value / 100.0));
        for (auto& voice : mVoices)
        {
          if (voice.active)
          {
            voice.envs[0].sustain_level(q::lin_to_db(static_cast<float>(value / 100.0)));
          }
        }
        break;
      case kParamOsc1Release:
        mOscParams[0].releaseMs = static_cast<float>(value);
        mEnvConfigs[0].release_rate = q::duration{value * 0.001};
        for (auto& voice : mVoices)
        {
          if (voice.active)
          {
            voice.envs[0].release_rate(q::duration{value * 0.001}, mSampleRate);
          }
        }
        break;
      
      // Oscillator 2 parameters
      case kParamOsc2Volume:
        mOscParams[1].volume = static_cast<float>(value / 100.0);
        break;
      case kParamOsc2Detune:
        mOscParams[1].detune = static_cast<float>(value);
        for (auto& voice : mVoices)
        {
          if (voice.active && voice.baseFrequency > 0.0f)
          {
            float octaveMultiplier = std::pow(2.0f, static_cast<float>(mOscParams[1].octave));
            float detuneRatio = std::pow(2.0f, mOscParams[1].detune / 1200.0f);
            float oscFreq = voice.baseFrequency * octaveMultiplier * detuneRatio;
            voice.phases[1].set(q::frequency{oscFreq}, mSampleRate);
          }
        }
        break;
      case kParamOsc2Octave:
        mOscParams[1].octave = static_cast<int>(value);
        for (auto& voice : mVoices)
        {
          if (voice.active && voice.baseFrequency > 0.0f)
          {
            float octaveMultiplier = std::pow(2.0f, static_cast<float>(mOscParams[1].octave));
            float detuneRatio = std::pow(2.0f, mOscParams[1].detune / 1200.0f);
            float oscFreq = voice.baseFrequency * octaveMultiplier * detuneRatio;
            voice.phases[1].set(q::frequency{oscFreq}, mSampleRate);
          }
        }
        break;
      case kParamOsc2Pan:
        mOscParams[1].pan = static_cast<float>(value);
        break;
      case kParamOsc2Waveform:
        mOscParams[1].waveform = static_cast<int>(value);
        break;
      case kParamOsc2Attack:
        mOscParams[1].attackMs = static_cast<float>(value);
        mEnvConfigs[1].attack_rate = q::duration{value * 0.001};
        for (auto& voice : mVoices)
        {
          if (voice.active)
          {
            voice.envs[1].attack_rate(q::duration{value * 0.001}, mSampleRate);
          }
        }
        break;
      case kParamOsc2Decay:
        mOscParams[1].decayMs = static_cast<float>(value);
        mEnvConfigs[1].decay_rate = q::duration{value * 0.001};
        for (auto& voice : mVoices)
        {
          if (voice.active)
          {
            voice.envs[1].decay_rate(q::duration{value * 0.001}, mSampleRate);
          }
        }
        break;
      case kParamOsc2Sustain:
        mOscParams[1].sustainLevel = static_cast<float>(value / 100.0);
        mEnvConfigs[1].sustain_level = q::lin_to_db(static_cast<float>(value / 100.0));
        for (auto& voice : mVoices)
        {
          if (voice.active)
          {
            voice.envs[1].sustain_level(q::lin_to_db(static_cast<float>(value / 100.0)));
          }
        }
        break;
      case kParamOsc2Release:
        mOscParams[1].releaseMs = static_cast<float>(value);
        mEnvConfigs[1].release_rate = q::duration{value * 0.001};
        for (auto& voice : mVoices)
        {
          if (voice.active)
          {
            voice.envs[1].release_rate(q::duration{value * 0.001}, mSampleRate);
          }
        }
        break;
      
      // Oscillator 3 parameters
      case kParamOsc3Volume:
        mOscParams[2].volume = static_cast<float>(value / 100.0);
        break;
      case kParamOsc3Detune:
        mOscParams[2].detune = static_cast<float>(value);
        for (auto& voice : mVoices)
        {
          if (voice.active && voice.baseFrequency > 0.0f)
          {
            float octaveMultiplier = std::pow(2.0f, static_cast<float>(mOscParams[2].octave));
            float detuneRatio = std::pow(2.0f, mOscParams[2].detune / 1200.0f);
            float oscFreq = voice.baseFrequency * octaveMultiplier * detuneRatio;
            voice.phases[2].set(q::frequency{oscFreq}, mSampleRate);
          }
        }
        break;
      case kParamOsc3Octave:
        mOscParams[2].octave = static_cast<int>(value);
        for (auto& voice : mVoices)
        {
          if (voice.active && voice.baseFrequency > 0.0f)
          {
            float octaveMultiplier = std::pow(2.0f, static_cast<float>(mOscParams[2].octave));
            float detuneRatio = std::pow(2.0f, mOscParams[2].detune / 1200.0f);
            float oscFreq = voice.baseFrequency * octaveMultiplier * detuneRatio;
            voice.phases[2].set(q::frequency{oscFreq}, mSampleRate);
          }
        }
        break;
      case kParamOsc3Pan:
        mOscParams[2].pan = static_cast<float>(value);
        break;
      case kParamOsc3Waveform:
        mOscParams[2].waveform = static_cast<int>(value);
        break;
      case kParamOsc3Attack:
        mOscParams[2].attackMs = static_cast<float>(value);
        mEnvConfigs[2].attack_rate = q::duration{value * 0.001};
        for (auto& voice : mVoices)
        {
          if (voice.active)
          {
            voice.envs[2].attack_rate(q::duration{value * 0.001}, mSampleRate);
          }
        }
        break;
      case kParamOsc3Decay:
        mOscParams[2].decayMs = static_cast<float>(value);
        mEnvConfigs[2].decay_rate = q::duration{value * 0.001};
        for (auto& voice : mVoices)
        {
          if (voice.active)
          {
            voice.envs[2].decay_rate(q::duration{value * 0.001}, mSampleRate);
          }
        }
        break;
      case kParamOsc3Sustain:
        mOscParams[2].sustainLevel = static_cast<float>(value / 100.0);
        mEnvConfigs[2].sustain_level = q::lin_to_db(static_cast<float>(value / 100.0));
        for (auto& voice : mVoices)
        {
          if (voice.active)
          {
            voice.envs[2].sustain_level(q::lin_to_db(static_cast<float>(value / 100.0)));
          }
        }
        break;
      case kParamOsc3Release:
        mOscParams[2].releaseMs = static_cast<float>(value);
        mEnvConfigs[2].release_rate = q::duration{value * 0.001};
        for (auto& voice : mVoices)
        {
          if (voice.active)
          {
            voice.envs[2].release_rate(q::duration{value * 0.001}, mSampleRate);
          }
        }
        break;
      
      // Filter parameters
            case kParamFilterCutoff:
        mFilterCutoff = static_cast<float>(value);
        for (auto& voice : mVoices)
        {
          if (voice.active)
          {
            voice.filter.config(q::frequency{mFilterCutoff}, mSampleRate, mFilterQ);
          }
        }
        break;
      case kParamFilterResonance:
        mFilterQ = static_cast<float>(value);
        for (auto& voice : mVoices)
        {
          if (voice.active)
          {
            voice.filter.config(q::frequency{mFilterCutoff}, mSampleRate, mFilterQ);
          }
        }
        break;
      
      default:
        break;
    }
  }
  
  void SetOscParam(int oscIndex, int paramType, double value)
  {
    if (oscIndex < 0 || oscIndex >= kNumOscillators)
      return;
    
    auto& osc = mOscParams[oscIndex];
    
    // Parameter types: 0=volume, 1=detune, 2=octave, 3=pan, 4=waveform,
    //                  5=attack, 6=decay, 7=sustain, 8=release
    switch (paramType)
    {
      case 0: // Volume
        osc.volume = static_cast<float>(value);
        break;
      case 1: // Detune (cents)
        osc.detune = static_cast<float>(value);
        // Update active voices
        for (auto& voice : mVoices)
        {
          if (voice.active && voice.baseFrequency > 0.0f)
          {
            float octaveMultiplier = std::pow(2.0f, static_cast<float>(osc.octave));
            float detuneRatio = std::pow(2.0f, osc.detune / 1200.0f);
            float oscFreq = voice.baseFrequency * octaveMultiplier * detuneRatio;
            voice.phases[oscIndex].set(q::frequency{oscFreq}, mSampleRate);
          }
        }
        break;
      case 2: // Octave
        osc.octave = static_cast<int>(value);
        // Update active voices
        for (auto& voice : mVoices)
        {
          if (voice.active && voice.baseFrequency > 0.0f)
          {
            float octaveMultiplier = std::pow(2.0f, static_cast<float>(osc.octave));
            float detuneRatio = std::pow(2.0f, osc.detune / 1200.0f);
            float oscFreq = voice.baseFrequency * octaveMultiplier * detuneRatio;
            voice.phases[oscIndex].set(q::frequency{oscFreq}, mSampleRate);
          }
        }
        break;
      case 3: // Pan
        osc.pan = static_cast<float>(value);
        break;
      case 4: // Waveform
        osc.waveform = static_cast<int>(value);
        break;
      case 5: // Attack
        osc.attackMs = static_cast<float>(value);
        mEnvConfigs[oscIndex].attack_rate = q::duration{value * 0.001};
        for (auto& voice : mVoices)
        {
          if (voice.active)
          {
            voice.envs[oscIndex].attack_rate(q::duration{value * 0.001}, mSampleRate);
          }
        }
        break;
      case 6: // Decay
        osc.decayMs = static_cast<float>(value);
        mEnvConfigs[oscIndex].decay_rate = q::duration{value * 0.001};
        for (auto& voice : mVoices)
        {
          if (voice.active)
          {
            voice.envs[oscIndex].decay_rate(q::duration{value * 0.001}, mSampleRate);
          }
        }
        break;
      case 7: // Sustain
        osc.sustainLevel = static_cast<float>(value);
        {
          float sustainLin = static_cast<float>(value);
          mEnvConfigs[oscIndex].sustain_level = q::lin_to_db(sustainLin);
          for (auto& voice : mVoices)
          {
            if (voice.active)
            {
              voice.envs[oscIndex].sustain_level(q::lin_to_db(sustainLin));
            }
          }
        }
        break;
      case 8: // Release
        osc.releaseMs = static_cast<float>(value);
        mEnvConfigs[oscIndex].release_rate = q::duration{value * 0.001};
        for (auto& voice : mVoices)
        {
          if (voice.active)
          {
            voice.envs[oscIndex].release_rate(q::duration{value * 0.001}, mSampleRate);
          }
        }
        break;
    }
  }
  
  void SetFilterCutoff(float cutoff)
  {
    mFilterCutoff = cutoff;
    for (auto& voice : mVoices)
    {
      if (voice.active)
      {
        voice.filter.config(q::frequency{mFilterCutoff}, mSampleRate, mFilterQ);
      }
    }
  }
  
  void SetFilterQ(float filterQ)
  {
    mFilterQ = filterQ;
    for (auto& voice : mVoices)
    {
      if (voice.active)
      {
        voice.filter.config(q::frequency{mFilterCutoff}, mSampleRate, mFilterQ);
      }
    }
  }

private:
  static constexpr int kMaxVoices = 8;

    struct Voice
  {
    std::array<q::phase_iterator, kNumOscillators> phases;
    std::array<q::adsr_envelope_gen, kNumOscillators> envs{
      q::adsr_envelope_gen{q::adsr_envelope_gen::config{}, 44100.0f},
      q::adsr_envelope_gen{q::adsr_envelope_gen::config{}, 44100.0f},
      q::adsr_envelope_gen{q::adsr_envelope_gen::config{}, 44100.0f}
    };
    q::lowpass filter{8000_Hz, 44100.0f, 0.707};
    float baseFrequency = 0.0f;
    int noteNumber = -1;
    float velocity = 0.0f;
    bool active = false;
    std::array<bool, kNumOscillators> oscReleasing = {false, false, false};

    // Voice stealing crossfade (5ms fade-out)
    bool isBeingStolen = false;
    q::lin_downward_ramp_gen stealFade{5_ms, 44100.0f};

    // Pending note (activated after steal fade completes)
    int pendingNote = -1;
    float pendingFrequency = 0.0f;
    float pendingVelocity = 0.0f;

                        void Reset(std::array<q::adsr_envelope_gen::config, kNumOscillators> const& configs, float sps, float cutoff, float filterQ)
    {
      for (int i = 0; i < kNumOscillators; ++i)
      {
        phases[i] = q::phase_iterator{};
        envs[i] = q::adsr_envelope_gen{configs[i], sps};
        oscReleasing[i] = false;
      }
      filter = q::lowpass{q::frequency{cutoff}, sps, filterQ};
      baseFrequency = 0.0f;
      noteNumber = -1;
      velocity = 0.0f;
      active = false;

      // Reset voice stealing state
      isBeingStolen = false;
      stealFade = q::lin_downward_ramp_gen{5_ms, sps};
      pendingNote = -1;
      pendingFrequency = 0.0f;
      pendingVelocity = 0.0f;
    }

                        void Activate(int note, float freq, float vel, float sps,
                  std::array<OscillatorParams, kNumOscillators> const& oscParams,
                  float cutoff, float filterQ)
    {
      // Check if this is a retrigger of the same note (keep phase for continuity)
      bool isRetrigger = (active && noteNumber == note);

      baseFrequency = freq;
      noteNumber = note;
      velocity = vel;
      active = true;

      // Clear any pending steal state
      isBeingStolen = false;
      pendingNote = -1;

      for (int i = 0; i < kNumOscillators; ++i)
      {
        // Calculate frequency with octave and detune
        float octaveMultiplier = std::pow(2.0f, static_cast<float>(oscParams[i].octave));
        float detuneRatio = std::pow(2.0f, oscParams[i].detune / 1200.0f);
        float oscFreq = baseFrequency * octaveMultiplier * detuneRatio;

        if (isRetrigger)
        {
          // Same note retriggered - keep phase position, just update frequency
          phases[i].set(q::frequency{oscFreq}, sps);
        }
        else
        {
          // New note - create fresh phase iterator
          phases[i] = q::phase_iterator{q::frequency{oscFreq}, sps};
        }

        // Create envelope config from current oscillator ADSR values
        q::adsr_envelope_gen::config envConfig{
          q::duration{oscParams[i].attackMs * 0.001f},
          q::duration{oscParams[i].decayMs * 0.001f},
          q::lin_to_db(oscParams[i].sustainLevel),
          50_s,
          q::duration{oscParams[i].releaseMs * 0.001f}
        };
        envs[i] = q::adsr_envelope_gen{envConfig, sps};
        envs[i].attack();
        oscReleasing[i] = false;
      }
      filter = q::lowpass{q::frequency{cutoff}, sps, filterQ};
    }

            void Release()
    {
      for (int i = 0; i < kNumOscillators; ++i)
      {
        envs[i].release();
        oscReleasing[i] = true;
      }
    }
    
    bool AllEnvelopesIdle() const
    {
      for (int i = 0; i < kNumOscillators; ++i)
      {
        if (!envs[i].in_idle_phase())
          return false;
      }
      return true;
    }
  };

    std::array<Voice, kMaxVoices> mVoices;
  std::array<OscillatorParams, kNumOscillators> mOscParams;
  std::array<q::adsr_envelope_gen::config, kNumOscillators> mEnvConfigs;
  T mGain = static_cast<T>(0.8);
  float mFilterCutoff = 8000.0f;
  float mFilterQ = 0.707f;
  float mSampleRate = 44100.0f;
  int mNextVoice = 0;

    void UpdateEnvelopeConfigs()
  {
    // Update envelope configs for all inactive voices
    // Active voices keep their current envelopes until they're recycled
    for (auto& voice : mVoices)
    {
      if (!voice.active)
      {
        for (int i = 0; i < kNumOscillators; ++i)
        {
          voice.envs[i] = q::adsr_envelope_gen{mEnvConfigs[i], mSampleRate};
        }
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

    int AllocateVoice(int requestedNote)
  {
    int bestVoice = -1;
    float lowestAmp = 2.0f; // Start above max possible

    // Priority 1: Find an inactive voice
    for (int i = 0; i < kMaxVoices; ++i)
    {
      if (!mVoices[i].active)
        return i;
    }

    // Priority 2: Find same note (retrigger) - musically expected
    for (int i = 0; i < kMaxVoices; ++i)
    {
      if (mVoices[i].noteNumber == requestedNote)
        return i;
    }

    // Priority 3: Find releasing voice with lowest amplitude
    for (int i = 0; i < kMaxVoices; ++i)
    {
      bool anyReleasing = false;
      for (int osc = 0; osc < kNumOscillators; ++osc)
      {
        if (mVoices[i].oscReleasing[osc])
        {
          anyReleasing = true;
          break;
        }
      }
      if (anyReleasing)
      {
        // Get max envelope amplitude across oscillators
        float maxAmp = 0.0f;
        for (int osc = 0; osc < kNumOscillators; ++osc)
        {
          float envAmp = mVoices[i].envs[osc]();
          maxAmp = std::max(maxAmp, envAmp);
        }
        if (maxAmp < lowestAmp)
        {
          lowestAmp = maxAmp;
          bestVoice = i;
        }
      }
    }
    if (bestVoice >= 0) return bestVoice;

    // Priority 4: Steal voice with lowest amplitude overall
    lowestAmp = 2.0f;
    for (int i = 0; i < kMaxVoices; ++i)
    {
      float maxAmp = 0.0f;
      for (int osc = 0; osc < kNumOscillators; ++osc)
      {
        float envAmp = mVoices[i].envs[osc]();
        maxAmp = std::max(maxAmp, envAmp);
      }
      if (maxAmp < lowestAmp)
      {
        lowestAmp = maxAmp;
        bestVoice = i;
      }
    }

    return (bestVoice >= 0) ? bestVoice : 0; // Fallback to voice 0
  }

        void ActivateVoice(int noteNumber, float frequency, float level)
  {
    int voiceIndex = FindVoiceByNote(noteNumber);

    if (voiceIndex < 0)
    {
      voiceIndex = AllocateVoice(noteNumber);
    }

    auto& voice = mVoices[voiceIndex];

    // Check if voice is currently playing (needs crossfade steal)
    if (voice.active && voice.noteNumber != noteNumber && !voice.isBeingStolen)
    {
      // Voice is playing a different note - initiate crossfade steal
      voice.isBeingStolen = true;
      voice.stealFade.reset();
      voice.pendingNote = noteNumber;
      voice.pendingFrequency = frequency;
      voice.pendingVelocity = level;
    }
    else
    {
      // Voice is free, or same note retrigger, or already being stolen - activate directly
      voice.Activate(noteNumber, frequency, level, mSampleRate, mOscParams, mFilterCutoff, mFilterQ);
    }
  }

  void ReleaseVoice(int noteNumber)
  {
    const int voiceIndex = FindVoiceByNote(noteNumber);

    if (voiceIndex < 0)
      return;

    mVoices[voiceIndex].Release();
  }
};
