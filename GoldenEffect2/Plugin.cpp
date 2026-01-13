#include "Plugin.h"
#include "IPlug_include_in_plug_src.h"

/**
 * Plugin Constructor - Initialize parameters
 */
PluginInstance::PluginInstance(const InstanceInfo& info)
: iplug::Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  // ===========================================================================
  // GAIN PARAMETER
  // ===========================================================================
  GetParam(kParamGain)->InitDouble("Gain", 0., -60., 12., 0.1, "dB");

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
