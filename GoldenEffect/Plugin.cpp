#include "Plugin.h"
#include "IPlug_include_in_plug_src.h"

/**
 * Plugin Constructor
 *
 * Initialize all parameters with sensible defaults.
 * Parameter ranges are chosen for musical usefulness.
 */
PluginInstance::PluginInstance(const InstanceInfo& info)
: iplug::Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  // ===========================================================================
  // MIX SECTION
  // ===========================================================================
  GetParam(kParamDry)->InitDouble("Dry", 100., 0., 100., 0.1, "%");
  GetParam(kParamWet)->InitDouble("Wet", 30., 0., 100., 0.1, "%");

  // ===========================================================================
  // CHARACTER SECTION
  // ===========================================================================
  GetParam(kParamSize)->InitDouble("Size", 70., 0., 100., 0.1, "%");
  GetParam(kParamDecay)->InitDouble("Decay", 70., 0., 99., 0.1, "%");
  GetParam(kParamPreDelay)->InitDouble("Pre-Delay", 10., 0., 200., 0.1, "ms");
  GetParam(kParamDiffusion)->InitDouble("Diffusion", 75., 0., 100., 0.1, "%");

  // ===========================================================================
  // TONE SECTION
  // ===========================================================================
  GetParam(kParamDamping)->InitDouble("Damping", 50., 0., 100., 0.1, "%");
  GetParam(kParamLowCut)->InitDouble("Low Cut", 80., 20., 500., 1., "Hz");
  GetParam(kParamHighCut)->InitDouble("High Cut", 8000., 1000., 20000., 10., "Hz");

  // ===========================================================================
  // MODULATION SECTION
  // ===========================================================================
  GetParam(kParamModRate)->InitDouble("Mod Rate", 0.5, 0.1, 2., 0.01, "Hz");
  GetParam(kParamModDepth)->InitDouble("Mod Depth", 50., 0., 100., 0.1, "%");

  // ===========================================================================
  // OUTPUT SECTION
  // ===========================================================================
  GetParam(kParamWidth)->InitDouble("Width", 100., 0., 100., 0.1, "%");

  // ===========================================================================
  // EDITOR SETUP
  // ===========================================================================
#if IPLUG_EDITOR
#if defined(WEBVIEW_EDITOR_DELEGATE)
  SetCustomUrlScheme("iplug2");
  SetEnableDevTools(true);

  mEditorInitFunc = [&]() {
    LoadIndexHtml(__FILE__, GetBundleID());
    EnableScroll(false);
  };
#endif
#endif
}

#if IPLUG_DSP
void PluginInstance::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  mDSP.ProcessBlock(inputs, outputs, NInChansConnected(), NOutChansConnected(), nFrames);
  mMeterSender.ProcessBlock(outputs, nFrames, kCtrlTagMeter);
}

void PluginInstance::OnIdle()
{
  mMeterSender.TransmitData(*this);
}

void PluginInstance::OnReset()
{
  mDSP.Reset(GetSampleRate(), GetBlockSize());
  mMeterSender.Reset(GetSampleRate());
}

void PluginInstance::OnParamChange(int paramIdx)
{
  mDSP.SetParam(paramIdx, GetParam(paramIdx)->Value());
}
#endif
