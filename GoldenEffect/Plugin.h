#pragma once

/**
 * Golden Effect - Production Plate Reverb
 *
 * A Dattorro-style plate reverb demonstrating professional DSP techniques:
 * - Allpass diffusion network for density
 * - Cross-coupled feedback tanks for stereo width
 * - Modulated delay lines for lush character
 * - Multi-tap output for rich reflections
 *
 * This serves as a golden example for LLM-generated effects.
 */

#include "IPlug_include_in_plug_hdr.h"
#include "ISender.h"
#ifndef NO_IGRAPHICS
#include "IControls.h"
#endif

const int kNumPresets = 1;

// =============================================================================
// REVERB MODES
// =============================================================================
// Different reverb space types with distinct characteristics.
// Each mode sets internal diffusion and early reflection patterns.
// =============================================================================
enum EReverbMode
{
  kModePlate = 0,     // Classic plate - instant attack, bright shimmer, no early reflections
  kModeChamber,       // Echo chamber - fast attack, dense early reflections, warm
  kModeHall,          // Concert hall - medium attack, wide stereo, balanced ERs
  kModeCathedral,     // Cathedral - very slow attack, sparse long ERs, huge ethereal space
  kNumReverbModes
};

// =============================================================================
// COLOR MODES
// =============================================================================
// Color modes control BOTH output filtering AND feedback damping.
// This is the FutureVerb-style approach: one semantic control for tonal character.
//
// WHY COMBINE OUTPUT FILTER + DAMPING:
// - Output filter: Affects the final EQ of the reverb (instant effect)
// - Damping: Affects how fast highs decay in the feedback loop (evolving effect)
// - Together they create cohesive tonal character that users understand intuitively
//
// "Bright" = bright output + low damping (highs sustain, airy tail)
// "Dark" = dark output + high damping (highs decay fast, vintage character)
// =============================================================================
enum EColorMode
{
  kColorBright = 0,   // No output filter + low damping (0.15) - full bandwidth, airy, highs sustain
  kColorNeutral,      // 8kHz LPF (12dB/oct) + medium damping (0.35) - natural, balanced decay
  kColorDark,         // 3kHz LPF (24dB/oct) + high damping (0.65) - vintage, highs decay fast
  kColorStudio,       // 600Hz HPF + 6kHz LPF + medium damping (0.45) - mix-ready, controlled
  kNumColorModes
};

// =============================================================================
// PARAMETERS
// =============================================================================
// Parameters are organized by section for clarity.
// Each parameter has a clear purpose and sensible range.
// =============================================================================
enum EParams
{
  // --- Mix Section ---
  kParamDry = 0,      // Dry signal level (0-100%)
  kParamWet,          // Wet signal level (0-100%)

  // --- Character Section ---
  kParamMode,         // Reverb type - Plate/Chamber (controls diffusion internally)
  kParamSize,         // Room size - scales all delay times (0-100%)
  kParamDecay,        // Feedback amount - controls tail length (0-100%)
  kParamPreDelay,     // Time before reverb starts (0-100ms)
  kParamDensity,      // Tank diffusion - affects tail texture (0-100%)

  // --- Tone Section ---
  kParamLowCut,       // Input highpass frequency (20-1000Hz)
  kParamHighCut,      // Input lowpass frequency (500-20000Hz)
  kParamColor,        // Tonal character: output filter + feedback damping (Bright/Neutral/Dark/Studio)

  // --- Modulation Section ---
  kParamModRate,      // LFO speed for chorus effect (0.1-2Hz)
  kParamModDepth,     // LFO intensity (0-100%)

  // --- Output Section ---
  kParamWidth,        // Stereo spread (0-100%)

  // --- Early/Late Section ---
  kParamEarlyLate,    // Balance between early reflections and late reverb (0-100%)

  kNumParams
};

#if IPLUG_DSP
#include "Plugin_DSP.h"
#endif

// =============================================================================
// CONTROL TAGS
// =============================================================================
// Tags for UI controls that receive data from DSP (meters, waveforms, etc.)
// =============================================================================
enum EControlTags
{
  kCtrlTagMeter = 0,  // Output level meter
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
  void OnReset() override;
  void OnParamChange(int paramIdx) override;
  void OnIdle() override;

private:
  PluginInstanceDSP<sample> mDSP;
  IPeakAvgSender<2> mMeterSender;
#endif
};
