#include "Plugin.h"
#include "IPlug_include_in_plug_src.h"

PluginInstance::PluginInstance(const InstanceInfo& info)
: iplug::Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
    GetParam(kParamGain)->InitDouble("Gain", 100., 0., 200.0, 0.01, "%");
  GetParam(kParamDelayTime)->InitDouble("Delay Time", 250., 1., 2000., 1., "ms");
    GetParam(kParamDelayFeedback)->InitDouble("Feedback", 30., 0., 95., 0.1, "%");
  GetParam(kParamDelayDry)->InitDouble("Dry", 100., 0., 100., 0.1, "%");
  GetParam(kParamDelayWet)->InitDouble("Wet", 25., 0., 100., 0.1, "%");
    GetParam(kParamDelaySync)->InitBool("Sync", false);
  GetParam(kParamDelayDivision)->InitEnum("Division", 2, {"1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/4T", "1/8T"});
  GetParam(kParamReverbOn)->InitBool("Reverb On", false);
  GetParam(kParamReverbMix)->InitDouble("Reverb Mix", 25., 0., 100., 0.1, "%");
  GetParam(kParamReverbSize)->InitDouble("Room Size", 50., 0., 100., 0.1, "%");
  GetParam(kParamReverbDecay)->InitDouble("Decay", 50., 0., 100., 0.1, "%");
  GetParam(kParamReverbDamping)->InitDouble("Damping", 50., 0., 100., 0.1, "%");
  GetParam(kParamReverbPreDelay)->InitDouble("Pre-Delay", 20., 0., 100., 0.1, "ms");
  GetParam(kParamReverbWidth)->InitDouble("Width", 100., 0., 100., 0.1, "%");
  GetParam(kParamReverbLowCut)->InitDouble("Low Cut", 80., 20., 500., 1., "Hz");

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
  mDSP.SetTempo(mTimeInfo.mTempo, mTimeInfo.mTransportIsRunning);
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

void PluginInstance::OnParamChangeUI(int paramIdx, EParamSource source)
{
#if IPLUG_EDITOR
  if (paramIdx == kParamDelaySync)
  {
    const bool sync = GetParam(kParamDelaySync)->Bool();
    if (auto pGraphics = GetUI())
    {
      pGraphics->HideControl(kCtrlTagDelayTime, sync);
    }
  }
#endif
}

bool PluginInstance::OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData)
{
  return false;
}
#endif
