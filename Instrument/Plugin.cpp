#include "Plugin.h"
#include "IPlug_include_in_plug_src.h"

PluginInstance::PluginInstance(const InstanceInfo& info)
: iplug::Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kParamGain)->InitDouble("Gain", 80., 0., 100.0, 0.01, "%");
  GetParam(kParamWaveform)->InitEnum("Waveform", 0, 4, "", IParam::kFlagsNone, "", "Sine", "Saw", "Square", "Triangle");
  GetParam(kParamAttack)->InitDouble("Attack", 10., 1., 1000., 0.1, "ms");
  GetParam(kParamDecay)->InitDouble("Decay", 100., 1., 2000., 0.1, "ms");
  GetParam(kParamSustain)->InitDouble("Sustain", 70., 0., 100., 0.1, "%");
  GetParam(kParamRelease)->InitDouble("Release", 200., 1., 5000., 0.1, "ms");

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
  mDSP.ProcessBlock(nullptr, outputs, 2, nFrames);
  mMeterSender.ProcessBlock(outputs, nFrames, kCtrlTagMeter);
  mWaveformSender.ProcessBlock(outputs, nFrames, kCtrlTagWaveform, 1, 0);
}

void PluginInstance::OnIdle()
{
  mMeterSender.TransmitData(*this);
  mWaveformSender.TransmitData(*this);
}

void PluginInstance::OnReset()
{
  mDSP.Reset(GetSampleRate(), GetBlockSize());
  mMeterSender.Reset(GetSampleRate());
}

void PluginInstance::ProcessMidiMsg(const IMidiMsg& msg)
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

void PluginInstance::OnParamChange(int paramIdx)
{
  mDSP.SetParam(paramIdx, GetParam(paramIdx)->Value());
}

void PluginInstance::OnParamChangeUI(int paramIdx, EParamSource source)
{
}

bool PluginInstance::OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData)
{
  return false;
}
#endif
