#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "ISender.h"
#ifndef NO_IGRAPHICS
#include "IControls.h"
#endif

const int kNumPresets = 1;

enum EParams
{
  kParamGain = 0,
  kParamWaveform,
  kParamWavetablePosition,  // Morph position within wavetable (0-100%)
  kParamAttack,
  kParamDecay,
  kParamSustain,
  kParamRelease,
  kParamEnvVelocity,        // Velocity → envelope time modulation (0-100%)
  // Filter parameters
  kParamFilterCutoff,       // Filter cutoff frequency (20-20000 Hz)
  kParamFilterResonance,    // Filter resonance (0-100%)
  kParamFilterType,         // Filter type (LP, HP, BP, Notch)
  // Pulse width modulation
  kParamPulseWidth,         // Pulse width (5-95%), 50% = square wave
  // FM synthesis parameters (Osc1)
  kParamFMRatio,            // Coarse ratio: 0.5, 1, 2, 3, 4, 5, 6, 7, 8 (harmonic)
  kParamFMFine,             // Fine ratio offset: -50% to +50% (allows inharmonic)
  kParamFMDepth,            // Modulation depth/index (0-100%)
  kParamOsc1Level,          // Osc1 mix level (0-100%) for balancing with Osc2
  kParamOsc1Octave,         // Osc1 octave offset: -2, -1, 0, +1, +2
  kParamOsc1Detune,         // Osc1 fine detune in cents (-100 to +100)
  kParamOsc1Pan,            // Osc1 stereo pan (-100% left to +100% right)
  // Oscillator 2 parameters (fully independent like Serum)
  kParamOsc2Waveform,       // Osc2 waveform (same options as Osc1)
  kParamOsc2Octave,         // Osc2 octave offset: -2, -1, 0, +1, +2
  kParamOsc2Detune,         // Osc2 fine detune in cents (-100 to +100)
  kParamOsc2Level,          // Osc2 mix level (0-100%)
  kParamOsc2Morph,          // Osc2 wavetable morph position (0-100%)
  kParamOsc2PulseWidth,     // Osc2 pulse width (5-95%)
  kParamOsc2FMRatio,        // Osc2 FM coarse ratio (0.5-8)
  kParamOsc2FMFine,         // Osc2 FM fine ratio offset (-50% to +50%)
  kParamOsc2FMDepth,        // Osc2 FM modulation depth (0-100%)
  kParamOsc2Pan,            // Osc2 stereo pan (-100% left to +100% right)
  // ═══════════════════════════════════════════════════════════════════════════
  // OSC1 UNISON - Stack multiple detuned voices for massive sound
  // ═══════════════════════════════════════════════════════════════════════════
  kParamOsc1UnisonVoices,   // Osc1 unison voices (1-8), 1 = off
  kParamOsc1UnisonDetune,   // Osc1 detune spread (0-100%)
  kParamOsc1UnisonWidth,    // Osc1 stereo spread (0-100%)
  kParamOsc1UnisonBlend,    // Osc1 center vs detuned mix (0-100%)
  // ═══════════════════════════════════════════════════════════════════════════
  // OSC2 UNISON - Independent unison for second oscillator (Serum-style)
  // ═══════════════════════════════════════════════════════════════════════════
  kParamOsc2UnisonVoices,   // Osc2 unison voices (1-8), 1 = off
  kParamOsc2UnisonDetune,   // Osc2 detune spread (0-100%)
  kParamOsc2UnisonWidth,    // Osc2 stereo spread (0-100%)
  kParamOsc2UnisonBlend,    // Osc2 center vs detuned mix (0-100%)
  // ═══════════════════════════════════════════════════════════════════════════
  // OSCILLATOR SYNC - Classic analog sync sound
  // ═══════════════════════════════════════════════════════════════════════════
  kParamOscSync,            // Sync mode: Off, Hard (Osc2 syncs to Osc1)
  // ═══════════════════════════════════════════════════════════════════════════
  // LFO1 - Low Frequency Oscillator 1 for modulation
  // Low/High system: LFO sweeps from Low to High, enabling unipolar, inverted,
  // and asymmetric modulation. Replaces simple Depth for full control.
  // Tempo Sync: When not "Off", LFO syncs to host tempo using musical divisions.
  // ═══════════════════════════════════════════════════════════════════════════
  kParamLFO1Rate,           // LFO1 rate (0.01-20 Hz) - used when sync is off
  kParamLFO1Sync,           // LFO1 tempo sync (Off, 4/1, 2/1, 1/1, 1/2, 1/4, 1/8, 1/16, 1/32 + dotted/triplet)
  kParamLFO1Low,            // LFO1 low point (-200% to +100%), where LFO goes at minimum
  kParamLFO1High,           // LFO1 high point (-100% to +200%), where LFO goes at maximum
  kParamLFO1Waveform,       // LFO1 waveform (Sine, Tri, SawUp, SawDown, Square, S&H)
  kParamLFO1Retrigger,      // LFO1 retrigger mode (Free, Retrig)
  kParamLFO1Destination,    // LFO1 destination (Off, Filter, Pitch, PW, Amp, FM, WT, per-osc)
  // ═══════════════════════════════════════════════════════════════════════════
  // LFO2 - Low Frequency Oscillator 2 for modulation
  // ═══════════════════════════════════════════════════════════════════════════
  kParamLFO2Rate,           // LFO2 rate (0.01-20 Hz) - used when sync is off
  kParamLFO2Sync,           // LFO2 tempo sync (Off, 4/1, 2/1, 1/1, 1/2, 1/4, 1/8, 1/16, 1/32 + dotted/triplet)
  kParamLFO2Low,            // LFO2 low point (-200% to +100%), where LFO goes at minimum
  kParamLFO2High,           // LFO2 high point (-100% to +200%), where LFO goes at maximum
  kParamLFO2Waveform,       // LFO2 waveform (Sine, Tri, SawUp, SawDown, Square, S&H)
  kParamLFO2Retrigger,      // LFO2 retrigger mode (Free, Retrig)
  kParamLFO2Destination,    // LFO2 destination (Off, Filter, Pitch, PW, Amp, FM, WT, per-osc)
  kNumParams
};

#if IPLUG_DSP
#include "Plugin_DSP.h"
#endif

enum EControlTags
{
  kCtrlTagMeter = 0,
  kCtrlTagWaveform,
  kNumCtrlTags
};

using namespace iplug;
#ifndef NO_IGRAPHICS
using namespace igraphics;
#endif

class PluginInstance final : public iplug::Plugin
{
public:
  PluginInstance(const InstanceInfo& info);

#if IPLUG_DSP
public:
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void ProcessMidiMsg(const IMidiMsg& msg) override;
  void OnReset() override;
  void OnParamChange(int paramIdx) override;
  void OnParamChangeUI(int paramIdx, EParamSource source) override;
  void OnIdle() override;
  bool OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData) override;

private:
  PluginInstanceDSP<sample> mDSP;
  IPeakAvgSender<2> mMeterSender;
  IBufferSender<1> mWaveformSender;
#endif
};
