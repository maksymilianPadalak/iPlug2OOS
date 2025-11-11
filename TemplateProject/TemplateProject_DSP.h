#pragma once

#include "MidiSynth.h"
#include "Oscillator.h"
#include "ADSREnvelope.h"
#include "Smoothers.h"
#include "LFO.h"
#include "SVF.h"
#include "delay_line.h"
#include "verbengine.h"

using namespace iplug;

enum EModulations
{
  kModGainSmoother = 0,
  kModSustainSmoother,
  kModLFO,
  kModLFO2Filter,
  kModFilterCutoffSmoother,
  kModDelayTimeSmoother,
  kModDelayFeedbackSmoother,
  kModDelayDrySmoother,
  kModDelayWetSmoother,
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
        mOSC3.Reset(); 
        mOSC4.Reset(); 
      })
    , mFilterEnv("filter", nullptr, false)
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
      mOSC3.Reset();
      mOSC4.Reset();
      mOsc1Phase = 0.0;
      mOsc2Phase = 0.0;
      mOsc3Phase = 0.0;
      mOsc4Phase = 0.0;
      
      if(isRetrigger)
      {
        mAMPEnv.Retrigger(level);
        mFilterEnv.Retrigger(level);
      }
      else
      {
        mAMPEnv.Start(level);
        mFilterEnv.Start(level);
      }
    }
    
    void Release() override
    {
      mAMPEnv.Release();
      mFilterEnv.Release();
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
    
    // Get phase from oscillator for waveform generation
    inline double GetOscPhase(FastSinOscillator<T>& osc)
    {
      // Access the phase member through the base class
      // Since we can't directly access mPhase, we'll track it ourselves
      return osc.mPhase;
    }

    void ProcessSamplesAccumulating(T** inputs, T** outputs, int nInputs, int nOutputs, int startIdx, int nFrames) override
    {
      double pitch = mInputs[kVoiceControlPitch].endValue;
      double pitchBend = mInputs[kVoiceControlPitchBend].endValue;

      // Get note number for keytracking
      int noteNumber = static_cast<int>(std::round((pitch + 1.0) * 12.0 + 69.0));
      
      // Write timbre buffer for potential future use
      mInputs[kVoiceControlTimbre].Write(mTimbreBuffer.Get(), startIdx, nFrames);
      
      // Calculate base frequency
      double baseFreq = 440. * pow(2., pitch + pitchBend + inputs[kModLFO][0]);
      
      // Process each sample
      for(auto i = startIdx; i < startIdx + nFrames; i++)
      {
        // Calculate oscillator frequencies with detune and octave
        double osc1Freq = baseFreq * pow(2., mOsc1Octave + mOsc1Detune / 1200.0);
        double osc2Freq = baseFreq * pow(2., mOsc2Octave + mOsc2Detune / 1200.0);
        double osc3Freq = baseFreq * pow(2., mOsc3Octave + mOsc3Detune / 1200.0);
        double osc4Freq = baseFreq * pow(2., mOsc4Octave + mOsc4Detune / 1200.0);
        
        // Sync oscillator 2 to oscillator 1 if enabled
        if(mSyncEnabled)
        {
          osc2Freq = osc1Freq * mSyncRatio;
        }
        
        // Process oscillators - track phase for waveform generation
        // For sine wave, use Process directly; for others, track phase manually
        T osc1Sample, osc2Sample, osc3Sample, osc4Sample;
        
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
        
        if(mOsc3Wave == 0) {
          osc3Sample = mOSC3.Process(osc3Freq);
        } else {
          mOsc3Phase += osc3Freq / mSampleRate;
          while(mOsc3Phase >= 1.0) mOsc3Phase -= 1.0;
          osc3Sample = WaveformFromPhase(mOsc3Phase, mOsc3Wave);
        }
        
        if(mOsc4Wave == 0) {
          osc4Sample = mOSC4.Process(osc4Freq);
        } else {
          mOsc4Phase += osc4Freq / mSampleRate;
          while(mOsc4Phase >= 1.0) mOsc4Phase -= 1.0;
          osc4Sample = WaveformFromPhase(mOsc4Phase, mOsc4Wave);
        }
        
        // Mix oscillators
        T oscMix = (osc1Sample * mOsc1Mix + osc2Sample * mOsc2Mix + osc3Sample * mOsc3Mix + osc4Sample * mOsc4Mix) / 4.0;
        
        // Calculate filter cutoff with envelope and keytracking
        T filterEnvValue = mFilterEnv.Process(inputs[kModSustainSmoother][i]);
        double filterCutoff = static_cast<double>(inputs[kModFilterCutoffSmoother][i]); // Use smoothed cutoff
        filterCutoff += filterEnvValue * mFilterEnvAmount;
        filterCutoff += (noteNumber - 69) * mFilterKeytrack;
        filterCutoff += inputs[kModLFO2Filter][i] * mFilterLFODepth;
        filterCutoff = std::max(20.0, std::min(20000.0, filterCutoff));
        
        mFilter.SetFreqCPS(filterCutoff);
        mFilter.SetQ(mFilterResonance);
        
        // Apply filter
        T filterInput[1] = {oscMix};
        T filterOutput[1];
        T* filterInputPtrs[1] = {filterInput};
        T* filterOutputPtrs[1] = {filterOutput};
        mFilter.ProcessBlock(filterInputPtrs, filterOutputPtrs, 1, 1);
        
        // Apply amplitude envelope
        T envValue = mAMPEnv.Process(inputs[kModSustainSmoother][i]);
        T output = filterOutput[0] * envValue * mGain;
        
        outputs[0][i] += output;
        outputs[1][i] += output;
      }
    }

    void SetSampleRateAndBlockSize(double sampleRate, int blockSize) override
    {
      mOSC1.SetSampleRate(sampleRate);
      mOSC2.SetSampleRate(sampleRate);
      mOSC3.SetSampleRate(sampleRate);
      mOSC4.SetSampleRate(sampleRate);
      mAMPEnv.SetSampleRate(sampleRate);
      mFilterEnv.SetSampleRate(sampleRate);
      mFilter.SetSampleRate(sampleRate);
      
      mTimbreBuffer.Resize(blockSize);
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
    FastSinOscillator<T> mOSC3;
    FastSinOscillator<T> mOSC4;
    ADSREnvelope<T> mAMPEnv;
    ADSREnvelope<T> mFilterEnv;
    SVF<T, 1> mFilter {SVF<T, 1>::kLowPass, 1000.0};

    // Oscillator parameters
    T mOsc1Mix = 1.0;
    T mOsc2Mix = 0.0;
    T mOsc3Mix = 0.0;
    T mOsc4Mix = 0.0;
    T mOsc1Detune = 0.0;
    T mOsc2Detune = 0.0;
    T mOsc3Detune = 0.0;
    T mOsc4Detune = 0.0;
    int mOsc1Octave = 0;
    int mOsc2Octave = 0;
    int mOsc3Octave = 0;
    int mOsc4Octave = 0;
    int mOsc1Wave = 0; // 0=sine, 1=saw, 2=square, 3=triangle
    int mOsc2Wave = 0;
    int mOsc3Wave = 0;
    int mOsc4Wave = 0;
    
    // Filter parameters
    double mFilterCutoffBase = 1000.0;
    double mFilterResonance = 1.0;
    double mFilterEnvAmount = 0.0;
    double mFilterKeytrack = 0.0;
    double mFilterLFODepth = 0.0;
    
    // Sync parameters
    bool mSyncEnabled = false;
    double mSyncRatio = 1.0;
    
    // Phase tracking for non-sine waveforms
    double mOsc1Phase = 0.0;
    double mOsc2Phase = 0.0;
    double mOsc3Phase = 0.0;
    double mOsc4Phase = 0.0;
    double mSampleRate = 44100.0;

  private:
    WDL_TypedBuf<float> mTimbreBuffer;

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

    // some MidiSynth API examples:
    // mSynth.SetKeyToPitchFn([](int k){return (k - 69.)/24.;}); // quarter-tone scale
    // mSynth.SetNoteGlideTime(0.5); // portamento
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
    
    // Process LFOs
    mLFO.ProcessBlock(mModulations.GetList()[kModLFO], nFrames, qnPos, transportIsRunning, tempo);
    mLFO2.ProcessBlock(mModulations.GetList()[kModLFO2Filter], nFrames, qnPos, transportIsRunning, tempo);
    
    // Process synth voices
    mSynth.ProcessBlock(mModulations.GetList(), outputs, 0, nOutputs, nFrames);
    
    // Apply gain and delay
    for(int s=0; s < nFrames; s++)
    {
      T smoothedGain = mModulations.GetList()[kModGainSmoother][s];
      T delayDry = mModulations.GetList()[kModDelayDrySmoother][s];
      T delayWet = mModulations.GetList()[kModDelayWetSmoother][s];
      T delayFeedback = mModulations.GetList()[kModDelayFeedbackSmoother][s];
      
      // Apply gain
      T dryL = outputs[0][s] * smoothedGain;
      T dryR = outputs[1][s] * smoothedGain;
      
      // Delay processing with feedback
      T delayInputL = dryL + mDelayFeedbackL * delayFeedback;
      T delayInputR = dryR + mDelayFeedbackR * delayFeedback;
      
      // Write to delay lines
      mDelayLineL.add_pairs(&delayInputL, 1);
      mDelayLineR.add_pairs(&delayInputR, 1);
      
      // Read from delay lines - peek from read pointer offset by delay time
      T delayOutputL = 0.0, delayOutputR = 0.0;
      int availL = mDelayLineL.get_avail_pairs();
      int availR = mDelayLineR.get_avail_pairs();
      
      if(availL >= mDelayTimeSamples && mDelayTimeSamples > 0) {
        mDelayLineL.peek_pairs(&delayOutputL, 1, mDelayTimeSamples - 1);
      }
      if(availR >= mDelayTimeSamples && mDelayTimeSamples > 0) {
        mDelayLineR.peek_pairs(&delayOutputR, 1, mDelayTimeSamples - 1);
      }
      
      // Store feedback
      mDelayFeedbackL = delayOutputL;
      mDelayFeedbackR = delayOutputR;
      
      // Mix dry and wet for delay (separate controls)
      outputs[0][s] = dryL * delayDry + delayOutputL * delayWet;
      outputs[1][s] = dryR * delayDry + delayOutputR * delayWet;
    }
    
    // Process reverb (processes entire block)
    if(nOutputs >= 2)
    {
      WDL_TypedBuf<double> reverbInputL, reverbInputR, reverbWetL, reverbWetR;
      reverbInputL.Resize(nFrames);
      reverbInputR.Resize(nFrames);
      reverbWetL.Resize(nFrames);
      reverbWetR.Resize(nFrames);
      
      // Copy delay output to reverb input (preserve original signal)
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
      
      // Mix dry and wet for reverb (separate controls)
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
    mLFO.SetSampleRate(sampleRate);
    mLFO2.SetSampleRate(sampleRate);
    
    // Initialize reverb engine
    mReverbEngine.SetSampleRate(sampleRate);
    mReverbEngine.SetRoomSize(0.5);
    mReverbEngine.SetDampening(0.5);
    mReverbEngine.SetWidth(0.0);
    // Reset reverb buffers - this must be called after setting sample rate
    mReverbEngine.Reset(true); // Reset with clearing buffers
    
    // Initialize delay lines
    int maxDelaySamples = static_cast<int>(sampleRate * 2.0); // 2 second max delay
    mDelayLineL.set_nch_length(1, maxDelaySamples, maxDelaySamples);
    mDelayLineR.set_nch_length(1, maxDelaySamples, maxDelaySamples);
    
    mModulationsData.Resize(blockSize * kNumModulations);
    mModulations.Empty();
    
    for(int i = 0; i < kNumModulations; i++)
    {
      mModulations.Add(mModulationsData.Get() + (blockSize * i));
    }
    
    mSampleRate = sampleRate;
    mBlockSize = blockSize;
    mDelayTimeSamples = static_cast<int>(sampleRate * 0.25); // Default 250ms delay
    mDelayFeedbackL = 0.0;
    mDelayFeedbackR = 0.0;
    
    // Initialize filter cutoff smoother
    mParamsToSmooth[kModFilterCutoffSmoother] = static_cast<T>(1000.0);
    mParamsToSmooth[kModDelayFeedbackSmoother] = static_cast<T>(0.0);
    mParamsToSmooth[kModDelayDrySmoother] = static_cast<T>(1.0); // 100% dry by default
    mParamsToSmooth[kModDelayWetSmoother] = static_cast<T>(0.0); // 0% wet by default
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
      case kParamNoteGlideTime:
        mSynth.SetNoteGlideTime(value / 1000.);
        break;
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
      case kParamLFODepth:
        mLFO.SetScalar(value / 100.);
        break;
      case kParamLFORateTempo:
        mLFO.SetQNScalarFromDivision(static_cast<int>(value));
        break;
      case kParamLFORateHz:
        mLFO.SetFreqCPS(value);
        break;
      case kParamLFORateMode:
        mLFO.SetRateMode(value > 0.5);
        break;
      case kParamLFOShape:
        mLFO.SetShape(static_cast<int>(value));
        break;
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
      case kParamOsc3Mix:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          dynamic_cast<TemplateProjectDSP::Voice&>(voice).mOsc3Mix = static_cast<T>(value / 100.0);
        });
        break;
      case kParamOsc4Mix:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          dynamic_cast<TemplateProjectDSP::Voice&>(voice).mOsc4Mix = static_cast<T>(value / 100.0);
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
      case kParamOsc3Detune:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          dynamic_cast<TemplateProjectDSP::Voice&>(voice).mOsc3Detune = static_cast<T>(value);
        });
        break;
      case kParamOsc4Detune:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          dynamic_cast<TemplateProjectDSP::Voice&>(voice).mOsc4Detune = static_cast<T>(value);
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
      case kParamOsc3Octave:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          dynamic_cast<TemplateProjectDSP::Voice&>(voice).mOsc3Octave = static_cast<int>(value);
        });
        break;
      case kParamOsc4Octave:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          dynamic_cast<TemplateProjectDSP::Voice&>(voice).mOsc4Octave = static_cast<int>(value);
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
      case kParamOsc3Wave:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          dynamic_cast<TemplateProjectDSP::Voice&>(voice).mOsc3Wave = static_cast<int>(value);
        });
        break;
      case kParamOsc4Wave:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          dynamic_cast<TemplateProjectDSP::Voice&>(voice).mOsc4Wave = static_cast<int>(value);
        });
        break;
      // Filter parameters
      case kParamFilterCutoff:
        mParamsToSmooth[kModFilterCutoffSmoother] = static_cast<T>(value);
        break;
      case kParamFilterResonance:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          dynamic_cast<TemplateProjectDSP::Voice&>(voice).mFilterResonance = value;
        });
        break;
      case kParamFilterEnvAmount:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          dynamic_cast<TemplateProjectDSP::Voice&>(voice).mFilterEnvAmount = value;
        });
        break;
      case kParamFilterKeytrack:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          dynamic_cast<TemplateProjectDSP::Voice&>(voice).mFilterKeytrack = value;
        });
        break;
      case kParamFilterAttack:
      case kParamFilterDecay:
      case kParamFilterRelease:
      {
        EEnvStage stage = static_cast<EEnvStage>(EEnvStage::kAttack + (paramIdx - kParamFilterAttack));
        mSynth.ForEachVoice([stage, value](SynthVoice& voice) {
          dynamic_cast<TemplateProjectDSP::Voice&>(voice).mFilterEnv.SetStageTime(stage, value);
        });
        break;
      }
      case kParamFilterSustain:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          dynamic_cast<TemplateProjectDSP::Voice&>(voice).mFilterEnv.SetStageTime(ADSREnvelope<T>::kSustain, value / 100.0);
        });
        break;
      // LFO2 parameters
      case kParamLFO2Depth:
        mLFO2.SetScalar(value / 100.);
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          dynamic_cast<TemplateProjectDSP::Voice&>(voice).mFilterLFODepth = value;
        });
        break;
      case kParamLFO2RateTempo:
        mLFO2.SetQNScalarFromDivision(static_cast<int>(value));
        break;
      case kParamLFO2RateHz:
        mLFO2.SetFreqCPS(value);
        break;
      case kParamLFO2RateMode:
        mLFO2.SetRateMode(value > 0.5);
        break;
      case kParamLFO2Shape:
        mLFO2.SetShape(static_cast<int>(value));
        break;
      // Delay parameters
      case kParamDelayTime:
        mDelayTimeSamples = static_cast<int>((value / 1000.0) * mSampleRate);
        mDelayTimeSamples = std::max(1, mDelayTimeSamples);
        break;
      case kParamDelayFeedback:
        mParamsToSmooth[kModDelayFeedbackSmoother] = static_cast<T>(value / 100.0);
        break;
      case kParamDelayDry:
        mParamsToSmooth[kModDelayDrySmoother] = static_cast<T>(value / 100.0);
        break;
      case kParamDelayWet:
        mParamsToSmooth[kModDelayWetSmoother] = static_cast<T>(value / 100.0);
        break;
      // Sync parameters
      case kParamOscSync:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          dynamic_cast<TemplateProjectDSP::Voice&>(voice).mSyncEnabled = value > 0.5;
        });
        break;
      case kParamOscSyncRatio:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          dynamic_cast<TemplateProjectDSP::Voice&>(voice).mSyncRatio = value;
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
  LFO<T> mLFO;
  LFO<T> mLFO2;
  
  // Delay state
  WDL_DelayLine<T> mDelayLineL;
  WDL_DelayLine<T> mDelayLineR;
  int mDelayTimeSamples = 0;
  T mDelayFeedbackL = 0.0;
  T mDelayFeedbackR = 0.0;
  
  // Reverb engine
  WDL_ReverbEngine mReverbEngine;
  
  double mSampleRate = 44100.0;
  int mBlockSize = 64;
};
