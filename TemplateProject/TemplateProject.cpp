#include "TemplateProject.h"
#include "IPlug_include_in_plug_src.h"

TemplateProject::TemplateProject(const InstanceInfo& info)
: iplug::Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kParamGain)->InitDouble("Gain", 100., 0., 100.0, 0.01, "%");
  GetParam(kParamAttack)->InitDouble("Attack", 10., 1., 1000., 0.1, "ms", IParam::kFlagsNone, "ADSR", IParam::ShapePowCurve(3.));
  GetParam(kParamDecay)->InitDouble("Decay", 10., 1., 1000., 0.1, "ms", IParam::kFlagsNone, "ADSR", IParam::ShapePowCurve(3.));
  GetParam(kParamSustain)->InitDouble("Sustain", 50., 0., 100., 1, "%", IParam::kFlagsNone, "ADSR");
  GetParam(kParamRelease)->InitDouble("Release", 10., 2., 1000., 0.1, "ms", IParam::kFlagsNone, "ADSR");
    
  // Oscillator parameters
  GetParam(kParamOsc1Mix)->InitPercentage("Osc1 Mix", 100.);
  GetParam(kParamOsc2Mix)->InitPercentage("Osc2 Mix", 0.);
  GetParam(kParamOsc1Detune)->InitDouble("Osc1 Detune", 0., -50., 50., 0.1, "cents");
  GetParam(kParamOsc2Detune)->InitDouble("Osc2 Detune", 0., -50., 50., 0.1, "cents");
  GetParam(kParamOsc1Octave)->InitInt("Osc1 Octave", 0, -2, 2, "");
  GetParam(kParamOsc2Octave)->InitInt("Osc2 Octave", 0, -2, 2, "");
  GetParam(kParamOsc1Wave)->InitEnum("Osc1 Wave", 0, {"Sine", "Saw", "Square", "Triangle"});
  GetParam(kParamOsc2Wave)->InitEnum("Osc2 Wave", 0, {"Sine", "Saw", "Square", "Triangle"});
  
  // Reverb parameters
  GetParam(kParamReverbRoomSize)->InitDouble("Reverb Room Size", 0.5, 0.3, 0.99, 0.01, "");
  GetParam(kParamReverbDamp)->InitPercentage("Reverb Damp", 50.);
  GetParam(kParamReverbWidth)->InitDouble("Reverb Width", 0.5, 0., 1., 0.01, "");
  GetParam(kParamReverbDry)->InitPercentage("Reverb Dry", 100.);
  GetParam(kParamReverbWet)->InitPercentage("Reverb Wet", 0.);
    
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
  mDSP.ProcessBlock(nullptr, outputs, 2, nFrames, mTimeInfo.mPPQPos, mTimeInfo.mTransportIsRunning);
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
  TRACE;
  
  int status = msg.StatusMsg();
  
  switch (status)
  {
    case IMidiMsg::kNoteOn:
    case IMidiMsg::kNoteOff:
    case IMidiMsg::kPolyAftertouch:
    case IMidiMsg::kControlChange:
    case IMidiMsg::kProgramChange:
    case IMidiMsg::kChannelAftertouch:
    case IMidiMsg::kPitchWheel:
    {
      goto handle;
    }
    default:
      return;
  }
  
handle:
  mDSP.ProcessMidiMsg(msg);
  SendMidiMsg(msg);
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
