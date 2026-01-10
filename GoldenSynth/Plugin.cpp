#include "Plugin.h"
#include "IPlug_include_in_plug_src.h"

PluginInstance::PluginInstance(const InstanceInfo& info)
: iplug::Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kParamGain)->InitDouble("Gain", 80., 0., 100.0, 0.01, "%");
  GetParam(kParamWaveform)->InitEnum("Waveform", 0, 7, "", IParam::kFlagsNone, "",
    "Sine", "Saw", "Square", "Triangle", "Pulse", "FM", "Wavetable");
  GetParam(kParamWavetablePosition)->InitDouble("WT Position", 0., 0., 100., 0.1, "%");
  GetParam(kParamAttack)->InitDouble("Attack", 10., 1., 1000., 0.1, "ms");
  GetParam(kParamDecay)->InitDouble("Decay", 100., 1., 2000., 0.1, "ms");
  GetParam(kParamSustain)->InitDouble("Sustain", 70., 0., 100., 0.1, "%");
  GetParam(kParamRelease)->InitDouble("Release", 200., 1., 5000., 0.1, "ms");

  // Envelope velocity sensitivity: how much MIDI velocity affects envelope times
  // At 0%: velocity has no effect on envelope (classic organ behavior)
  // At 100%: harder hits = snappier attack/decay (expressive, piano-like)
  GetParam(kParamEnvVelocity)->InitDouble("Env Velocity", 50., 0., 100., 1., "%");

  // Filter parameters
  GetParam(kParamFilterCutoff)->InitDouble("Filter Cutoff", 10000., 20., 20000., 1., "Hz",
    IParam::kFlagsNone, "", IParam::ShapePowCurve(3.0));  // Exponential curve for natural feel
  GetParam(kParamFilterResonance)->InitDouble("Filter Reso", 0., 0., 100., 0.1, "%");
  GetParam(kParamFilterType)->InitEnum("Filter Type", 0, 4, "", IParam::kFlagsNone, "",
    "Lowpass", "Highpass", "Bandpass", "Notch");

  // Pulse width modulation (only affects Pulse waveform)
  GetParam(kParamPulseWidth)->InitDouble("Pulse Width", 50., 5., 95., 0.1, "%");

  // FM synthesis parameters (only affects FM waveform)
  // Coarse ratio: harmonic values for musical tones
  GetParam(kParamFMRatio)->InitEnum("FM Ratio", 2, 9, "", IParam::kFlagsNone, "",
    "0.5:1", "1:1", "2:1", "3:1", "4:1", "5:1", "6:1", "7:1", "8:1");  // Default 2:1 (index 2)
  // Fine ratio: percentage offset for inharmonic/bell sounds (DX7-style)
  GetParam(kParamFMFine)->InitDouble("FM Fine", 0., -50., 50., 0.1, "%");  // 0 = no detune
  GetParam(kParamFMDepth)->InitDouble("FM Depth", 50., 0., 100., 0.1, "%");  // Modulation index

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
