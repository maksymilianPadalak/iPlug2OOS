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
// COLOR MODES
// =============================================================================
// Output color modes for tonal character.
// These apply filtering AFTER the reverb tank (not in the feedback loop).
// =============================================================================
enum EColorMode
{
  kColorBright = 0,   // No filtering - full bandwidth, airy
  kColorNeutral,      // 10kHz lowpass - takes off harshness, still open
  kColorDark,         // 4kHz lowpass - clearly warm, vintage character
  kColorStudio,       // 600Hz highpass + 8kHz lowpass - bandpass for mix clarity
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
  kParamSize,         // Room size - scales all delay times (0-100%)
  kParamDecay,        // Feedback amount - controls tail length (0-100%)
  kParamPreDelay,     // Time before reverb starts (0-100ms)
  kParamDiffusion,    // Input smearing - affects attack character (0-100%)
  kParamDensity,      // Tank diffusion - affects tail texture (0-100%)

  // --- Tone Section ---
  kParamDamping,      // High frequency decay rate (0-100%)
  kParamLowCut,       // Input highpass frequency (20-500Hz)
  kParamHighCut,      // Tank lowpass frequency (1000-20000Hz)
  kParamColor,        // Output color mode (Bright/Neutral/Dark/Studio)

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
