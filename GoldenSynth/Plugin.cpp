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
  // Osc1 level for balancing with Osc2
  GetParam(kParamOsc1Level)->InitDouble("Osc1 Level", 100., 0., 100., 0.1, "%");  // 100% = full
  // Osc1 octave and detune (for full symmetry with Osc2)
  GetParam(kParamOsc1Octave)->InitEnum("Osc1 Octave", 2, 5, "", IParam::kFlagsNone, "",
    "-2", "-1", "0", "+1", "+2");  // Default 0 (index 2)
  GetParam(kParamOsc1Detune)->InitDouble("Osc1 Detune", 0., -100., 100., 0.1, "cents");  // 0 = no detune

  // Oscillator 2 parameters (fully independent like Serum)
  // Osc2 is mixed with Osc1 to create fatter, layered sounds
  GetParam(kParamOsc2Waveform)->InitEnum("Osc2 Wave", 1, 7, "", IParam::kFlagsNone, "",
    "Sine", "Saw", "Square", "Triangle", "Pulse", "FM", "Wavetable");  // Default Saw (index 1)
  GetParam(kParamOsc2Octave)->InitEnum("Osc2 Octave", 2, 5, "", IParam::kFlagsNone, "",
    "-2", "-1", "0", "+1", "+2");  // Default 0 (index 2)
  GetParam(kParamOsc2Detune)->InitDouble("Osc2 Detune", 7., -100., 100., 0.1, "cents");  // +7 cents = classic fat
  GetParam(kParamOsc2Level)->InitDouble("Osc2 Level", 50., 0., 100., 0.1, "%");  // 50% = equal mix
  // Osc2 wavetable morph (independent from Osc1)
  GetParam(kParamOsc2Morph)->InitDouble("Osc2 Morph", 0., 0., 100., 0.1, "%");
  // Osc2 pulse width (independent from Osc1)
  GetParam(kParamOsc2PulseWidth)->InitDouble("Osc2 PW", 50., 5., 95., 0.1, "%");
  // Osc2 FM parameters (independent from Osc1 for different harmonic content)
  GetParam(kParamOsc2FMRatio)->InitEnum("Osc2 FM Ratio", 2, 9, "", IParam::kFlagsNone, "",
    "0.5:1", "1:1", "2:1", "3:1", "4:1", "5:1", "6:1", "7:1", "8:1");  // Default 2:1
  GetParam(kParamOsc2FMFine)->InitDouble("Osc2 FM Fine", 0., -50., 50., 0.1, "%");
  GetParam(kParamOsc2FMDepth)->InitDouble("Osc2 FM Depth", 50., 0., 100., 0.1, "%");

  // ═══════════════════════════════════════════════════════════════════════════════
  // OSC1 UNISON PARAMETERS
  // ═══════════════════════════════════════════════════════════════════════════════
  // Unison stacks multiple detuned copies of an oscillator to create massive,
  // thick sounds. This is THE feature that defines modern EDM supersaws and leads.
  //
  // HOW IT WORKS:
  //   Single Voice:    440Hz  ────►  Thin
  //   8-Voice Unison:  437Hz ─┐
  //                    438Hz ─┤
  //                    439Hz ─┤
  //                    440Hz ─┼────►  MASSIVE (beating between voices)
  //                    441Hz ─┤
  //                    442Hz ─┤
  //                    443Hz ─┘
  //
  // Classic synths using unison: Roland JP-8000 (Supersaw), Access Virus, Serum
  // PER-OSCILLATOR UNISON: Like Serum, each oscillator has independent unison.
  // This allows: thick supersaw Osc1 (8 voices) + clean sub bass Osc2 (1 voice)
  // ═══════════════════════════════════════════════════════════════════════════════
  GetParam(kParamOsc1UnisonVoices)->InitEnum("Osc1 Uni", 0, 8, "", IParam::kFlagsNone, "",
    "1", "2", "3", "4", "5", "6", "7", "8");  // Default 1 = off (index 0)

  // Detune spread: how far apart the unison voices are pitched
  // 0%: All voices at same pitch (no effect)
  // 25%: Subtle chorus/doubling
  // 50%: Classic supersaw thickness
  // 100%: Extreme, almost out-of-tune spread
  GetParam(kParamOsc1UnisonDetune)->InitDouble("Osc1 Uni Det", 25., 0., 100., 0.1, "%");

  // Stereo width: how much the unison voices are spread across L/R
  // 0%: All voices center (mono)
  // 100%: Full stereo spread (outer voices hard L/R)
  GetParam(kParamOsc1UnisonWidth)->InitDouble("Osc1 Uni Wid", 80., 0., 100., 0.1, "%");

  // Blend: balance between center voice and detuned voices
  // 0%: Only center voice (defeats the purpose, but useful for morphing)
  // 50%: Equal mix of all voices
  // 100%: Only detuned voices, no center (hollow but wide)
  GetParam(kParamOsc1UnisonBlend)->InitDouble("Osc1 Uni Bld", 75., 0., 100., 0.1, "%");

  // ═══════════════════════════════════════════════════════════════════════════════
  // OSC2 UNISON PARAMETERS (Independent from Osc1 - Serum-style)
  // ═══════════════════════════════════════════════════════════════════════════════
  // Osc2 has its own unison engine with independent voice count, detune, width,
  // and blend. This enables powerful sound design combinations:
  //
  // EXAMPLE USE CASES:
  //   - Thick lead (Osc1: 8 voices) + clean sub (Osc2: 1 voice)
  //   - Subtle doubling (both: 2 voices with different detune amounts)
  //   - Massive pad (both: 8 voices with different widths for depth)
  //   - Moving texture (Osc1: wide spread, Osc2: narrow spread for focus)
  // ═══════════════════════════════════════════════════════════════════════════════
  GetParam(kParamOsc2UnisonVoices)->InitEnum("Osc2 Uni", 0, 8, "", IParam::kFlagsNone, "",
    "1", "2", "3", "4", "5", "6", "7", "8");  // Default 1 = off (index 0)

  GetParam(kParamOsc2UnisonDetune)->InitDouble("Osc2 Uni Det", 25., 0., 100., 0.1, "%");
  GetParam(kParamOsc2UnisonWidth)->InitDouble("Osc2 Uni Wid", 80., 0., 100., 0.1, "%");
  GetParam(kParamOsc2UnisonBlend)->InitDouble("Osc2 Uni Bld", 75., 0., 100., 0.1, "%");

  // ═══════════════════════════════════════════════════════════════════════════════
  // OSCILLATOR SYNC
  // ═══════════════════════════════════════════════════════════════════════════════
  // Hard sync forces Osc2 to reset its phase whenever Osc1 completes a cycle.
  // This creates a distinctive, aggressive "sync sweep" sound when Osc2's pitch
  // is different from Osc1. Classic analog synth technique from the 1970s.
  //
  // SYNC SWEEP SOUND:
  //   - Set Osc1 and Osc2 to same waveform (e.g., Saw)
  //   - Enable Hard Sync
  //   - Sweep Osc2 octave up → creates harmonic "tearing" sound
  //   - Used in: Van Halen "Jump", Depeche Mode, countless EDM tracks
  //
  // TECHNICAL: When Osc1's phase crosses zero, Osc2's phase is reset to match.
  // This creates complex waveform shapes with rich harmonic content.
  // ═══════════════════════════════════════════════════════════════════════════════
  GetParam(kParamOscSync)->InitEnum("Osc Sync", 0, 2, "", IParam::kFlagsNone, "",
    "Off", "Hard");  // Default Off (index 0)

  // ═══════════════════════════════════════════════════════════════════════════════
  // LFO (Low Frequency Oscillator) - Modulation for movement and expression
  // ═══════════════════════════════════════════════════════════════════════════════
  // The LFO modulates the filter cutoff to create wobbles, sweeps, and movement.
  // This is THE feature that brings static sounds to life.
  //
  // RATE: How fast the LFO cycles
  //   0.01 Hz = 100 second cycle (glacially slow pads)
  //   1 Hz = typical dubstep wobble
  //   10+ Hz = vibrato/tremolo territory
  //
  // DEPTH: How much the LFO affects the filter cutoff
  //   Modulation is in octaves: ±4 octaves at 100% depth
  //   At 50% depth: ±2 octaves of filter sweep
  //
  // WAVEFORMS: Different shapes create different feels
  //   Sine = smooth, natural
  //   Triangle = linear, more "pointy"
  //   Saw Up/Down = building/falling tension
  //   Square = hard gating effects
  //   S&H = random stepped values
  //
  // RETRIGGER: Controls LFO phase behavior on note-on
  //   Off = free-running (each note different phase, evolving pads)
  //   On = phase resets to 0 (consistent attack, good for bass/leads)
  // ═══════════════════════════════════════════════════════════════════════════════
  GetParam(kParamLFORate)->InitDouble("LFO Rate", 1., 0.01, 20., 0.01, "Hz",
    IParam::kFlagsNone, "", IParam::ShapePowCurve(3.0));  // Exponential for musical feel

  GetParam(kParamLFODepth)->InitDouble("LFO Depth", 0., 0., 100., 0.1, "%");  // 0 = off by default

  GetParam(kParamLFOWaveform)->InitEnum("LFO Wave", 0, 6, "", IParam::kFlagsNone, "",
    "Sine", "Triangle", "Saw Up", "Saw Down", "Square", "S&H");

  GetParam(kParamLFORetrigger)->InitEnum("LFO Retrig", 0, 2, "", IParam::kFlagsNone, "",
    "Free", "Retrig");  // Default free-running

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
