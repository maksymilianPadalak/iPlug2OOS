#pragma once

#include "MidiSynth.h"
#include "Oscillator.h"
#include "ADSREnvelope.h"
#include "Smoothers.h"
#include "verbengine.h"

using namespace iplug;

enum EModulations
{
  kModGainSmoother = 0,
  kModSustainSmoother,
  kModReverbDrySmoother,
  kModReverbWetSmoother,
  kNumModulations,
};

template<typename T>
class TemplateProjectDSP
{
public:
#pragma mark - Voice
  class Voice : public SynthVoice
  {
  public:
    Voice()
    : mAMPEnv("gain", [&](){ 
        mOSC1.Reset(); 
        mOSC2.Reset(); 
      })
    {
    }

    bool GetBusy() const override
    {
      return mAMPEnv.GetBusy();
    }

    void Trigger(double level, bool isRetrigger) override
    {
      mOSC1.Reset();
      mOSC2.Reset();
      mOsc1Phase = 0.0;
      mOsc2Phase = 0.0;
      
      if(isRetrigger)
      {
        mAMPEnv.Retrigger(level);
      }
      else
      {
        mAMPEnv.Start(level);
      }
    }
    
    void Release() override
    {
      mAMPEnv.Release();
    }

    // Generate waveform from phase (0-1)
    inline T WaveformFromPhase(double phase, int waveType)
    {
      // Wrap phase to [0, 1)
      while(phase >= 1.0) phase -= 1.0;
      while(phase < 0.0) phase += 1.0;
      
      switch(waveType)
      {
        case 0: // Sine - use FastSinOscillator lookup
          return FastSinOscillator<T>::Lookup(phase * 2.0 * PI);
        case 1: // Saw - ramp from -1 to 1
          return static_cast<T>((phase * 2.0) - 1.0);
        case 2: // Square - 1 for first half, -1 for second half
          return phase < 0.5 ? static_cast<T>(1.0) : static_cast<T>(-1.0);
        case 3: // Triangle - ramp up then down
          return phase < 0.5 ? static_cast<T>(phase * 4.0 - 1.0) : static_cast<T>(3.0 - phase * 4.0);
        default:
          return FastSinOscillator<T>::Lookup(phase * 2.0 * PI);
      }
    }

    void ProcessSamplesAccumulating(T** inputs, T** outputs, int nInputs, int nOutputs, int startIdx, int nFrames) override
    {
      double pitch = mInputs[kVoiceControlPitch].endValue;
      double pitchBend = mInputs[kVoiceControlPitchBend].endValue;
      
      // Calculate base frequency
      double baseFreq = 440. * pow(2., pitch + pitchBend);
      
      // Process each sample
      for(auto i = startIdx; i < startIdx + nFrames; i++)
      {
        // Calculate oscillator frequencies with detune and octave
        double osc1Freq = baseFreq * pow(2., mOsc1Octave + mOsc1Detune / 1200.0);
        double osc2Freq = baseFreq * pow(2., mOsc2Octave + mOsc2Detune / 1200.0);
        
        // Process oscillators - track phase for waveform generation
        T osc1Sample, osc2Sample;
        
        if(mOsc1Wave == 0) {
          osc1Sample = mOSC1.Process(osc1Freq);
        } else {
          // Update phase manually for non-sine waveforms
          mOsc1Phase += osc1Freq / mSampleRate;
          while(mOsc1Phase >= 1.0) mOsc1Phase -= 1.0;
          osc1Sample = WaveformFromPhase(mOsc1Phase, mOsc1Wave);
        }
        
        if(mOsc2Wave == 0) {
          osc2Sample = mOSC2.Process(osc2Freq);
        } else {
          mOsc2Phase += osc2Freq / mSampleRate;
          while(mOsc2Phase >= 1.0) mOsc2Phase -= 1.0;
          osc2Sample = WaveformFromPhase(mOsc2Phase, mOsc2Wave);
        }
        
        // Mix oscillators
        T oscMix = (osc1Sample * mOsc1Mix + osc2Sample * mOsc2Mix) / 2.0;
        
        // Apply amplitude envelope
        T envValue = mAMPEnv.Process(inputs[kModSustainSmoother][i]);
        T output = oscMix * envValue * mGain;
        
        outputs[0][i] += output;
        outputs[1][i] += output;
      }
    }

    void SetSampleRateAndBlockSize(double sampleRate, int blockSize) override
    {
      mOSC1.SetSampleRate(sampleRate);
      mOSC2.SetSampleRate(sampleRate);
      mAMPEnv.SetSampleRate(sampleRate);
      mSampleRate = sampleRate;
    }

    void SetProgramNumber(int pgm) override
    {
      //TODO:
    }

    void SetControl(int controlNumber, float value) override
    {
      //TODO:
    }

  public:
    FastSinOscillator<T> mOSC1;
    FastSinOscillator<T> mOSC2;
    ADSREnvelope<T> mAMPEnv;

    // Oscillator parameters
    T mOsc1Mix = 1.0;
    T mOsc2Mix = 0.0;
    T mOsc1Detune = 0.0;
    T mOsc2Detune = 0.0;
    int mOsc1Octave = 0;
    int mOsc2Octave = 0;
    int mOsc1Wave = 0; // 0=sine, 1=saw, 2=square, 3=triangle
    int mOsc2Wave = 0;
    
    // Gain
    T mGain = 1.0;
    
    // Phase tracking for non-sine waveforms
    double mOsc1Phase = 0.0;
    double mOsc2Phase = 0.0;
    double mSampleRate = 44100.0;

  private:
    uint32_t mRandSeed = 0;
    
    float Rand()
    {
      mRandSeed = mRandSeed * 0x0019660D + 0x3C6EF35F;
      uint32_t temp = ((mRandSeed >> 9) & 0x007FFFFF) | 0x3F800000;
      return (*reinterpret_cast<float*>(&temp))*2.f - 3.f;
    }

  };

public:
#pragma mark -
  TemplateProjectDSP(int nVoices)
  {
    for (auto i = 0; i < nVoices; i++)
    {
      // add a voice to Zone 0.
      mSynth.AddVoice(new Voice(), 0);
    }
  }

  void ProcessBlock(T** inputs, T** outputs, int nOutputs, int nFrames, double qnPos = 0., bool transportIsRunning = false, double tempo = 120.)
  {
    // clear outputs
    for(auto i = 0; i < nOutputs; i++)
    {
      memset(outputs[i], 0, nFrames * sizeof(T));
    }
    
    // Process parameter smoothers
    mParamSmoother.ProcessBlock(mParamsToSmooth, mModulations.GetList(), nFrames);
    
    // Process synth voices
    mSynth.ProcessBlock(mModulations.GetList(), outputs, 0, nOutputs, nFrames);
    
    // Apply gain
    for(int s=0; s < nFrames; s++)
    {
      T smoothedGain = mModulations.GetList()[kModGainSmoother][s];
      
      // Apply gain
      T dryL = outputs[0][s] * smoothedGain;
      T dryR = outputs[1][s] * smoothedGain;
      
      outputs[0][s] = dryL;
      outputs[1][s] = dryR;
    }
    
    // Process reverb (processes entire block)
    if(nOutputs >= 2)
    {
      WDL_TypedBuf<double> reverbInputL, reverbInputR, reverbWetL, reverbWetR;
      reverbInputL.Resize(nFrames);
      reverbInputR.Resize(nFrames);
      reverbWetL.Resize(nFrames);
      reverbWetR.Resize(nFrames);
      
      // Copy output to reverb input
      for(int s = 0; s < nFrames; s++)
      {
        reverbInputL.Get()[s] = static_cast<double>(outputs[0][s]);
        reverbInputR.Get()[s] = static_cast<double>(outputs[1][s]);
      }
      
      // Process reverb - writes wet signal to reverbWetL/R, doesn't modify inputs
      mReverbEngine.ProcessSampleBlock(
        reverbInputL.Get(), reverbInputR.Get(),
        reverbWetL.Get(), reverbWetR.Get(),
        nFrames
      );
      
      // Mix dry and wet for reverb
      for(int s = 0; s < nFrames; s++)
      {
        T reverbDry = mModulations.GetList()[kModReverbDrySmoother][s];
        T reverbWet = mModulations.GetList()[kModReverbWetSmoother][s];
        
        T dryL = static_cast<T>(reverbInputL.Get()[s]);
        T wetL = static_cast<T>(reverbWetL.Get()[s]);
        outputs[0][s] = dryL * reverbDry + wetL * reverbWet;
        
        T dryR = static_cast<T>(reverbInputR.Get()[s]);
        T wetR = static_cast<T>(reverbWetR.Get()[s]);
        outputs[1][s] = dryR * reverbDry + wetR * reverbWet;
      }
    }
  }

  void Reset(double sampleRate, int blockSize)
  {
    mSynth.SetSampleRateAndBlockSize(sampleRate, blockSize);
    mSynth.Reset();
    
    // Initialize reverb engine
    mReverbEngine.SetSampleRate(sampleRate);
    mReverbEngine.SetRoomSize(0.5);
    mReverbEngine.SetDampening(0.5);
    mReverbEngine.SetWidth(0.0);
    // Reset reverb buffers - this must be called after setting sample rate
    mReverbEngine.Reset(true); // Reset with clearing buffers
    
    mModulationsData.Resize(blockSize * kNumModulations);
    mModulations.Empty();
    
    for(int i = 0; i < kNumModulations; i++)
    {
      mModulations.Add(mModulationsData.Get() + (blockSize * i));
    }
    
    mSampleRate = sampleRate;
    mBlockSize = blockSize;
    
    // Initialize parameter smoothers
    mParamsToSmooth[kModGainSmoother] = static_cast<T>(1.0);
    mParamsToSmooth[kModSustainSmoother] = static_cast<T>(0.5);
    mParamsToSmooth[kModReverbDrySmoother] = static_cast<T>(1.0); // 100% dry by default
    mParamsToSmooth[kModReverbWetSmoother] = static_cast<T>(0.0); // 0% wet by default
  }

  void ProcessMidiMsg(const IMidiMsg& msg)
  {
    mSynth.AddMidiMsgToQueue(msg);
  }

  void SetParam(int paramIdx, double value)
  {
    using EEnvStage = ADSREnvelope<sample>::EStage;
    
    switch (paramIdx) {
      case kParamGain:
        mParamsToSmooth[kModGainSmoother] = (T) value / 100.;
        break;
      case kParamSustain:
        mParamsToSmooth[kModSustainSmoother] = (T) value / 100.;
        break;
      case kParamAttack:
      case kParamDecay:
      case kParamRelease:
      {
        EEnvStage stage = static_cast<EEnvStage>(EEnvStage::kAttack + (paramIdx - kParamAttack));
        mSynth.ForEachVoice([stage, value](SynthVoice& voice) {
          dynamic_cast<TemplateProjectDSP::Voice&>(voice).mAMPEnv.SetStageTime(stage, value);
        });
        break;
      }
      // Oscillator parameters
      case kParamOsc1Mix:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          dynamic_cast<TemplateProjectDSP::Voice&>(voice).mOsc1Mix = static_cast<T>(value / 100.0);
        });
        break;
      case kParamOsc2Mix:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          dynamic_cast<TemplateProjectDSP::Voice&>(voice).mOsc2Mix = static_cast<T>(value / 100.0);
        });
        break;
      case kParamOsc1Detune:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          dynamic_cast<TemplateProjectDSP::Voice&>(voice).mOsc1Detune = static_cast<T>(value);
        });
        break;
      case kParamOsc2Detune:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          dynamic_cast<TemplateProjectDSP::Voice&>(voice).mOsc2Detune = static_cast<T>(value);
        });
        break;
      case kParamOsc1Octave:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          dynamic_cast<TemplateProjectDSP::Voice&>(voice).mOsc1Octave = static_cast<int>(value);
        });
        break;
      case kParamOsc2Octave:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          dynamic_cast<TemplateProjectDSP::Voice&>(voice).mOsc2Octave = static_cast<int>(value);
        });
        break;
      case kParamOsc1Wave:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          dynamic_cast<TemplateProjectDSP::Voice&>(voice).mOsc1Wave = static_cast<int>(value);
        });
        break;
      case kParamOsc2Wave:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          dynamic_cast<TemplateProjectDSP::Voice&>(voice).mOsc2Wave = static_cast<int>(value);
        });
        break;
      // Reverb parameters
      case kParamReverbRoomSize:
        mReverbEngine.SetRoomSize(0.3 + value * 0.69); // 0.3 to 0.99
        break;
      case kParamReverbDamp:
        mReverbEngine.SetDampening(value);
        break;
      case kParamReverbWidth:
        mReverbEngine.SetWidth(value * 2.0 - 1.0); // 0-1 to -1 to 1
        break;
      case kParamReverbDry:
        mParamsToSmooth[kModReverbDrySmoother] = static_cast<T>(value / 100.0);
        break;
      case kParamReverbWet:
        mParamsToSmooth[kModReverbWetSmoother] = static_cast<T>(value / 100.0);
        break;
      default:
        break;
    }
  }
  
public:
  MidiSynth mSynth { VoiceAllocator::kPolyModePoly, MidiSynth::kDefaultBlockSize };
  WDL_TypedBuf<T> mModulationsData; // Sample data for global modulations (e.g. smoothed sustain)
  WDL_PtrList<T> mModulations; // Ptrlist for global modulations
  LogParamSmooth<T, kNumModulations> mParamSmoother;
  sample mParamsToSmooth[kNumModulations];
  
  // Reverb engine
  WDL_ReverbEngine mReverbEngine;
  
  double mSampleRate = 44100.0;
  int mBlockSize = 64;
};
