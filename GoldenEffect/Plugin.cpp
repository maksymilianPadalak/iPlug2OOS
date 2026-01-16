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
  // Mode selects the reverb type - each mode sets internal diffusion and ER patterns
  // Plate = instant attack, bright shimmer | Chamber = fast attack, dense ERs, warm
  // Hall = medium attack, wide stereo | Cathedral = very slow attack, huge ethereal space
  GetParam(kParamMode)->InitEnum("Mode", kModePlate, kNumReverbModes,
    "", IParam::kFlagsNone, "",
    "Plate", "Chamber", "Hall", "Cathedral");
  GetParam(kParamSize)->InitDouble("Size", 70., 0., 100., 0.1, "%");
  GetParam(kParamDecay)->InitDouble("Decay", 70., 0., 99., 0.1, "%");
  GetParam(kParamPreDelay)->InitDouble("Pre-Delay", 10., 0., 200., 0.1, "ms");
  // Density controls the tank allpass feedback - texture of the tail
  // Low = grainy, you hear individual reflections
  // High = smooth, continuous wash
  GetParam(kParamDensity)->InitDouble("Density", 70., 0., 100., 0.1, "%");

  // ===========================================================================
  // TONE SECTION
  // ===========================================================================
  GetParam(kParamLowCut)->InitDouble("Low Cut", 80., 20., 1000., 1., "Hz");
  GetParam(kParamHighCut)->InitDouble("High Cut", 8000., 500., 20000., 10., "Hz");
  // Color controls BOTH output filtering AND feedback damping (FutureVerb-style).
  // This combines two related controls into one intuitive "tonal character" selector.
  // Bright = airy (no filter, low damping) | Dark = vintage (steep filter, high damping)
  GetParam(kParamColor)->InitEnum("Color", kColorNeutral, kNumColorModes,
    "", IParam::kFlagsNone, "",
    "Bright", "Neutral", "Dark", "Studio");

  // ===========================================================================
  // MODULATION SECTION
  // ===========================================================================
  GetParam(kParamModRate)->InitDouble("Mod Rate", 0.5, 0.1, 2., 0.01, "Hz");
  GetParam(kParamModDepth)->InitDouble("Mod Depth", 50., 0., 100., 0.1, "%");

  // ===========================================================================
  // OUTPUT SECTION
  // ===========================================================================
  GetParam(kParamWidth)->InitDouble("Width", 100., 0., 100., 0.1, "%");
  // Freeze captures the current reverb tail and sustains it indefinitely.
  // When active: feedback = 100%, input to tank = 0, damping bypassed.
  GetParam(kParamFreeze)->InitBool("Freeze", false);

  // ===========================================================================
  // EARLY/LATE (Listener Position)
  // ===========================================================================
  // Controls the amount of early reflections - simulates listener distance.
  // 0% = Close to source (full early reflections, punchy attack)
  // 100% = Far from source (no early reflections, slow diffuse attack)
  // Late reverb is always at full volume.
  GetParam(kParamEarlyLate)->InitDouble("Early/Late", 50., 0., 100., 0.1, "%");

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
