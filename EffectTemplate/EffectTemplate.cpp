#include "EffectTemplate.h"
#include "IPlug_include_in_plug_src.h"

EffectTemplate::EffectTemplate(const InstanceInfo& info)
: iplug::Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kParamGain)->InitDouble("Gain", 100., 0., 200.0, 0.01, "%");
  GetParam(kParamMix)->InitDouble("Mix", 50., 0., 100., 0.1, "%");
  GetParam(kParamDelayTime)->InitDouble("Delay Time", 250., 0., 1000., 1., "ms");
  GetParam(kParamDelayFeedback)->InitDouble("Feedback", 30., 0., 100., 0.1, "%");
  GetParam(kParamReverbSize)->InitDouble("Room Size", 50., 0., 100., 0.1, "%");
  GetParam(kParamReverbDamping)->InitDouble("Damping", 50., 0., 100., 0.1, "%");
  GetParam(kParamReverbWidth)->InitDouble("Width", 100., 0., 100., 0.1, "%");
  GetParam(kParamBypass)->InitBool("Bypass", false);

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
void EffectTemplate::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  mDSP.ProcessBlock(inputs, outputs, NInChansConnected(), NOutChansConnected(), nFrames);
  mMeterSender.ProcessBlock(outputs, nFrames, kCtrlTagMeter);
}

void EffectTemplate::OnIdle()
{
  mMeterSender.TransmitData(*this);
}

void EffectTemplate::OnReset()
{
  mDSP.Reset(GetSampleRate(), GetBlockSize());
  mMeterSender.Reset(GetSampleRate());
}

void EffectTemplate::ProcessMidiMsg(const IMidiMsg& msg)
{
  // Effect plugin - no MIDI processing needed
}

void EffectTemplate::OnParamChange(int paramIdx)
{
  mDSP.SetParam(paramIdx, GetParam(paramIdx)->Value());
}

void EffectTemplate::OnParamChangeUI(int paramIdx, EParamSource source)
{
}

bool EffectTemplate::OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData)
{
  return false;
}
#endif
