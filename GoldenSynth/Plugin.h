#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "ISender.h"
#ifndef NO_IGRAPHICS
#include "IControls.h"
#endif

// ═══════════════════════════════════════════════════════════════════════════════
// SINGLE SOURCE OF TRUTH INCLUDES
// ═══════════════════════════════════════════════════════════════════════════════
// EParams enum is defined in Plugin_Params.h (single source of truth)
// Presets are defined in Plugin_PresetList.h (auto-counted, uses kParamXxx values)
// ═══════════════════════════════════════════════════════════════════════════════
#include "Plugin_Params.h"      // EParams enum - single source of truth
#include "Plugin_PresetList.h"  // Preset definitions - uses kParamXxx directly

// kNumPresets auto-derived from kPresetDefs[] array - see Plugin_PresetList.h
const int kNumPresets = kPresetCount;

#if IPLUG_DSP
#include "Plugin_DSP.h"
#endif

enum EControlTags
{
  kCtrlTagMeter = 0,
  kCtrlTagWaveform,
  kNumCtrlTags
};

enum EMsgTags
{
  kMsgTagRestorePreset = 0,
  kNumMsgTags
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
  void OnParamChange(int paramIdx, EParamSource source, int sampleOffset = -1) override;
  void OnParamChangeUI(int paramIdx, EParamSource source) override;
  void OnIdle() override;
  bool OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData) override;

private:
  PluginInstanceDSP<sample> mDSP;
  IPeakAvgSender<2> mMeterSender;
  IBufferSender<1> mWaveformSender;
#endif
};
