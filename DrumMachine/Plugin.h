#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "ISender.h"
#ifndef NO_IGRAPHICS
#include "IControls.h"
#endif

const int kNumPresets = 1;

enum EParams
{
  // Master
  kParamGain = 0,

  // Kick (MIDI note 48 / C3)
  kParamKickPitchStart,
  kParamKickPitchEnd,
  kParamKickPitchDecay,
  kParamKickAmpDecay,

  kNumParams
};

#if IPLUG_DSP
#include "Plugin_DSP.h"
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

class PluginInstance final : public iplug::Plugin
{
public:
  PluginInstance(const InstanceInfo& info);

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
  DrumMachineDSP<sample> mDSP;
  IPeakAvgSender<2> mMeterSender;
#endif
};
