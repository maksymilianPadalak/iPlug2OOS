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
  class Voice : public SynthVoice
  {
  public:
    Voice()
    : mAMPEnv("gain", [&]() {
        mOSC1.Reset();
        mOSC2.Reset();
        mOSC3.Reset();
        mOSC4.Reset();
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
      mOSC3.Reset();
      mOSC4.Reset();
      mOsc1Phase = 0.0;
      mOsc2Phase = 0.0;
      mOsc3Phase = 0.0;
      mOsc4Phase = 0.0;

      if (isRetrigger)
        mAMPEnv.Retrigger(level);
      else
        mAMPEnv.Start(level);
    }

    void Release() override
    {
      mAMPEnv.Release();
    }

    inline T WaveformFromPhase(double phase, int waveType)
    {
      while (phase >= 1.0) phase -= 1.0;
      while (phase < 0.0) phase += 1.0;

      switch (waveType)
      {
        case 0:
          return FastSinOscillator<T>::Lookup(phase * 2.0 * PI);
        case 1:
          return static_cast<T>((phase * 2.0) - 1.0);
        case 2:
          return phase < 0.5 ? static_cast<T>(1.0) : static_cast<T>(-1.0);
        case 3:
          return phase < 0.5 ? static_cast<T>(phase * 4.0 - 1.0) : static_cast<T>(3.0 - phase * 4.0);
        default:
          return FastSinOscillator<T>::Lookup(phase * 2.0 * PI);
      }
    }

    void ProcessSamplesAccumulating(T** inputs, T** outputs, int nInputs, int nOutputs, int startIdx, int nFrames) override
    {
      const double pitch = mInputs[kVoiceControlPitch].endValue;
      const double pitchBend = mInputs[kVoiceControlPitchBend].endValue;

      mInputs[kVoiceControlTimbre].Write(mTimbreBuffer.Get(), startIdx, nFrames);

      const double baseFreq = 440. * pow(2., pitch + pitchBend);

      for (int i = startIdx; i < startIdx + nFrames; i++)
      {
        const double osc1Freq = baseFreq * pow(2., mOsc1Octave + mOsc1Detune / 1200.0);
        const double osc2Freq = baseFreq * pow(2., mOsc2Octave + mOsc2Detune / 1200.0);
        const double osc3Freq = baseFreq * pow(2., mOsc3Octave + mOsc3Detune / 1200.0);
        const double osc4Freq = baseFreq * pow(2., mOsc4Octave + mOsc4Detune / 1200.0);

        T osc1Sample, osc2Sample, osc3Sample, osc4Sample;

        if (mOsc1Wave == 0)
          osc1Sample = mOSC1.Process(osc1Freq);
        else
        {
          mOsc1Phase += osc1Freq / mSampleRate;
          while (mOsc1Phase >= 1.0) mOsc1Phase -= 1.0;
          osc1Sample = WaveformFromPhase(mOsc1Phase, mOsc1Wave);
        }

        if (mOsc2Wave == 0)
          osc2Sample = mOSC2.Process(osc2Freq);
        else
        {
          mOsc2Phase += osc2Freq / mSampleRate;
          while (mOsc2Phase >= 1.0) mOsc2Phase -= 1.0;
          osc2Sample = WaveformFromPhase(mOsc2Phase, mOsc2Wave);
        }

        if (mOsc3Wave == 0)
          osc3Sample = mOSC3.Process(osc3Freq);
        else
        {
          mOsc3Phase += osc3Freq / mSampleRate;
          while (mOsc3Phase >= 1.0) mOsc3Phase -= 1.0;
          osc3Sample = WaveformFromPhase(mOsc3Phase, mOsc3Wave);
        }

        if (mOsc4Wave == 0)
          osc4Sample = mOSC4.Process(osc4Freq);
        else
        {
          mOsc4Phase += osc4Freq / mSampleRate;
          while (mOsc4Phase >= 1.0) mOsc4Phase -= 1.0;
          osc4Sample = WaveformFromPhase(mOsc4Phase, mOsc4Wave);
        }

        const T oscMix = osc1Sample * mOsc1Mix + osc2Sample * mOsc2Mix + osc3Sample * mOsc3Mix + osc4Sample * mOsc4Mix;

        const T envValue = mAMPEnv.Process(inputs[kModSustainSmoother][i]);
        const T output = oscMix * envValue * mGain;

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

      mTimbreBuffer.Resize(blockSize);
      mSampleRate = sampleRate;
    }

    void SetProgramNumber(int pgm) override {}

    void SetControl(int controlNumber, float value) override {}

  public:
    FastSinOscillator<T> mOSC1;
    FastSinOscillator<T> mOSC2;
    FastSinOscillator<T> mOSC3;
    FastSinOscillator<T> mOSC4;
    ADSREnvelope<T> mAMPEnv;

    T mOsc1Mix = static_cast<T>(0.6);
    T mOsc2Mix = static_cast<T>(0.25);
    T mOsc3Mix = static_cast<T>(0.1);
    T mOsc4Mix = static_cast<T>(0.05);

    T mOsc1Detune = 0.0;
    T mOsc2Detune = static_cast<T>(7.0);
    T mOsc3Detune = static_cast<T>(-7.0);
    T mOsc4Detune = static_cast<T>(19.0);

    int mOsc1Octave = 0;
    int mOsc2Octave = 0;
    int mOsc3Octave = -1;
    int mOsc4Octave = 1;

    int mOsc1Wave = 1;
    int mOsc2Wave = 1;
    int mOsc3Wave = 1;
    int mOsc4Wave = 3;

    double mOsc1Phase = 0.0;
    double mOsc2Phase = 0.0;
    double mOsc3Phase = 0.0;
    double mOsc4Phase = 0.0;
    double mSampleRate = 44100.0;

    T mGain = static_cast<T>(1.0);

  private:
    WDL_TypedBuf<float> mTimbreBuffer;

    uint32_t mRandSeed = 0;

    float Rand()
    {
      mRandSeed = mRandSeed * 0x0019660D + 0x3C6EF35F;
      const uint32_t temp = ((mRandSeed >> 9) & 0x007FFFFF) | 0x3F800000;
      return (*reinterpret_cast<const float*>(&temp)) * 2.f - 3.f;
    }
  };

public:
  TemplateProjectDSP(int nVoices)
  {
    for (int i = 0; i < nVoices; i++)
      mSynth.AddVoice(new Voice(), 0);
  }

  void ProcessBlock(T** inputs, T** outputs, int nOutputs, int nFrames, double qnPos = 0., bool transportIsRunning = false, double tempo = 120.)
  {
    for (int i = 0; i < nOutputs; i++)
      memset(outputs[i], 0, nFrames * sizeof(T));

    mParamSmoother.ProcessBlock(mParamsToSmooth, mModulations.GetList(), nFrames);

    mSynth.ProcessBlock(mModulations.GetList(), outputs, 0, nOutputs, nFrames);

    for (int s = 0; s < nFrames; s++)
    {
      const T smoothedGain = mModulations.GetList()[kModGainSmoother][s];
      outputs[0][s] *= smoothedGain;
      outputs[1][s] *= smoothedGain;
    }

    if (nOutputs >= 2)
    {
      WDL_TypedBuf<double> reverbInputL, reverbInputR, reverbWetL, reverbWetR;
      reverbInputL.Resize(nFrames);
      reverbInputR.Resize(nFrames);
      reverbWetL.Resize(nFrames);
      reverbWetR.Resize(nFrames);

      for (int s = 0; s < nFrames; s++)
      {
        reverbInputL.Get()[s] = static_cast<double>(outputs[0][s]);
        reverbInputR.Get()[s] = static_cast<double>(outputs[1][s]);
      }

      mReverbEngine.ProcessSampleBlock(
        reverbInputL.Get(), reverbInputR.Get(),
        reverbWetL.Get(), reverbWetR.Get(),
        nFrames);

      for (int s = 0; s < nFrames; s++)
      {
        const T reverbDry = mModulations.GetList()[kModReverbDrySmoother][s];
        const T reverbWet = mModulations.GetList()[kModReverbWetSmoother][s];

        const T dryL = static_cast<T>(reverbInputL.Get()[s]);
        const T wetL = static_cast<T>(reverbWetL.Get()[s]);
        outputs[0][s] = dryL * reverbDry + wetL * reverbWet;

        const T dryR = static_cast<T>(reverbInputR.Get()[s]);
        const T wetR = static_cast<T>(reverbWetR.Get()[s]);
        outputs[1][s] = dryR * reverbDry + wetR * reverbWet;
      }
    }
  }

  void Reset(double sampleRate, int blockSize)
  {
    mSynth.SetSampleRateAndBlockSize(sampleRate, blockSize);
    mSynth.Reset();

    mReverbEngine.SetSampleRate(sampleRate);
    mReverbEngine.SetRoomSize(0.85);
    mReverbEngine.SetDampening(0.35);
    mReverbEngine.SetWidth(0.8);
    mReverbEngine.Reset(true);

    mModulationsData.Resize(blockSize * kNumModulations);
    mModulations.Empty();

    for (int i = 0; i < kNumModulations; i++)
      mModulations.Add(mModulationsData.Get() + (blockSize * i));

    mSampleRate = sampleRate;
    mBlockSize = blockSize;

    mParamsToSmooth[kModGainSmoother] = static_cast<T>(0.8);
    mParamsToSmooth[kModSustainSmoother] = static_cast<T>(0.7);
    mParamsToSmooth[kModReverbDrySmoother] = static_cast<T>(0.7);
    mParamsToSmooth[kModReverbWetSmoother] = static_cast<T>(0.3);
  }

  void ProcessMidiMsg(const IMidiMsg& msg)
  {
    mSynth.AddMidiMsgToQueue(msg);
  }

  void SetParam(int paramIdx, double value)
  {
    using EEnvStage = ADSREnvelope<sample>::EStage;

    switch (paramIdx)
    {
      case kParamGain:
        mParamsToSmooth[kModGainSmoother] = static_cast<T>(value / 100.0);
        break;
      case kParamSustain:
        mParamsToSmooth[kModSustainSmoother] = static_cast<T>(value / 100.0);
        break;
      case kParamAttack:
      case kParamDecay:
      case kParamRelease:
      {
        const EEnvStage stage = static_cast<EEnvStage>(EEnvStage::kAttack + (paramIdx - kParamAttack));
        mSynth.ForEachVoice([stage, value](SynthVoice& voice) {
          dynamic_cast<TemplateProjectDSP::Voice&>(voice).mAMPEnv.SetStageTime(stage, value);
        });
        break;
      }

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

      case kParamReverbRoomSize:
        mReverbEngine.SetRoomSize(value);
        break;
      case kParamReverbDamp:
        mReverbEngine.SetDampening(value / 100.0);
        break;
      case kParamReverbWidth:
        mReverbEngine.SetWidth(value * 2.0 - 1.0);
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
  WDL_TypedBuf<T> mModulationsData;
  WDL_PtrList<T> mModulations;
  LogParamSmooth<T, kNumModulations> mParamSmoother;
  sample mParamsToSmooth[kNumModulations];

  WDL_ReverbEngine mReverbEngine;

  double mSampleRate = 44100.0;
  int mBlockSize = 64;
};
