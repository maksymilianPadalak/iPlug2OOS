#include "TemplateProject.h"
#include "IPlug_include_in_plug_src.h"

TemplateProject::TemplateProject(const InstanceInfo& info)
: iplug::Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kParamGain)->InitDouble("Gain", 80., 0., 100.0, 0.01, "%");
  GetParam(kParamAttack)->InitDouble("Attack", 20., 1., 2000., 0.1, "ms", IParam::kFlagsNone, "ADSR", IParam::ShapePowCurve(3.));
  GetParam(kParamDecay)->InitDouble("Decay", 400., 1., 4000., 0.1, "ms", IParam::kFlagsNone, "ADSR", IParam::ShapePowCurve(3.));
  GetParam(kParamSustain)->InitDouble("Sustain", 70., 0., 100., 1., "%", IParam::kFlagsNone, "ADSR");
  GetParam(kParamRelease)->InitDouble("Release", 600., 2., 8000., 0.1, "ms", IParam::kFlagsNone, "ADSR", IParam::ShapePowCurve(3.));

  // Oscillators: default to a lush supersaw stack
  GetParam(kParamOsc1Mix)->InitPercentage("Osc1 Mix", 60.);
  GetParam(kParamOsc2Mix)->InitPercentage("Osc2 Mix", 25.);
  GetParam(kParamOsc3Mix)->InitPercentage("Osc3 Mix", 10.);
  GetParam(kParamOsc4Mix)->InitPercentage("Osc4 Mix", 5.);

  GetParam(kParamOsc1Detune)->InitDouble("Osc1 Detune", 0., -50., 50., 0.1, "cents");
  GetParam(kParamOsc2Detune)->InitDouble("Osc2 Detune", 7., -50., 50., 0.1, "cents");
  GetParam(kParamOsc3Detune)->InitDouble("Osc3 Detune", -7., -50., 50., 0.1, "cents");
  GetParam(kParamOsc4Detune)->InitDouble("Osc4 Detune", 19., -50., 50., 0.1, "cents");

  GetParam(kParamOsc1Octave)->InitInt("Osc1 Octave", 0, -2, 2, "");
  GetParam(kParamOsc2Octave)->InitInt("Osc2 Octave", 0, -2, 2, "");
  GetParam(kParamOsc3Octave)->InitInt("Osc3 Octave", -1, -2, 2, "");
  GetParam(kParamOsc4Octave)->InitInt("Osc4 Octave", 1, -2, 2, "");

  GetParam(kParamOsc1Wave)->InitEnum("Osc1 Wave", 1, {"Sine", "Saw", "Square", "Triangle"});
  GetParam(kParamOsc2Wave)->InitEnum("Osc2 Wave", 1, {"Sine", "Saw", "Square", "Triangle"});
  GetParam(kParamOsc3Wave)->InitEnum("Osc3 Wave", 1, {"Sine", "Saw", "Square", "Triangle"});
  GetParam(kParamOsc4Wave)->InitEnum("Osc4 Wave", 3, {"Sine", "Saw", "Square", "Triangle"});

  // Reverb: wide and lush by default
  GetParam(kParamReverbRoomSize)->InitDouble("Reverb Room Size", 0.85, 0.3, 0.99, 0.01, "");
  GetParam(kParamReverbDamp)->InitPercentage("Reverb Damp", 35.);
  GetParam(kParamReverbWidth)->InitDouble("Reverb Width", 0.9, 0., 1., 0.01, "");
  GetParam(kParamReverbDry)->InitPercentage("Reverb Dry", 70.);
  GetParam(kParamReverbWet)->InitPercentage("Reverb Wet", 30.);

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

  const int status = msg.StatusMsg();

  switch (status)
  {
    case IMidiMsg::kNoteOn:
    case IMidiMsg::kNoteOff:
    case IMidiMsg::kPolyAftertouch:
    case IMidiMsg::kControlChange:
    case IMidiMsg::kProgramChange:
    case IMidiMsg::kChannelAftertouch:
    case IMidiMsg::kPitchWheel:
      mDSP.ProcessMidiMsg(msg);
      SendMidiMsg(msg);
      break;
    default:
      break;
  }
}

void TemplateProject::OnParamChange(int paramIdx)
{
  mDSP.SetParam(paramIdx, GetParam(paramIdx)->Value());
}

void TemplateProject::OnParamChangeUI(int paramIdx, EParamSource source)
{
}

bool TemplateProject::OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData)
{
  return false;
}
#endif
