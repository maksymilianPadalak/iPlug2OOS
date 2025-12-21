#include "Plugin.h"
#include "IPlug_include_in_plug_src.h"

PluginInstance::PluginInstance(const InstanceInfo& info)
: iplug::Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  // Master
  GetParam(kParamGain)->InitDouble("Gain", 80., 0., 100.0, 0.01, "%");

  // Kick parameters
  GetParam(kParamKickPitchStart)->InitDouble("Kick Pitch Start", 300., 100., 500., 1., "Hz");
  GetParam(kParamKickPitchEnd)->InitDouble("Kick Pitch End", 50., 30., 150., 1., "Hz");
  GetParam(kParamKickPitchDecay)->InitDouble("Kick Pitch Decay", 50., 10., 200., 1., "ms");
  GetParam(kParamKickAmpDecay)->InitDouble("Kick Amp Decay", 300., 50., 1000., 1., "ms");

  // Snare parameters
  GetParam(kParamSnareFilterFreq)->InitDouble("Snare Filter", 2000., 500., 8000., 10., "Hz");
  GetParam(kParamSnareFilterQ)->InitDouble("Snare Q", 1.0, 0.3, 5.0, 0.1, "");
  GetParam(kParamSnareNoiseDecay)->InitDouble("Snare Decay", 150., 50., 500., 1., "ms");
  GetParam(kParamSnareBodyMix)->InitDouble("Snare Body", 30., 0., 100., 1., "%");

  // HiHat parameters
  GetParam(kParamHiHatFilterFreq)->InitDouble("HiHat Tone", 8000., 4000., 16000., 100., "Hz");
  GetParam(kParamHiHatClosedDecay)->InitDouble("HiHat Closed", 30., 5., 100., 1., "ms");
  GetParam(kParamHiHatOpenDecay)->InitDouble("HiHat Open", 400., 100., 1000., 10., "ms");

  // Tom parameters
  GetParam(kParamTomPitchStart)->InitDouble("Tom Pitch Start", 200., 80., 400., 1., "Hz");
  GetParam(kParamTomPitchEnd)->InitDouble("Tom Pitch End", 80., 40., 200., 1., "Hz");
  GetParam(kParamTomPitchDecay)->InitDouble("Tom Pitch Decay", 60., 20., 150., 1., "ms");
  GetParam(kParamTomAmpDecay)->InitDouble("Tom Amp Decay", 400., 100., 800., 1., "ms");

  // Clap parameters
  GetParam(kParamClapFilterFreq)->InitDouble("Clap Filter", 1500., 500., 4000., 10., "Hz");
  GetParam(kParamClapDecay)->InitDouble("Clap Decay", 200., 50., 500., 1., "ms");
  GetParam(kParamClapSpread)->InitDouble("Clap Spread", 20., 5., 50., 1., "ms");

  // Rim parameters
  GetParam(kParamRimPitch)->InitDouble("Rim Pitch", 800., 400., 2000., 10., "Hz");
  GetParam(kParamRimDecay)->InitDouble("Rim Decay", 20., 5., 80., 1., "ms");
  GetParam(kParamRimClick)->InitDouble("Rim Click", 50., 0., 100., 1., "%");

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

void PluginInstance::ProcessMidiMsg(const IMidiMsg& msg)
{
  TRACE;

  const int status = msg.StatusMsg();

  switch (status)
  {
    case IMidiMsg::kNoteOn:
    case IMidiMsg::kNoteOff:
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
