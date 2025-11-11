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
  kParamNoteGlideTime,
  kParamAttack,
  kParamDecay,
  kParamSustain,
  kParamRelease,
  kParamLFOShape,
  kParamLFORateHz,
  kParamLFORateTempo,
  kParamLFORateMode,
  kParamLFODepth,
  // Oscillators
  kParamOsc1Mix,
  kParamOsc2Mix,
  kParamOsc3Mix,
  kParamOsc4Mix,
  kParamOsc1Detune,
  kParamOsc2Detune,
  kParamOsc3Detune,
  kParamOsc4Detune,
  kParamOsc1Octave,
  kParamOsc2Octave,
  kParamOsc3Octave,
  kParamOsc4Octave,
  kParamOsc1Wave,
  kParamOsc2Wave,
  kParamOsc3Wave,
  kParamOsc4Wave,
  // Filter
  kParamFilterCutoff,
  kParamFilterResonance,
  kParamFilterEnvAmount,
  kParamFilterKeytrack,
  kParamFilterAttack,
  kParamFilterDecay,
  kParamFilterSustain,
  kParamFilterRelease,
  // LFO2
  kParamLFO2RateHz,
  kParamLFO2RateTempo,
  kParamLFO2RateMode,
  kParamLFO2Shape,
  kParamLFO2Depth,
  // Delay
  kParamDelayTime,
  kParamDelayFeedback,
  kParamDelayDry,
  kParamDelayWet,
  // Sync
  kParamOscSync,
  kParamOscSyncRatio,
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
  kCtrlTagLFOVis,
  kCtrlTagScope,
  kCtrlTagRTText,
  kCtrlTagKeyboard,
  kCtrlTagBender,
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
  ISender<1> mLFOVisSender;
  IPeakAvgSender<2> mMeterSender;
#endif
};
