#pragma once

/**
 * GoldenEffect2 - Simple Gain Controller
 *
 * A minimal effect template demonstrating basic DSP structure.
 * Use this as a starting point for building more complex effects.
 */

#include "IPlug_include_in_plug_hdr.h"
#include "ISender.h"
#ifndef NO_IGRAPHICS
#include "IControls.h"
#endif

const int kNumPresets = 1;

// =============================================================================
// PARAMETERS
// =============================================================================
enum EParams
{
  kParamGain = 0,  // Output gain in dB (-60 to +12)
  kNumParams
};

#if IPLUG_DSP
#include "Plugin_DSP.h"
#endif

// =============================================================================
// CONTROL TAGS
// =============================================================================
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
  void OnReset() override;
  void OnParamChange(int paramIdx) override;
  void OnIdle() override;

private:
  PluginInstanceDSP<sample> mDSP;
  IPeakAvgSender<2> mMeterSender;
#endif
};
