#include "TemplateProject2.h"
#include "IPlug_include_in_plug_src.h"

TemplateProject::TemplateProject(const InstanceInfo& info)
: iplug::Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kParamGain)->InitDouble("Gain", 100., 0., 200.0, 0.01, "%");
    
#if IPLUG_EDITOR
#if defined(WEBVIEW_EDITOR_DELEGATE)
  // WebView editor delegate setup (for AU/VST3)
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
void TemplateProject::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  mDSP.ProcessBlock(inputs, outputs, 2, nFrames);
  mMeterSender.ProcessBlock(outputs, nFrames, kCtrlTagMeter);
}

void TemplateProject::OnIdle()
{
  mMeterSender.TransmitData(*this);
}

void TemplateProject::OnReset()
{
  mDSP.Reset(GetSampleRate(), GetBlockSize());
  mMeterSender.Reset(GetSampleRate());
}

void TemplateProject::ProcessMidiMsg(const IMidiMsg& msg)
{
  // Effect plugin - no MIDI processing needed
}

void TemplateProject::OnParamChange(int paramIdx)
{
  mDSP.SetParam(paramIdx, GetParam(paramIdx)->Value());
}

void TemplateProject::OnParamChangeUI(int paramIdx, EParamSource source)
{
  // UI updates are handled in web view (resources/web/src/components/App.tsx)
}

bool TemplateProject::OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData)
{
  // Message handling is done in web view (resources/web/src/components/App.tsx)
  return false;
}
#endif
