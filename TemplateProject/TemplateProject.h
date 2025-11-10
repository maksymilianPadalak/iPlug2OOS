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
  kParamAttack,
  kParamDecay,
  kParamSustain,
  kParamRelease,
  // Oscillators
  kParamOsc1Mix,
  kParamOsc2Mix,
  kParamOsc1Detune,
  kParamOsc2Detune,
  kParamOsc1Octave,
  kParamOsc2Octave,
  kParamOsc1Wave,
  kParamOsc2Wave,
  // Reverb
  kParamReverbRoomSize,
  kParamReverbDamp,
  kParamReverbWidth,
  kParamReverbDry,
  kParamReverbWet,
  kNumParams
};

#if IPLUG_DSP
// will use EParams in TemplateProject_DSP.h
#include "TemplateProject_DSP.h"
#endif

enum EControlTags
{
  kCtrlTagMeter = 0,
  kCtrlTagKeyboard,
  kNumCtrlTags
};

using namespace iplug;
#ifndef NO_IGRAPHICS
using namespace igraphics;
#endif

class TemplateProject final : public Plugin
{
public:
  TemplateProject(const InstanceInfo& info);

#if IPLUG_DSP // http://bit.ly/2S64BDd
public:
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void ProcessMidiMsg(const IMidiMsg& msg) override;
  void OnReset() override;
  void OnParamChange(int paramIdx) override;
  void OnParamChangeUI(int paramIdx, EParamSource source) override;
  void OnIdle() override;
  bool OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData) override;

private:
  TemplateProjectDSP<sample> mDSP {16};
  IPeakAvgSender<2> mMeterSender;
#endif
};
