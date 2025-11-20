#pragma once

#include "Smoothers.h"

using namespace iplug;

enum EModulations
{
  kModGainSmoother = 0,
  kModDelayTimeSmoother = 1,
  kModDelayFeedbackSmoother = 2,
  kModDelayDrySmoother = 3,
  kModDelayWetSmoother = 4,
  kNumModulations,
};

template<typename T>
class TemplateProject2DSP
{
public:
  TemplateProject2DSP() = default;

  void ProcessBlock(T** inputs, T** outputs, int nOutputs, int nFrames)
  {
    if (!outputs || nOutputs < 1)
      return;

    mParamSmoother.ProcessBlock(mParamsToSmooth, mModulations.GetList(), nFrames);

    const T* gainBlock = mModulations.GetList()[kModGainSmoother];
    const T* timeBlock = mModulations.GetList()[kModDelayTimeSmoother];
    const T* fbBlock   = mModulations.GetList()[kModDelayFeedbackSmoother];
    const T* dryBlock  = mModulations.GetList()[kModDelayDrySmoother];
    const T* wetBlock  = mModulations.GetList()[kModDelayWetSmoother];

    const T currentDelayMs = timeBlock[0];
    int baseDelaySamples = static_cast<int>(currentDelayMs * mSampleRate / 1000.0);
    const int maxDelaySamples = static_cast<int>(mSampleRate * 4.0);
    if (baseDelaySamples < 1)
      baseDelaySamples = 1;
    if (baseDelaySamples > maxDelaySamples)
      baseDelaySamples = maxDelaySamples;

    if (baseDelaySamples > mBufferSize)
    {
      mBufferSize = baseDelaySamples;
      mDelayBufferL.Resize(mBufferSize);
      mDelayBufferR.Resize(mBufferSize);
      memset(mDelayBufferL.Get(), 0, mBufferSize * sizeof(T));
      memset(mDelayBufferR.Get(), 0, mBufferSize * sizeof(T));
      mWriteIndex = 0;
    }

    T* delayL = mDelayBufferL.Get();
    T* delayR = mDelayBufferR.Get();
    if (!delayL || !delayR)
      return;

    for (int s = 0; s < nFrames; ++s)
    {
      const T g   = gainBlock[s];
      const T fb  = fbBlock[s];
      const T dry = dryBlock[s];
      const T wet = wetBlock[s];

      const T slowPhase = static_cast<T>(mLfoPhase);
      const T lfoValPrimary = static_cast<T>(0.5 * (1.0 + std::sin(2.0 * M_PI * slowPhase)));
      const T lfoValSecondary = static_cast<T>(0.5 * (1.0 + std::sin(2.0 * M_PI * (slowPhase * 1.37 + 0.23))));
      const T lfoBlend = static_cast<T>(0.6) * lfoValPrimary + static_cast<T>(0.4) * lfoValSecondary;

      const T modAmount = static_cast<T>(0.45);
      T modDelayMs = timeBlock[s] * (static_cast<T>(0.8) + modAmount * lfoBlend);
      T modDelaySamples = modDelayMs * static_cast<T>(mSampleRate / 1000.0);
      if (modDelaySamples < 1)
        modDelaySamples = 1;
      if (modDelaySamples > static_cast<T>(maxDelaySamples - 2))
        modDelaySamples = static_cast<T>(maxDelaySamples - 2);

      const T readIndexF = static_cast<T>(mWriteIndex) - modDelaySamples;
      int idxA = static_cast<int>(std::floor(readIndexF));
      int idxB = idxA + 1;
      const T frac = readIndexF - static_cast<T>(idxA);

      auto wrap = [this](int i) -> int
      {
        while (i < 0) i += mBufferSize;
        while (i >= mBufferSize) i -= mBufferSize;
        return i;
      };

      idxA = wrap(idxA);
      idxB = wrap(idxB);

      const T dLA = delayL[idxA];
      const T dLB = delayL[idxB];
      const T dRA = delayR[idxA];
      const T dRB = delayR[idxB];

      T delayedL = dLA + (dLB - dLA) * frac;
      T delayedR = dRA + (dRB - dRA) * frac;

      const T shimmerMix = static_cast<T>(0.25);
      const T shimmerGain = static_cast<T>(0.4);
      const T upOctL = SoftSat(delayedL * static_cast<T>(2.0));
      const T upOctR = SoftSat(delayedR * static_cast<T>(2.0));
      delayedL = (static_cast<T>(1.0) - shimmerMix) * delayedL + shimmerMix * upOctL * shimmerGain;
      delayedR = (static_cast<T>(1.0) - shimmerMix) * delayedR + shimmerMix * upOctR * shimmerGain;

      const T toneMix = static_cast<T>(0.7);
      const T prevTL = mToneStateL;
      const T prevTR = mToneStateR;
      mToneStateL = prevTL + static_cast<T>(0.03) * (delayedL - prevTL);
      mToneStateR = prevTR + static_cast<T>(0.03) * (delayedR - prevTR);
      const T tonedL = toneMix * mToneStateL + (static_cast<T>(1.0) - toneMix) * delayedL;
      const T tonedR = toneMix * mToneStateR + (static_cast<T>(1.0) - toneMix) * delayedR;

      const T width = static_cast<T>(1.35);
      const T midL = (tonedL + tonedR) * static_cast<T>(0.5);
      const T sideL = (tonedL - tonedR) * static_cast<T>(0.5) * width;
      const T wideL = midL + sideL;
      const T wideR = midL - sideL;

      T inL = 0.0;
      T inR = 0.0;
      if (inputs && inputs[0])
      {
        inL = inputs[0][s];
        inR = (nOutputs > 1 && inputs[1]) ? inputs[1][s] : inputs[0][s];
      }

      inL *= g;
      inR *= g;

      const T cross = static_cast<T>(0.32);
      const T fbInL = inL + fb * (wideL * (static_cast<T>(1.0) - cross) + wideR * cross);
      const T fbInR = inR + fb * (wideR * (static_cast<T>(1.0) - cross) + wideL * cross);

      delayL[mWriteIndex] = fbInL;
      delayR[mWriteIndex] = fbInR;

      const T smear = static_cast<T>(0.18);
      const int smearIndex = wrap(mWriteIndex - 3);
      delayL[smearIndex] = delayL[smearIndex] * (static_cast<T>(1.0) - smear) + fbInL * smear;
      delayR[smearIndex] = delayR[smearIndex] * (static_cast<T>(1.0) - smear) + fbInR * smear;

      const T outL = dry * inL + wet * wideL;
      const T outR = dry * inR + wet * wideR;

      outputs[0][s] = outL;
      if (nOutputs > 1)
        outputs[1][s] = outR;

      ++mWriteIndex;
      if (mWriteIndex >= mBufferSize)
        mWriteIndex = 0;

      mLfoPhase += mLfoIncrement;
      if (mLfoPhase >= 1.0)
        mLfoPhase -= 1.0;
    }
  }

  void Reset(double sampleRate, int blockSize)
  {
    mModulationsData.Resize(blockSize * kNumModulations);
    mModulations.Empty();

    for (int i = 0; i < kNumModulations; ++i)
      mModulations.Add(mModulationsData.Get() + (blockSize * i));

    mSampleRate = sampleRate;
    mBlockSize = blockSize;

    mParamsToSmooth[kModGainSmoother]          = static_cast<T>(1.0);
    mParamsToSmooth[kModDelayTimeSmoother]     = static_cast<T>(900.0);
    mParamsToSmooth[kModDelayFeedbackSmoother] = static_cast<T>(0.70);
    mParamsToSmooth[kModDelayDrySmoother]      = static_cast<T>(0.25);
    mParamsToSmooth[kModDelayWetSmoother]      = static_cast<T>(0.75);

    mBufferSize = static_cast<int>(sampleRate * 4.0);
    if (mBufferSize < 1)
      mBufferSize = 1;

    mDelayBufferL.Resize(mBufferSize);
    mDelayBufferR.Resize(mBufferSize);
    memset(mDelayBufferL.Get(), 0, mBufferSize * sizeof(T));
    memset(mDelayBufferR.Get(), 0, mBufferSize * sizeof(T));

    mWriteIndex = 0;
    mToneStateL = 0;
    mToneStateR = 0;

    mLfoPhase = 0.0;
    const double lfoFreq = 0.09;
    mLfoIncrement = lfoFreq / mSampleRate;
  }

  void SetParam(int paramIdx, double value)
  {
    switch (paramIdx)
    {
      case kParamGain:
        mParamsToSmooth[kModGainSmoother] = static_cast<T>(value / 100.0);
        break;
      case kParamDelayTime:
        mParamsToSmooth[kModDelayTimeSmoother] = static_cast<T>(value);
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
      default:
        break;
    }
  }

private:
  static inline T SoftSat(T x)
  {
    const T drive = static_cast<T>(0.9);
    const T xd = x * drive;
    return xd / (static_cast<T>(1.0) + std::abs(xd));
  }

public:
  WDL_TypedBuf<T> mModulationsData;
  WDL_PtrList<T> mModulations;
  LogParamSmooth<T, kNumModulations> mParamSmoother;
  sample mParamsToSmooth[kNumModulations]{};

  WDL_TypedBuf<T> mDelayBufferL;
  WDL_TypedBuf<T> mDelayBufferR;
  int mBufferSize = 0;
  int mWriteIndex = 0;

  T mToneStateL = 0;
  T mToneStateR = 0;

  double mSampleRate = 44100.0;
  int mBlockSize = 64;

  double mLfoPhase = 0.0;
  double mLfoIncrement = 0.0;
};
