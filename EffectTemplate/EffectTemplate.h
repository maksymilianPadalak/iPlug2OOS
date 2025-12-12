#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "ISender.h"
#ifndef NO_IGRAPHICS
#include "IControls.h"
#endif

const int kNumPresets = 1;

enum EParams
{
  kParamGain = 0,
  kParamMix,            // Dry/Wet mix 0-100%
  kParamDelayTime,      // Delay time in ms (0-1000)
  kParamDelayFeedback,  // Delay feedback 0-100%
  kParamReverbSize,     // Reverb room size 0-100%
  kParamReverbDamping,  // Reverb damping/tone 0-100%
  kParamReverbWidth,    // Stereo width 0-100%
  kParamBypass,         // Bypass effect
  kNumParams
};

#if IPLUG_DSP
#include "EffectTemplate_DSP.h"
#endif

enum EControlTags
{
  kCtrlTagMeter = 0,
  kNumCtrlTags
};

using namespace iplug;
#ifndef NO_IGRAPHICS
using namespace igraphics;
#endif

class EffectTemplate final : public Plugin
{
public:
  EffectTemplate(const InstanceInfo& info);

#if IPLUG_DSP
public:
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void ProcessMidiMsg(const IMidiMsg& msg) override;
  void OnReset() override;
  void OnParamChange(int paramIdx) override;
  void OnParamChangeUI(int paramIdx, EParamSource source) override;
  void OnIdle() override;
  bool OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData) override;

private:
  EffectTemplateDSP<sample> mDSP;
  IPeakAvgSender<2> mMeterSender;
#endif
};
