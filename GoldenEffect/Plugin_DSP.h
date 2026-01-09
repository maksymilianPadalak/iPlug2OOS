#pragma once

#include <q/support/literals.hpp>
#include <q/fx/lowpass.hpp>
#include <q/fx/delay.hpp>
#include <q/fx/biquad.hpp>
#include <cmath>
#include <array>
#include <algorithm>

using namespace iplug;
using namespace cycfi::q::literals;

static constexpr int kMaxDelaySamples = 384000; // 2 seconds at 192kHz max sample rate
static constexpr float kMinDelayMs = 1.0f;
static constexpr float kMaxDelayMs = 2000.0f;
static constexpr int kNumCombFilters = 8;
static constexpr int kNumAllpassFilters = 4;
static constexpr int kMaxPreDelaySamples = 19200; // 100ms at 192kHz

struct CombFilter
{
  std::array<float, 9600> buffer{};
  int writeIndex = 0;
  int delaySamples = 1000;
  float feedback = 0.7f;
  float dampState = 0.0f;
  float damping = 0.5f;

  void setDelay(int samples)
  {
    delaySamples = std::clamp(samples, 1, static_cast<int>(buffer.size()) - 1);
  }

  float process(float input)
  {
    int readIndex = writeIndex - delaySamples;
    if (readIndex < 0)
      readIndex += static_cast<int>(buffer.size());

    float output = buffer[readIndex];
    dampState = output * (1.0f - damping) + dampState * damping;
    buffer[writeIndex] = input + dampState * feedback + 1e-15f;
    writeIndex = (writeIndex + 1) % static_cast<int>(buffer.size());
    return output;
  }

    void clear()
  {
    std::fill(buffer.begin(), buffer.end(), 0.0f);
    dampState = 0.0f;
  }
};

struct AllpassFilter
{
  std::array<float, 2400> buffer{};
  int writeIndex = 0;
  int delaySamples = 500;
  static constexpr float feedback = 0.5f;

  void setDelay(int samples)
  {
    delaySamples = std::clamp(samples, 1, static_cast<int>(buffer.size()) - 1);
  }

  float process(float input)
  {
    int readIndex = writeIndex - delaySamples;
    if (readIndex < 0)
      readIndex += static_cast<int>(buffer.size());

    float bufOut = buffer[readIndex];
    float output = -input + bufOut;
    buffer[writeIndex] = input + bufOut * feedback + 1e-15f;
    writeIndex = (writeIndex + 1) % static_cast<int>(buffer.size());
    return output;
  }

  void clear()
  {
    std::fill(buffer.begin(), buffer.end(), 0.0f);
  }
};

template<typename T>
class PluginInstanceDSP
{
public:
    PluginInstanceDSP() = default;

  void SetTempo(double tempo, bool transportRunning)
  {
    mHostTempo = static_cast<float>(std::max(20.0, std::min(300.0, tempo)));
    mTransportRunning = transportRunning;
  }

  void ProcessBlock(T** inputs, T** outputs, int nInputs, int nOutputs, int nFrames)
  {
    if (!inputs || !outputs || nOutputs < 1 || nInputs < 1 || !inputs[0])
      return;

        T* inL = inputs[0];
    T* inR = (nInputs > 1 && inputs[1]) ? inputs[1] : inputs[0];

    const float targetTimeMs = mSyncEnabled ? GetSyncedDelayMs() : mDelayTimeMs;

        for (int s = 0; s < nFrames; ++s)
    {
      const float smoothedTimeMs = mTimeSmoother(targetTimeMs);
      const float delaySamples = (smoothedTimeMs / 1000.0f) * mSampleRate;

      const float smoothedFeedback = mFeedbackSmoother(mDelayFeedback);
            const float smoothedDry = mDrySmoother(mDelayDry);
      const float smoothedWet = mWetSmoother(mDelayWet);

      const float gain = mGainSmoother(mTargetGain);

      const T dryL = inL[s] * static_cast<T>(gain);
      const T dryR = inR[s] * static_cast<T>(gain);

      const float wetL = mDelayL(delaySamples);
      const float wetR = mDelayR(delaySamples);

      mDelayL.push(static_cast<float>(dryL) + wetL * smoothedFeedback + 1e-15f);
      mDelayR.push(static_cast<float>(dryR) + wetR * smoothedFeedback + 1e-15f);

                        const T mixedL = dryL * smoothedDry + static_cast<T>(wetL) * smoothedWet;
      const T mixedR = dryR * smoothedDry + static_cast<T>(wetR) * smoothedWet;

      const float reverbOnSmoothed = mReverbOnSmoother(mReverbOn);
      const float reverbMixSmoothed = mReverbMixSmoother(mReverbMix);
      T finalL = mixedL;
      T finalR = mixedR;
      if (reverbOnSmoothed > 0.001f)
      {
        auto [revWetL, revWetR] = ProcessReverb(static_cast<float>(mixedL), static_cast<float>(mixedR));
        const float dryReverb = 1.0f - reverbMixSmoothed * reverbOnSmoothed;
        const float wetReverb = reverbMixSmoothed * reverbOnSmoothed;
        finalL = mixedL * dryReverb + static_cast<T>(revWetL) * wetReverb;
        finalR = mixedR * dryReverb + static_cast<T>(revWetR) * wetReverb;
      }

      const T outL = std::clamp(finalL, static_cast<T>(-1.0), static_cast<T>(1.0));
      const T outR = std::clamp(finalR, static_cast<T>(-1.0), static_cast<T>(1.0));

      outputs[0][s] = outL;
      if (nOutputs > 1)
        outputs[1][s] = outR;
    }
  }

    void Reset(double sampleRate, int /*blockSize*/)
  {
    mSampleRate = static_cast<float>(sampleRate);

    // Recreate delay lines with new sample rate
    mDelayL = cycfi::q::delay{2_s, mSampleRate};
    mDelayR = cycfi::q::delay{2_s, mSampleRate};

            // Recreate all smoothers with new sample rate
    mGainSmoother = cycfi::q::one_pole_lowpass{100_Hz, mSampleRate};
    mTimeSmoother = cycfi::q::one_pole_lowpass{10_Hz, mSampleRate};
    mFeedbackSmoother = cycfi::q::one_pole_lowpass{50_Hz, mSampleRate};
    mDrySmoother = cycfi::q::one_pole_lowpass{50_Hz, mSampleRate};
    mWetSmoother = cycfi::q::one_pole_lowpass{50_Hz, mSampleRate};
    mReverbMixSmoother = cycfi::q::one_pole_lowpass{50_Hz, mSampleRate};
    mReverbOnSmoother = cycfi::q::one_pole_lowpass{20_Hz, mSampleRate};

                // Prime smoothers with current target values to prevent startup artifacts
    mGainSmoother(mTargetGain);
    mTimeSmoother(mDelayTimeMs);
    mFeedbackSmoother(mDelayFeedback);
    mDrySmoother(mDelayDry);
    mWetSmoother(mDelayWet);
    mReverbMixSmoother(mReverbMix);
    mReverbOnSmoother(mReverbOn);

    // Reset feedback state
    mFeedbackStateL = 0.0f;
    mFeedbackStateR = 0.0f;

    // Clear reverb state
    for (auto& comb : mCombsL) comb.clear();
    for (auto& comb : mCombsR) comb.clear();
    for (auto& ap : mAllpassL) ap.clear();
    for (auto& ap : mAllpassR) ap.clear();
    std::fill(mPreDelayL.begin(), mPreDelayL.end(), 0.0f);
    std::fill(mPreDelayR.begin(), mPreDelayR.end(), 0.0f);
    mPreDelayWriteIdx = 0;

    // Recreate low-cut filters with new sample rate
    mLowCutL = cycfi::q::highpass{cycfi::q::frequency{mReverbLowCutHz}, mSampleRate, 0.707f};
    mLowCutR = cycfi::q::highpass{cycfi::q::frequency{mReverbLowCutHz}, mSampleRate, 0.707f};

    // Configure reverb with new sample rate
    ConfigureReverb();
  }

        void SetParam(int paramIdx, double value)
  {
    switch (paramIdx)
    {
      case kParamGain:
        mTargetGain = static_cast<float>(value / 100.0);
        break;
      case kParamDelayTime:
        mDelayTimeMs = static_cast<float>(std::clamp(value, static_cast<double>(kMinDelayMs), static_cast<double>(kMaxDelayMs)));
        break;
      case kParamDelayFeedback:
        mDelayFeedback = static_cast<float>(value / 100.0) * 0.95f;
        break;
            case kParamDelayDry:
        mDelayDry = static_cast<float>(value / 100.0);
        break;
      case kParamDelayWet:
        mDelayWet = static_cast<float>(value / 100.0);
        break;
      case kParamDelaySync:
        mSyncEnabled = (value > 0.5);
        break;
            case kParamDelayDivision:
        mDivisionIndex = static_cast<int>(value);
        if (mDivisionIndex < 0) mDivisionIndex = 0;
        if (mDivisionIndex > 7) mDivisionIndex = 7;
        break;
      case kParamReverbOn:
        mReverbOn = (value > 0.5) ? 1.0f : 0.0f;
        break;
      case kParamReverbMix:
        mReverbMix = static_cast<float>(value / 100.0);
        break;
      case kParamReverbSize:
        mReverbSize = static_cast<float>(value / 100.0);
        ConfigureReverb();
        break;
      case kParamReverbDecay:
        mReverbDecay = static_cast<float>(value / 100.0);
        ConfigureReverb();
        break;
      case kParamReverbDamping:
        mReverbDamping = static_cast<float>(value / 100.0);
        ConfigureReverb();
        break;
      case kParamReverbPreDelay:
        mReverbPreDelayMs = static_cast<float>(value);
        ConfigureReverb();
        break;
      case kParamReverbWidth:
        mReverbWidth = static_cast<float>(value / 100.0);
        break;
      case kParamReverbLowCut:
        mReverbLowCutHz = static_cast<float>(value);
        mLowCutL.config(cycfi::q::frequency{mReverbLowCutHz}, mSampleRate, 0.707f);
        mLowCutR.config(cycfi::q::frequency{mReverbLowCutHz}, mSampleRate, 0.707f);
        break;
      default:
        break;
    }
  }

private:
    float GetSyncedDelayMs() const
  {
    const float beatMs = 60000.0f / mHostTempo;
    const float divisions[] = {4.0f, 2.0f, 1.0f, 0.5f, 0.25f, 0.125f, 0.6667f, 0.3333f};
    return std::clamp(beatMs * divisions[mDivisionIndex], kMinDelayMs, kMaxDelayMs);
  }

    void ConfigureReverb()
  {
    const float baseDelays[] = {1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617};
    const float allpassDelays[] = {556, 441, 341, 225};
    const float sizeScale = 0.5f + mReverbSize * 1.0f;
    const float decayFeedback = 0.7f + mReverbDecay * 0.28f;

    for (int i = 0; i < kNumCombFilters; ++i)
    {
      int delayL = static_cast<int>(baseDelays[i] * sizeScale * mSampleRate / 44100.0f);
      int delayR = static_cast<int>((baseDelays[i] + 23) * sizeScale * mSampleRate / 44100.0f);
      mCombsL[i].setDelay(delayL);
      mCombsR[i].setDelay(delayR);
      mCombsL[i].feedback = decayFeedback;
      mCombsR[i].feedback = decayFeedback;
      mCombsL[i].damping = mReverbDamping;
      mCombsR[i].damping = mReverbDamping;
    }

    for (int i = 0; i < kNumAllpassFilters; ++i)
    {
      int delay = static_cast<int>(allpassDelays[i] * mSampleRate / 44100.0f);
      mAllpassL[i].setDelay(delay);
      mAllpassR[i].setDelay(delay);
    }

    mPreDelaySamples = static_cast<int>((mReverbPreDelayMs / 1000.0f) * mSampleRate);
    mPreDelaySamples = std::clamp(mPreDelaySamples, 1, kMaxPreDelaySamples - 1);
  }

  std::pair<float, float> ProcessReverb(float inputL, float inputR)
  {
    int preReadIdx = mPreDelayWriteIdx - mPreDelaySamples;
    if (preReadIdx < 0)
      preReadIdx += kMaxPreDelaySamples;

    float preL = mPreDelayL[preReadIdx];
    float preR = mPreDelayR[preReadIdx];

    mPreDelayL[mPreDelayWriteIdx] = inputL;
    mPreDelayR[mPreDelayWriteIdx] = inputR;
    mPreDelayWriteIdx = (mPreDelayWriteIdx + 1) % kMaxPreDelaySamples;

    preL = mLowCutL(preL);
    preR = mLowCutR(preR);

    float combOutL = 0.0f, combOutR = 0.0f;
    for (int i = 0; i < kNumCombFilters; ++i)
    {
      combOutL += mCombsL[i].process(preL);
      combOutR += mCombsR[i].process(preR);
    }
    combOutL /= static_cast<float>(kNumCombFilters);
    combOutR /= static_cast<float>(kNumCombFilters);

    float apOutL = combOutL, apOutR = combOutR;
    for (int i = 0; i < kNumAllpassFilters; ++i)
    {
      apOutL = mAllpassL[i].process(apOutL);
      apOutR = mAllpassR[i].process(apOutR);
    }

    float midL = (apOutL + apOutR) * 0.5f;
    float sideL = (apOutL - apOutR) * 0.5f;
    float wetL = midL + sideL * mReverbWidth;
    float wetR = midL - sideL * mReverbWidth;

    return {wetL, wetR};
  }

  float mTargetGain = 1.0f;
  cycfi::q::one_pole_lowpass mGainSmoother{100_Hz, 44100.0f};

  // Delay lines (fractional delay from Q library)
  cycfi::q::delay mDelayL{2_s, 44100.0f};
  cycfi::q::delay mDelayR{2_s, 44100.0f};

    // Delay parameter targets
  float mDelayTimeMs = 250.0f;
  float mDelayFeedback = 0.3f;
  float mDelayDry = 1.0f;
  float mDelayWet = 0.25f;

  // Parameter smoothers
  cycfi::q::one_pole_lowpass mTimeSmoother{10_Hz, 44100.0f};
  cycfi::q::one_pole_lowpass mFeedbackSmoother{50_Hz, 44100.0f};
    cycfi::q::one_pole_lowpass mDrySmoother{50_Hz, 44100.0f};
  cycfi::q::one_pole_lowpass mWetSmoother{50_Hz, 44100.0f};

  // Reverb smoothers
  cycfi::q::one_pole_lowpass mReverbMixSmoother{50_Hz, 44100.0f};
  cycfi::q::one_pole_lowpass mReverbOnSmoother{20_Hz, 44100.0f};

  // Tempo sync members
  bool mSyncEnabled = false;
  int mDivisionIndex = 2;
  float mHostTempo = 120.0f;
  bool mTransportRunning = false;

  // Sample rate storage
  float mSampleRate = 44100.0f;

    // Feedback state storage
  float mFeedbackStateL = 0.0f;
  float mFeedbackStateR = 0.0f;

    // Reverb parameter members
  float mReverbOn = 0.0f;
  float mReverbMix = 0.25f;
  float mReverbSize = 0.5f;
  float mReverbDecay = 0.5f;
  float mReverbDamping = 0.5f;
  float mReverbPreDelayMs = 20.0f;
  float mReverbWidth = 1.0f;
  float mReverbLowCutHz = 80.0f;

  // Reverb processing members
  std::array<CombFilter, kNumCombFilters> mCombsL;
  std::array<CombFilter, kNumCombFilters> mCombsR;
  std::array<AllpassFilter, kNumAllpassFilters> mAllpassL;
  std::array<AllpassFilter, kNumAllpassFilters> mAllpassR;

  // Pre-delay buffers
  std::array<float, kMaxPreDelaySamples> mPreDelayL{};
  std::array<float, kMaxPreDelaySamples> mPreDelayR{};
  int mPreDelayWriteIdx = 0;
  int mPreDelaySamples = 882;

  // Low-cut filter
  cycfi::q::highpass mLowCutL{80_Hz, 44100.0f, 0.707f};
  cycfi::q::highpass mLowCutR{80_Hz, 44100.0f, 0.707f};
};
