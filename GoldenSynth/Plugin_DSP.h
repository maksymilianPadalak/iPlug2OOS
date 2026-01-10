#pragma once

#include <cmath>
#include <array>
#include <algorithm>

// iPlug2 Synth Infrastructure
#include "MidiSynth.h"

// Q DSP Library
#include <q/support/literals.hpp>
#include <q/support/phase.hpp>
#include <q/synth/sin_osc.hpp>
#include <q/synth/saw_osc.hpp>
#include <q/synth/square_osc.hpp>
#include <q/synth/triangle_osc.hpp>
#include <q/synth/pulse_osc.hpp>
#include <q/synth/envelope_gen.hpp>

using namespace iplug;
namespace q = cycfi::q;
using namespace q::literals;

// ═══════════════════════════════════════════════════════════════════════════════
// Q LIBRARY OSCILLATORS - PolyBLEP/PolyBLAMP Anti-Aliasing
// ═══════════════════════════════════════════════════════════════════════════════
//
// WHAT ARE q::sin, q::saw, q::square, q::triangle?
// These are Q library's real-time oscillators that use PolyBLEP (Polynomial
// Band-Limited Step) anti-aliasing. They generate waveforms sample-by-sample
// with minimal aliasing at low CPU cost.
//
// HOW POLYBLEP WORKS:
// 1. Generate a "naive" waveform (e.g., saw = 2*phase - 1)
// 2. Detect discontinuities (jumps) in the waveform
// 3. Apply a polynomial correction near transitions to "round the corners"
//
// The correction polynomial (2nd order) is applied for ~2 samples around each
// discontinuity: `t + t - t*t - 1` smooths the step function.
//
// POLYBLAMP (for triangle):
// Triangle waves don't have amplitude discontinuities, but have SLOPE
// discontinuities (sharp corners). PolyBLAMP (Band-Limited rAMP) is the
// integrated version of PolyBLEP, using a cubic polynomial to smooth corners.
//
// POLYBLEP vs WAVETABLE (COMPARISON):
// ┌─────────────────────┬──────────────────────┬──────────────────────┐
// │ Aspect              │ PolyBLEP (Q library) │ Wavetable (ours)     │
// ├─────────────────────┼──────────────────────┼──────────────────────┤
// │ CPU per sample      │ Very low (O(1))      │ Low (table lookup)   │
// │ Memory              │ None                 │ ~1MB for mipmaps     │
// │ Anti-aliasing       │ Good (some residual) │ Perfect (band-limit) │
// │ High frequencies    │ Some aliasing        │ No aliasing          │
// │ Morphing support    │ No                   │ Yes (wavetable)      │
// │ Best for            │ Classic analog sound │ Complex/morphing     │
// └─────────────────────┴──────────────────────┴──────────────────────┘
//
// PHASE ITERATOR (q::phase_iterator):
// Q library represents phase as a 32-bit unsigned integer (fixed-point 1.31).
// - Full cycle = 2^32 (natural wraparound via integer overflow)
// - Step = (2^32 × frequency) / sampleRate
// - `mPhase++` advances phase by one step and returns oscillator output
// - `mPhase.set(freq, sps)` updates the step size for a new frequency
//
// REFERENCES:
// - Välimäki & Pekonen, "Perceptually informed synthesis of bandlimited
//   classical waveforms using integrated polynomial interpolation" (2012)
// - Q library source: q/synth/saw_osc.hpp, q/utility/antialiasing.hpp
//
// ═══════════════════════════════════════════════════════════════════════════════

// ═══════════════════════════════════════════════════════════════════════════════
// PULSE WIDTH MODULATION (PWM)
// ═══════════════════════════════════════════════════════════════════════════════
//
// WHAT IS A PULSE WAVE?
// A pulse wave alternates between +1 and -1 with variable "duty cycle" (pulse width).
// Duty cycle = percentage of time the wave is "high" per cycle.
//
//   50% (square):  ████████________  ← Only odd harmonics (hollow, clarinet)
//   25% (narrow):  ████____________  ← All harmonics (brighter, richer)
//   75% (wide):    ████████████____  ← All harmonics (same as 25%, inverted)
//
// HARMONIC CONTENT BY DUTY CYCLE:
// ┌────────────┬─────────────────────────────────────┬────────────────────────┐
// │ Duty Cycle │ Harmonics Present                   │ Character              │
// ├────────────┼─────────────────────────────────────┼────────────────────────┤
// │ 50%        │ Odd only (1, 3, 5, 7...)            │ Hollow, clarinet-like  │
// │ 25% / 75%  │ All except every 4th (4, 8, 12...)  │ Brighter, richer       │
// │ 12.5%      │ All except every 8th                │ Even brighter          │
// │ 10% / 90%  │ Approaches impulse train            │ Very bright, reedy     │
// │ 5% / 95%   │ Near impulse                        │ Thin, buzzy            │
// └────────────┴─────────────────────────────────────┴────────────────────────┘
//
// WHY 5-95% LIMITS?
// At 0% or 100%, the wave is DC (silence). Hardware synths typically limit to
// 5-95% to avoid silence and the "ripping" sound as the waveform collapses.
// This is an industry-standard range used by Roland, Moog, Sequential, etc.
//
// PWM EFFECT (Pulse Width Modulation):
// Slowly modulating the pulse width with an LFO creates a shimmering, chorus-like
// effect as the harmonic content constantly shifts. This is the classic analog
// synth pad sound heard in countless recordings.
//
// Classic PWM synths: Roland Juno-60/106, Sequential Prophet-5, Oberheim OB-X
//
// IMPLEMENTATION NOTES:
// - Uses Q library's pulse_osc with PolyBLEP anti-aliasing on both edges
// - Pulse width parameter is smoothed (~5ms) to prevent clicks during modulation
// - Without smoothing, sudden width changes cause the falling edge to jump
//   mid-cycle, creating an audible discontinuity
//
// KNOWN LIMITATION:
// At very high frequencies (>3kHz fundamental) combined with narrow pulse widths
// (<10%), both rising and falling edges can occur within a single sample. The
// PolyBLEP corrections may overlap, causing subtle artifacts. This edge case is
// rarely musically relevant - most PWM use is in the bass-to-mid frequency range.
//
// ═══════════════════════════════════════════════════════════════════════════════

// ═══════════════════════════════════════════════════════════════════════════════
// FM SYNTHESIS (Frequency Modulation) - DX7-Style Implementation
// ═══════════════════════════════════════════════════════════════════════════════
//
// WHAT IS FM SYNTHESIS?
// Instead of filtering harmonics (subtractive) or adding them (additive), FM
// creates harmonics by modulating one oscillator's frequency with another:
//
//   Modulator ──┐
//               ├──► Carrier ──► Output
//   (hidden)    │    (audible)
//               └── modulates frequency
//
// FORMULA (actually Phase Modulation, but called FM):
//   output = sin(carrierPhase + depth * sin(modulatorPhase))
//
// The modulator's output is added to the carrier's PHASE, not frequency directly.
// This is mathematically equivalent to FM but easier to implement. The DX7 uses
// the same technique (PM labeled as FM).
//
// KEY PARAMETERS (DX7-STYLE COARSE + FINE):
// ┌───────────────┬────────────────────────────────────────────────────────────┐
// │ Parameter     │ Effect                                                     │
// ├───────────────┼────────────────────────────────────────────────────────────┤
// │ Ratio (Coarse)│ Modulator freq = Carrier freq × Ratio                      │
// │               │ Preset harmonic values: 0.5, 1, 2, 3, 4, 5, 6, 7, 8        │
// │               │ Integer ratios produce harmonic partials (musical tones)   │
// ├───────────────┼────────────────────────────────────────────────────────────┤
// │ Fine          │ Percentage offset from coarse ratio (-50% to +50%)         │
// │               │ Final ratio = Coarse × (1 + Fine)                          │
// │               │ At 0%: Pure harmonic sound                                 │
// │               │ Non-zero: Inharmonic partials for bells, metallic sounds   │
// │               │ Example: Coarse 2:1 + Fine +20% = effective ratio 2.4      │
// ├───────────────┼────────────────────────────────────────────────────────────┤
// │ Depth (Index) │ How much the modulator affects the carrier                 │
// │               │ Low (0-20%): Subtle warmth, adds a few harmonics           │
// │               │ Medium (20-50%): Rich, evolving timbres                    │
// │               │ High (50-100%): Bright, metallic, aggressive               │
// │               │ Internally scaled to 0-4π radians (modulation index ~12)   │
// │               │ VELOCITY SENSITIVE: Harder hits = brighter sound           │
// │               │ Formula: effectiveDepth = depth × (0.3 + 0.7 × velocity)   │
// └───────────────┴────────────────────────────────────────────────────────────┘
//
// SOUND DESIGN GUIDE:
// ┌─────────────────────────────────────────────────────────────────────────────┐
// │ Sound Type        │ Coarse │ Fine  │ Depth │ Notes                         │
// ├───────────────────┼────────┼───────┼───────┼───────────────────────────────┤
// │ Electric Piano    │ 2:1    │ 0%    │ 30%   │ Classic Rhodes-like tone      │
// │ Bright Bell       │ 2:1    │ 0%    │ 70%   │ Church bell, chime            │
// │ Tubular Bell      │ 2:1    │ +41%  │ 60%   │ Fine ≈ √2 ratio (inharmonic)  │
// │ Gong / Metallic   │ 3:1    │ +17%  │ 80%   │ Complex inharmonic spectrum   │
// │ Bass Enhancement  │ 0.5:1  │ 0%    │ 40%   │ Sub-harmonic warmth           │
// │ Hollow/Clarinet   │ 1:1    │ 0%    │ 25%   │ Odd harmonics emphasis        │
// │ Aggressive Lead   │ 3:1    │ 0%    │ 90%   │ Bright, cutting tone          │
// └─────────────────────────────────────────────────────────────────────────────┘
//
// FM vs SUBTRACTIVE:
// ┌─────────────────┬──────────────────────┬──────────────────────┐
// │ Aspect          │ FM                   │ Subtractive          │
// ├─────────────────┼──────────────────────┼──────────────────────┤
// │ Harmonics       │ Created by modulation│ Filtered from rich   │
// │ Character       │ Bell, glass, metallic│ Warm, analog         │
// │ CPU cost        │ Very low (just sin)  │ Higher (filters)     │
// │ Classic sounds  │ E-piano, bells, bass │ Pads, leads, brass   │
// └─────────────────┴──────────────────────┴──────────────────────┘
//
// CLASSIC FM SYNTHS:
// Yamaha DX7 (1983), Yamaha TX81Z, Native Instruments FM8, Dexed (open source)
//
// IMPLEMENTATION NOTES:
// - Uses two sine oscillators: carrier (audible) and modulator (hidden)
// - Coarse ratio sets harmonic relationship (enum for clean UI)
// - Fine ratio allows inharmonic detuning (percentage offset)
// - Combined ratio = Coarse × (1 + Fine/100)
// - Depth controls modulation index (scaled to 0-4π radians internally)
// - No anti-aliasing needed - pure sine waves have no harmonics to alias
// - All parameters are smoothed to prevent clicks during modulation
//
// ═══════════════════════════════════════════════════════════════════════════════

// ═══════════════════════════════════════════════════════════════════════════════
// ADSR ENVELOPE - Amplitude Shaping Over Note Lifetime
// ═══════════════════════════════════════════════════════════════════════════════
//
// WHAT IS AN ADSR ENVELOPE?
// An envelope shapes how a sound's amplitude (volume) changes over time. ADSR
// stands for the four stages of the envelope:
//
//   Amplitude
//       │
//     1 │    ╱╲
//       │   ╱  ╲___________
//       │  ╱               ╲
//     0 │ ╱                 ╲________
//       └──────────────────────────────► Time
//         │A│ D │    S     │   R   │
//         ↑   ↑      ↑          ↑
//       Attack Decay Sustain  Release
//        (note on)           (note off)
//
// STAGE DESCRIPTIONS:
// ┌─────────┬────────────────────────────────────────────────────────────────────┐
// │ Stage   │ Behavior                                                           │
// ├─────────┼────────────────────────────────────────────────────────────────────┤
// │ Attack  │ Time to rise from 0 to full amplitude (1.0)                        │
// │         │ Fast attack (1-10ms): Percussive, snappy                           │
// │         │ Slow attack (100-1000ms): Swells, pads, strings                    │
// ├─────────┼────────────────────────────────────────────────────────────────────┤
// │ Decay   │ Time to fall from full amplitude to sustain level                  │
// │         │ Fast decay: Plucky, staccato                                       │
// │         │ Slow decay: Gradual transition to sustain                          │
// ├─────────┼────────────────────────────────────────────────────────────────────┤
// │ Sustain │ Level held while key is pressed (0-100%)                           │
// │         │ 0%: Sound decays to silence (organ-like if attack/decay are fast)  │
// │         │ 100%: No decay, stays at full volume                               │
// │         │ 70%: Typical for many sounds                                       │
// ├─────────┼────────────────────────────────────────────────────────────────────┤
// │ Release │ Time to fall from sustain to 0 after key release                   │
// │         │ Fast release (10-50ms): Tight, punchy                              │
// │         │ Slow release (500-5000ms): Reverberant, pad-like                   │
// └─────────┴────────────────────────────────────────────────────────────────────┘
//
// COMMON PRESET PATTERNS:
// ┌──────────────────┬────────┬───────┬─────────┬─────────┬──────────────────────┐
// │ Sound Type       │ Attack │ Decay │ Sustain │ Release │ Notes                │
// ├──────────────────┼────────┼───────┼─────────┼─────────┼──────────────────────┤
// │ Piano/Keys       │ 1ms    │ 500ms │ 0%      │ 200ms   │ Percussive decay     │
// │ Organ            │ 5ms    │ 0ms   │ 100%    │ 50ms    │ Instant on/off       │
// │ Strings/Pad      │ 300ms  │ 100ms │ 80%     │ 500ms   │ Slow swell           │
// │ Pluck/Guitar     │ 1ms    │ 200ms │ 30%     │ 100ms   │ Fast attack, decay   │
// │ Brass            │ 50ms   │ 100ms │ 70%     │ 100ms   │ Moderate attack      │
// │ Percussion       │ 1ms    │ 100ms │ 0%      │ 50ms    │ All in decay         │
// └──────────────────┴────────┴───────┴─────────┴─────────┴──────────────────────┘
//
// Q LIBRARY IMPLEMENTATION:
// Uses exponential curves for natural-sounding envelopes:
// - Attack: Exponential rise (fast start, slows near peak)
// - Decay: Exponential fall (fast start, slows near sustain)
// - Sustain: Linear (holds level, very slow decay over 50 seconds)
// - Release: Exponential fall (natural decay to silence)
//
// Exponential curves match human perception better than linear ramps.
// Our ears perceive loudness logarithmically, so exponential amplitude
// changes sound more "natural" and "smooth" than linear ones.
//
// IMPORTANT BEHAVIOR NOTES:
// 1. PARAMETER CHANGES: Modifying A/D/S/R only affects FUTURE notes.
//    Currently playing notes continue with their original envelope.
//    This is standard behavior matching hardware synths.
//
// 2. RETRIGGER: When a note is retriggered (legato), we crossfade from
//    the current amplitude to avoid clicks. See Voice::Trigger().
//
// 3. ZERO SUSTAIN: Setting sustain to 0% is valid - the note will decay
//    to silence and stay silent until released. We clamp to a tiny
//    positive value (0.001) to avoid floating-point edge cases.
//
// 4. VOICE DEACTIVATION: A voice becomes inactive when:
//    - Envelope reaches idle phase (after release completes), AND
//    - Amplitude falls below 0.0001 (silence threshold)
//
// 5. VELOCITY MODULATION: MIDI velocity can scale envelope times for expressiveness.
//    The "Env Velocity" parameter (0-100%) controls how much velocity affects times:
//    - At 0%: Velocity has no effect (organ-like, consistent response)
//    - At 100%: Hard hits have 10% of normal time (very snappy, piano-like)
//    - At 50% (default): Moderate scaling for balanced expressiveness
//    Formula: scaledTime = baseTime × (1 - sensitivity × velocity × 0.9)
//    This affects Attack, Decay, and Release times. Sustain level is unaffected.
//
// ═══════════════════════════════════════════════════════════════════════════════

// ═══════════════════════════════════════════════════════════════════════════════
// WAVETABLE OSCILLATOR - Band-Limited with Mip Levels
// ═══════════════════════════════════════════════════════════════════════════════
//
// WHAT IS A WAVETABLE?
// A wavetable stores pre-computed waveform cycles in memory. Instead of calculating
// waveforms mathematically each sample (expensive), we look up values from a table
// (fast). This also allows complex waveforms that can't be expressed as simple formulas.
//
// WHY MIPMAPS?
// At high frequencies, waveform harmonics can exceed Nyquist (sampleRate/2), causing
// aliasing artifacts. Mipmaps store multiple versions of the wavetable with progressively
// fewer harmonics. We select the appropriate mip level based on playback frequency.
//
// WHY STATIC ALLOCATION?
// WebAssembly (WASM) has a limited stack size (~64KB by default). A 1MB wavetable
// would overflow the stack if allocated as a local variable. Static storage places
// the data in the binary's data segment, avoiding stack limitations.
//
// ═══════════════════════════════════════════════════════════════════════════════

// ═══════════════════════════════════════════════════════════════════════════════
// UNISON ENGINE - Multiple Detuned Voices for Massive Sound
// ═══════════════════════════════════════════════════════════════════════════════
//
// WHAT IS UNISON?
// Unison stacks multiple copies of the same oscillator, each slightly detuned,
// to create a thick, chorused, "wall of sound" effect. This is THE defining
// feature of modern synthesizer sounds, especially EDM supersaws and leads.
//
// WITHOUT UNISON (1 voice):
//   440Hz ─────────────────────────────────► Thin, single tone
//
// WITH UNISON (7 voices, symmetric spread):
//   Voice 1: 437.0Hz ─┐  (leftmost, lowest)
//   Voice 2: 438.5Hz ─┤
//   Voice 3: 439.5Hz ─┤
//   Voice 4: 440.0Hz ─┼─────────────────────► MASSIVE, chorused, wide
//   Voice 5: 440.5Hz ─┤
//   Voice 6: 441.5Hz ─┤
//   Voice 7: 443.0Hz ─┘  (rightmost, highest)
//
// The slight frequency differences create "beating" - the voices drift in and
// out of phase, creating constant movement and thickness. This is fundamentally
// different from reverb or delay - it's a pitch-based chorus effect.
//
// DETUNE SPREAD ALGORITHM:
// We use a symmetric spread around the center frequency. For N voices:
//   - Voice 0 (center): no detune
//   - Voices 1,2,...: alternating +/- detune, increasing outward
//
// Detune amount in cents: spreadCents × (voiceIndex / (numVoices - 1))
// where spreadCents = detuneParam × kMaxUnisonDetuneCents
//
// STEREO WIDTH:
// Outer voices are panned to create stereo spread:
//   - Center voice: mono (center)
//   - Voice pairs: panned L/R symmetrically
//   - Width parameter controls how far they're panned
//
// This creates the "huge" stereo image of modern synth sounds.
//
// BLEND CONTROL:
// Controls the mix between the center voice and the detuned voices:
//   - 0%: Only center voice (thin, but stable pitch)
//   - 50%: Equal mix (balanced thickness)
//   - 100%: Only detuned voices (maximum thickness, but pitch can sound wobbly)
//
// CLASSIC SYNTHS WITH UNISON:
// - Roland JP-8000: Invented the "Supersaw" (7-voice unison saw)
// - Access Virus: Up to 9 unison voices
// - Xfer Serum: 16 unison voices per oscillator
// - Vital: 16 unison voices with advanced spread modes
//
// IMPLEMENTATION NOTES:
// - We support up to 8 unison voices (good balance of CPU vs. sound)
// - Each unison voice has its own phase accumulator (free-running)
// - Unison applies to BOTH oscillators simultaneously
// - All unison parameters are smoothed to prevent clicks
// - CPU scales linearly with unison count (8x at max)
//
// ═══════════════════════════════════════════════════════════════════════════════

// ═══════════════════════════════════════════════════════════════════════════════
// OSCILLATOR SYNC - Classic Analog Synthesis Technique
// ═══════════════════════════════════════════════════════════════════════════════
//
// WHAT IS OSCILLATOR SYNC?
// Hard sync forces one oscillator (the "slave") to reset its phase whenever
// another oscillator (the "master") completes a cycle. This creates complex,
// harmonically rich waveforms that are impossible to achieve otherwise.
//
// HARD SYNC VISUALIZATION:
//
//   Master (Osc1):  /|  /|  /|  /|  /|  /|    (completes full cycles)
//                   / | / | / | / | / | / |
//   ────────────────────────────────────────
//   Slave (Osc2):   /| /| /| /| /| /| /| /|   (faster, gets reset)
//    before sync    / |/ |/ |/ |/ |/ |/ |/ |
//   ────────────────────────────────────────
//   Slave (Osc2):   /|  /|  /|  /|  /|  /|    (after sync - truncated!)
//    after sync     / ‾ / ‾ / ‾ / ‾ / ‾ / ‾
//
// When the slave is at a higher pitch than the master, each cycle gets
// "cut off" partway through, creating asymmetrical waveforms with complex
// harmonic content that changes as you sweep the slave pitch.
//
// THE "SYNC SWEEP" SOUND:
// The classic sync sound is achieved by:
// 1. Setting both oscillators to sawtooth
// 2. Enabling hard sync
// 3. Sweeping Osc2's pitch up (using octave or an envelope)
//
// As Osc2's pitch rises, more partial cycles fit into each master cycle,
// creating a distinctive "tearing" or "ripping" sound that's harmonically
// related to the master frequency.
//
// FAMOUS USES OF SYNC:
// - The Cars "Let's Go" - that iconic lead sound
// - Van Halen "Jump" - Oberheim sync brass
// - Depeche Mode - countless tracks
// - Gary Numan - signature sound
//
// ANTI-ALIASING CONSIDERATION:
// Naive digital sync creates aliasing because the phase reset introduces a
// discontinuity. Our implementation uses PolyBLEP-style oscillators which
// help, but for perfect anti-aliasing you'd need BLIT or oversampling.
// For this educational synth, we accept minor aliasing at extreme settings.
//
// ═══════════════════════════════════════════════════════════════════════════════

// Unison configuration
constexpr int kMaxUnisonVoices = 8;           // Maximum unison voices (CPU-friendly)
constexpr float kMaxUnisonDetuneCents = 50.0f; // Max detune spread in cents (±25 from center)

// Wavetable dimensions
constexpr int kWavetableSize = 2048;      // Samples per cycle (power of 2 for fast wrapping)
constexpr int kWavetableFrames = 16;      // Morph positions (more = smoother morphing)
constexpr int kNumMipLevels = 8;          // 8 mip levels: 128→64→32→16→8→4→2→1 harmonics
constexpr float kWavetableSizeF = static_cast<float>(kWavetableSize);

// Morph configuration - 4 classic waveforms
constexpr int kNumWaveShapes = 4;         // Sine, Triangle, Saw, Square

// Memory: 2048 × 16 × 8 × 4 bytes = 1MB (static allocation, not stack)

// ═══════════════════════════════════════════════════════════════════════════════
// MATHEMATICAL CONSTANTS
// ═══════════════════════════════════════════════════════════════════════════════
// Named constants for commonly used values. Improves readability and avoids
// magic numbers scattered throughout the codebase.
// ═══════════════════════════════════════════════════════════════════════════════
constexpr float kPi = 3.14159265358979323846f;
constexpr float kTwoPi = 2.0f * kPi;              // Full circle in radians
constexpr float kHalfPi = 0.5f * kPi;             // Quarter circle
constexpr float kQuarterPi = 0.25f * kPi;         // Eighth circle (used in constant-power pan)
constexpr float kSqrtHalf = 0.7071067811865476f;  // 1/sqrt(2), equal power stereo

// ═══════════════════════════════════════════════════════════════════════════════
// DSP UTILITIES
// ═══════════════════════════════════════════════════════════════════════════════

// Calculate one-pole smoothing coefficient from time constant
// This creates a sample-rate independent smoothing filter.
// Formula: coeff = 1 - e^(-1 / (time * sampleRate))
// At each sample: smoothed += coeff * (target - smoothed)
inline float calcSmoothingCoeff(float timeSeconds, float sampleRate)
{
  return 1.0f - std::exp(-1.0f / (timeSeconds * sampleRate));
}

// ═══════════════════════════════════════════════════════════════════════════════
// DENORMAL PREVENTION
// ═══════════════════════════════════════════════════════════════════════════════
// Denormals are tiny floating point numbers (< 1e-38) that occur in filter
// feedback loops as signals decay. Processing denormals is 10-100x slower
// than normal floats, causing audio dropouts. We flush them to zero.
constexpr float kDenormalThreshold = 1e-15f;

inline float flushDenormal(float x)
{
  // Simple denormal flush - if |x| < threshold, return 0
  // More sophisticated approaches exist (FTZ/DAZ CPU flags) but this is portable
  return (std::abs(x) < kDenormalThreshold) ? 0.0f : x;
}

// Soft saturation using fast tanh approximation
// Creates warm, analog-style clipping instead of harsh digital clipping
inline float softSaturate(float x)
{
  // Pade approximant of tanh, accurate to ~0.1% for |x| < 3
  // Much faster than std::tanh
  if (x > 3.0f) return 1.0f;
  if (x < -3.0f) return -1.0f;
  float x2 = x * x;
  return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

// ═══════════════════════════════════════════════════════════════════════════════
// RESONANT FILTER - Using Q Library's Biquad Filters
// ═══════════════════════════════════════════════════════════════════════════════
//
// This filter uses Q library's biquad implementations (Audio-EQ Cookbook).
// - Lowpass, Highpass: True 2nd-order biquad responses
// - Bandpass: Constant Skirt Gain (CSG) - peak rises with Q
// - Notch: Complementary filter (input - bandpass_cpg)
//
// WHY BIQUADS WITH SMOOTHING?
// Biquads can cause zipper noise when coefficients change abruptly. We solve
// this by smoothing the cutoff and resonance parameters before updating
// coefficients. This gives us:
// - Full 20Hz-20kHz frequency range (no stability limits)
// - True filter responses for LP/HP/BP
// - Click-free parameter automation
//
// NOTCH IMPLEMENTATION:
// We use the complementary filter technique: notch = input - bandpass.
// The bandpass_cpg (Constant Peak Gain) ensures unity gain at center frequency,
// so subtraction creates a true notch with full attenuation at cutoff.
//
// WHAT IS Q (RESONANCE)?
// Q controls the "sharpness" of the filter at the cutoff frequency.
// - Q = 0.707: Butterworth (maximally flat passband)
// - Q = 1-5: Moderate resonance, musical "sweep" sound
// - Q = 10+: Sharp peak, near self-oscillation
//
// ═══════════════════════════════════════════════════════════════════════════════

#include <q/fx/biquad.hpp>

enum class FilterType
{
  Lowpass = 0,
  Highpass,
  Bandpass,
  Notch,
  kNumFilterTypes
};

class ResonantFilter
{
public:
  void SetSampleRate(float sampleRate)
  {
    mSampleRate = sampleRate;
    mSmoothCoeff = calcSmoothingCoeff(0.005f, sampleRate);  // 5ms smoothing

    // Biquads are stable up to Nyquist, but we cap at 0.45 * sampleRate
    // to avoid numerical issues very close to Nyquist
    mMaxCutoffHz = sampleRate * 0.45f;  // ~21.6kHz at 48kHz

    // Reinitialize all filters at new sample rate
    UpdateAllFilters();
  }

  // Set cutoff frequency in Hz (20 - 20000)
  void SetCutoff(float freqHz)
  {
    mTargetCutoffHz = std::max(20.0f, std::min(mMaxCutoffHz, freqHz));
  }

  // Set resonance (0.0 - 1.0) - mapped to Q range
  void SetResonance(float resonance)
  {
    // Map 0-1 resonance to Q range: 0.5 (gentle) to 15 (near self-oscillation)
    // Using exponential mapping for more musical feel
    mTargetResonance = std::max(0.0f, std::min(1.0f, resonance));
  }

  void SetType(FilterType type)
  {
    if (mType != type)
    {
      mType = type;
      // Reset filter states when switching types to avoid artifacts
      ResetFilterStates();
      // Ensure the new filter type has current coefficients
      UpdateAllFilters();
    }
  }

  void Reset()
  {
    ResetFilterStates();
    mCurrentCutoffHz = mTargetCutoffHz;
    mCurrentResonance = mTargetResonance;
    // Sync biquad coefficients to current values
    // Without this, the filter uses stale coefficients after note trigger
    UpdateAllFilters();
  }

  // Process a single sample through the filter
  float Process(float input)
  {
    // ─────────────────────────────────────────────────────────────────────────
    // SMOOTH PARAMETERS
    // Prevents zipper noise by interpolating towards target values
    // ─────────────────────────────────────────────────────────────────────────
    bool needsUpdate = false;

    float cutoffDiff = mTargetCutoffHz - mCurrentCutoffHz;
    if (std::abs(cutoffDiff) > 0.1f)
    {
      mCurrentCutoffHz += mSmoothCoeff * cutoffDiff;
      needsUpdate = true;
    }

    float resoDiff = mTargetResonance - mCurrentResonance;
    if (std::abs(resoDiff) > 0.001f)
    {
      mCurrentResonance += mSmoothCoeff * resoDiff;
      needsUpdate = true;
    }

    // Recalculate coefficients for ALL filters when parameters change
    // This ensures switching filter types always has correct coefficients
    if (needsUpdate)
    {
      UpdateAllFilters();
    }

    // ─────────────────────────────────────────────────────────────────────────
    // FILTER PROCESSING
    // Each filter type uses its own biquad with correct coefficients
    // ─────────────────────────────────────────────────────────────────────────
    float output;
    switch (mType)
    {
      case FilterType::Lowpass:
        output = mLowpass(input);
        FlushBiquadDenormals(mLowpass);
        break;

      case FilterType::Highpass:
        output = mHighpass(input);
        FlushBiquadDenormals(mHighpass);
        break;

      case FilterType::Bandpass:
        output = mBandpass(input);
        FlushBiquadDenormals(mBandpass);
        break;

      case FilterType::Notch:
        // Notch = input - bandpass_cpg (complementary filter technique)
        // bandpass_cpg has unity gain at center frequency, so:
        //   At cutoff: output = input - input = 0 (full attenuation)
        //   Away from cutoff: output ≈ input (passthrough)
        {
          float bpOut = mNotchBandpass(input);
          FlushBiquadDenormals(mNotchBandpass);
          output = input - bpOut;
        }
        break;

      default:
        output = mLowpass(input);
        break;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // SOFT SATURATION
    // At high resonance, filter can exceed unity gain. Apply gentle saturation
    // to prevent harsh digital clipping while preserving the character.
    // ─────────────────────────────────────────────────────────────────────────
    if (mCurrentResonance > 0.5f)
    {
      float satAmount = (mCurrentResonance - 0.5f) * 2.0f;
      float dry = output;
      float wet = softSaturate(output * 1.5f);
      output = dry + satAmount * (wet - dry);
    }

    return output;
  }

private:
  // Map resonance (0-1) to Q factor using exponential curve
  // Formula: Q = 0.5 × 30^resonance
  //   0.0 → Q = 0.5   (very gentle, below Butterworth)
  //   0.5 → Q ≈ 2.74  (moderate resonance)
  //   1.0 → Q = 15    (high resonance, near self-oscillation)
  float ResonanceToQ(float resonance) const
  {
    return 0.5f * std::pow(30.0f, resonance);
  }

  void UpdateAllFilters()
  {
    if (mSampleRate <= 0.0f) return;

    q::frequency freq{mCurrentCutoffHz};
    double qVal = ResonanceToQ(mCurrentResonance);

    mLowpass.config(freq, mSampleRate, qVal);
    mHighpass.config(freq, mSampleRate, qVal);
    mBandpass.config(freq, mSampleRate, qVal);
    mNotchBandpass.config(freq, mSampleRate, qVal);
  }

  void ResetFilterStates()
  {
    mLowpass.x1 = mLowpass.x2 = mLowpass.y1 = mLowpass.y2 = 0.0f;
    mHighpass.x1 = mHighpass.x2 = mHighpass.y1 = mHighpass.y2 = 0.0f;
    mBandpass.x1 = mBandpass.x2 = mBandpass.y1 = mBandpass.y2 = 0.0f;
    mNotchBandpass.x1 = mNotchBandpass.x2 = mNotchBandpass.y1 = mNotchBandpass.y2 = 0.0f;
  }

  void FlushBiquadDenormals(q::biquad& bq)
  {
    bq.x1 = flushDenormal(bq.x1);
    bq.x2 = flushDenormal(bq.x2);
    bq.y1 = flushDenormal(bq.y1);
    bq.y2 = flushDenormal(bq.y2);
  }

  float mSampleRate = 48000.0f;
  float mMaxCutoffHz = 20000.0f;
  float mTargetCutoffHz = 10000.0f;
  float mTargetResonance = 0.0f;
  float mCurrentCutoffHz = 10000.0f;
  float mCurrentResonance = 0.0f;
  float mSmoothCoeff = 0.01f;

  // Q library biquad filters - one for each type
  // Initialized with default values, reconfigured in SetSampleRate()
  q::lowpass mLowpass{q::frequency{1000.0f}, 48000.0f, 0.707};
  q::highpass mHighpass{q::frequency{1000.0f}, 48000.0f, 0.707};
  q::bandpass_csg mBandpass{q::frequency{1000.0f}, 48000.0f, 0.707};
  // Notch uses bandpass_cpg (Constant Peak Gain) + subtraction
  // CPG has unity gain at center frequency, so input - bandpass = true notch
  // (CSG would have variable peak gain, causing incomplete notch attenuation)
  q::bandpass_cpg mNotchBandpass{q::frequency{1000.0f}, 48000.0f, 0.707};

  FilterType mType = FilterType::Lowpass;
};

class WavetableOscillator
{
public:
  // Mipmapped wavetable: [mipLevel][frame][sample]
  using MipLevel = std::array<std::array<float, kWavetableSize>, kWavetableFrames>;
  using WavetableData = std::array<MipLevel, kNumMipLevels>;

  void SetWavetable(const WavetableData* table) { mTable = table; }

  void SetSampleRate(float sampleRate)
  {
    mSampleRate = sampleRate;
    mNyquist = sampleRate * 0.5f;
    // Position smoothing: 10ms time constant for glitch-free morphing
    // This prevents audible clicks when the WT Position knob is moved quickly
    mSmoothCoeff = calcSmoothingCoeff(0.01f, sampleRate);
  }

  void SetFrequency(float freq, float sampleRate)
  {
    mPhaseInc = freq / sampleRate;
    mFrequency = freq;
    mNyquist = sampleRate * 0.5f;
  }

  void SetPosition(float pos)
  {
    mTargetPosition = std::max(0.0f, std::min(1.0f, pos)) * (kWavetableFrames - 1);
  }

  void Reset() { mPhase = 0.0f; }

  // ─────────────────────────────────────────────────────────────────────────────
  // PHASE ACCESS FOR HARD SYNC
  // Hard sync requires reading Osc1's phase to detect zero-crossings, and
  // resetting Osc2's phase when sync triggers. These methods expose the
  // internal phase for that purpose.
  // ─────────────────────────────────────────────────────────────────────────────
  float GetPhase() const { return mPhase; }
  void ResetPhase() { mPhase = 0.0f; }

  float Process()
  {
    if (!mTable) return 0.0f;

    // ─────────────────────────────────────────────────────────────────────────
    // STEP 1: Smooth the morph position (one-pole lowpass filter)
    // This prevents clicks when the position changes rapidly
    // ─────────────────────────────────────────────────────────────────────────
    mPosition += mSmoothCoeff * (mTargetPosition - mPosition);

    // ─────────────────────────────────────────────────────────────────────────
    // STEP 2: Calculate mip level based on playback frequency and Nyquist
    // We select the mip level that has enough harmonics without aliasing.
    // Formula: mip = log2(baseHarmonics * frequency / Nyquist)
    //
    // Example at 48kHz (Nyquist = 24kHz), 128 base harmonics:
    //   100Hz: log2(128 * 100 / 24000) = log2(0.53) ≈ -0.9 → mip 0 (128 harmonics)
    //   440Hz: log2(128 * 440 / 24000) = log2(2.35) ≈ 1.2 → mip 1 (64 harmonics)
    //   2kHz:  log2(128 * 2000 / 24000) = log2(10.7) ≈ 3.4 → mip 3 (16 harmonics)
    //
    // DESIGN NOTE: We use floor() for mip selection, which prioritizes brightness
    // over perfect anti-aliasing. At transition points (e.g., 440Hz using mip 1),
    // some harmonics may slightly exceed Nyquist, but the crossfade to the next
    // mip level (fewer harmonics) attenuates them. ceil() would be alias-free but darker.
    // ─────────────────────────────────────────────────────────────────────────
    constexpr float kBaseHarmonics = 128.0f;
    float mipFloat = std::log2(std::max(1.0f, kBaseHarmonics * mFrequency / mNyquist));
    mipFloat = std::max(0.0f, std::min(mipFloat, static_cast<float>(kNumMipLevels - 1)));

    int mip0 = static_cast<int>(mipFloat);  // floor - prioritizes brightness
    int mip1 = std::min(mip0 + 1, kNumMipLevels - 1);
    float mipFrac = mipFloat - mip0;

    // ─────────────────────────────────────────────────────────────────────────
    // STEP 3: Calculate frame indices for morph interpolation
    // ─────────────────────────────────────────────────────────────────────────
    int frame0 = static_cast<int>(mPosition);
    int frame1 = std::min(frame0 + 1, kWavetableFrames - 1);
    float frameFrac = mPosition - frame0;

    // ─────────────────────────────────────────────────────────────────────────
    // STEP 4: Calculate sample indices with wrapping
    // Using bitwise AND for fast modulo (works because size is power of 2)
    // ─────────────────────────────────────────────────────────────────────────
    float samplePos = mPhase * kWavetableSizeF;
    int idx0 = static_cast<int>(samplePos) & (kWavetableSize - 1);
    int idx1 = (idx0 + 1) & (kWavetableSize - 1);
    float sampleFrac = samplePos - std::floor(samplePos);

    // ─────────────────────────────────────────────────────────────────────────
    // STEP 5: Trilinear interpolation (mip × frame × sample)
    // We interpolate along 3 axes for smooth output:
    // - Between adjacent samples (removes stepping artifacts)
    // - Between adjacent frames (smooth morphing)
    // - Between adjacent mip levels (smooth anti-aliasing transition)
    //
    // Linear interpolation formula: result = a + frac * (b - a)
    // Quality note: Linear gives ~-40dB THD. Cubic Hermite would give ~-80dB.
    // ─────────────────────────────────────────────────────────────────────────
    auto sampleFrame = [&](int mip, int frame) {
      const auto& t = (*mTable)[mip][frame];
      return t[idx0] + sampleFrac * (t[idx1] - t[idx0]);
    };

    // Interpolate within each mip level (frame + sample)
    float s_mip0_f0 = sampleFrame(mip0, frame0);
    float s_mip0_f1 = sampleFrame(mip0, frame1);
    float s_mip0 = s_mip0_f0 + frameFrac * (s_mip0_f1 - s_mip0_f0);

    float s_mip1_f0 = sampleFrame(mip1, frame0);
    float s_mip1_f1 = sampleFrame(mip1, frame1);
    float s_mip1 = s_mip1_f0 + frameFrac * (s_mip1_f1 - s_mip1_f0);

    // Final mip interpolation
    float sample = s_mip0 + mipFrac * (s_mip1 - s_mip0);

    // ─────────────────────────────────────────────────────────────────────────
    // STEP 6: Advance phase accumulator
    // Phase wraps from 0.0 to 1.0, representing one cycle of the waveform
    // Using while loop handles edge case where phaseInc > 1.0 (freq > sampleRate)
    // ─────────────────────────────────────────────────────────────────────────
    mPhase += mPhaseInc;
    while (mPhase >= 1.0f) mPhase -= 1.0f;

    return sample;
  }

  // ─────────────────────────────────────────────────────────────────────────────
  // PROCESS AT EXTERNAL PHASE (for unison voices)
  // ─────────────────────────────────────────────────────────────────────────────
  // This method allows unison voices to share the wavetable and morph position
  // while each having their own phase for detuning. The external phase is passed
  // by reference and advanced after sampling.
  //
  // IMPORTANT: Call Process() first for voice 0 to update the morph position
  // smoothing, then call ProcessAtPhase() for additional unison voices.
  // ─────────────────────────────────────────────────────────────────────────────
  float ProcessAtPhase(float& phase, float phaseInc, float frequency)
  {
    if (!mTable) return 0.0f;

    // Use the already-smoothed morph position from the main oscillator
    // (mPosition was updated by Process() call for voice 0)

    // Calculate mip level for this voice's frequency
    constexpr float kBaseHarmonics = 128.0f;
    float mipFloat = std::log2(std::max(1.0f, kBaseHarmonics * frequency / mNyquist));
    mipFloat = std::max(0.0f, std::min(mipFloat, static_cast<float>(kNumMipLevels - 1)));

    int mip0 = static_cast<int>(mipFloat);
    int mip1 = std::min(mip0 + 1, kNumMipLevels - 1);
    float mipFrac = mipFloat - mip0;

    // Frame interpolation (morph position)
    int frame0 = static_cast<int>(mPosition);
    int frame1 = std::min(frame0 + 1, kWavetableFrames - 1);
    float frameFrac = mPosition - frame0;

    // Sample position
    float samplePos = phase * kWavetableSizeF;
    int idx0 = static_cast<int>(samplePos) & (kWavetableSize - 1);
    int idx1 = (idx0 + 1) & (kWavetableSize - 1);
    float sampleFrac = samplePos - std::floor(samplePos);

    // Trilinear interpolation
    auto sampleFrame = [&](int mip, int frame) {
      const auto& t = (*mTable)[mip][frame];
      return t[idx0] + sampleFrac * (t[idx1] - t[idx0]);
    };

    float s_mip0_f0 = sampleFrame(mip0, frame0);
    float s_mip0_f1 = sampleFrame(mip0, frame1);
    float s_mip0 = s_mip0_f0 + frameFrac * (s_mip0_f1 - s_mip0_f0);

    float s_mip1_f0 = sampleFrame(mip1, frame0);
    float s_mip1_f1 = sampleFrame(mip1, frame1);
    float s_mip1 = s_mip1_f0 + frameFrac * (s_mip1_f1 - s_mip1_f0);

    float sample = s_mip0 + mipFrac * (s_mip1 - s_mip0);

    // Advance external phase
    phase += phaseInc;
    while (phase >= 1.0f) phase -= 1.0f;

    return sample;
  }

private:
  const WavetableData* mTable = nullptr;
  float mSampleRate = 48000.0f;
  float mNyquist = 24000.0f;  // Half sample rate, for mip level calculation
  float mFrequency = 440.0f;
  float mSmoothCoeff = 0.01f;
  float mPhase = 0.0f;
  float mPhaseInc = 0.0f;
  float mPosition = 0.0f;
  float mTargetPosition = 0.0f;
};

// ═══════════════════════════════════════════════════════════════════════════════
// WAVETABLE GENERATOR - Band-Limited Additive Synthesis
// ═══════════════════════════════════════════════════════════════════════════════
//
// WHY ADDITIVE SYNTHESIS?
// Naive waveforms (e.g., saw = 2*phase - 1) contain infinite harmonics, causing
// aliasing at high frequencies. Additive synthesis builds waveforms from individual
// sine harmonics, allowing us to stop before reaching Nyquist.
//
// FOURIER SERIES REFERENCE:
// - Saw:      Σ sin(n·x) / n           (all harmonics)
// - Square:   Σ sin(n·x) / n           (odd harmonics only: 1, 3, 5, ...)
// - Triangle: Σ (-1)^k · sin(n·x) / n² (odd harmonics, alternating sign)
//
// ═══════════════════════════════════════════════════════════════════════════════

class WavetableGenerator
{
public:
  using WavetableData = WavetableOscillator::WavetableData;
  // Using global kPi and kTwoPi constants defined at file level

  // Harmonics per mip level - halves each octave to stay below Nyquist
  // Mip 0 has 128 harmonics (used for fundamentals up to ~187Hz at 48kHz)
  // Formula: maxFreq = Nyquist / harmonics = 24000 / 128 ≈ 187Hz
  // Each mip halves harmonics: mip1=64, mip2=32, ... mip7=1
  static constexpr int kBaseHarmonics = 128;
  static int GetMaxHarmonics(int mipLevel)
  {
    return std::max(1, kBaseHarmonics >> mipLevel);
  }

  // ─────────────────────────────────────────────────────────────────────────────
  // BAND-LIMITED WAVEFORM GENERATORS
  // Each function computes the Fourier series sum for one sample position
  // ─────────────────────────────────────────────────────────────────────────────

  // Sawtooth: sum of all harmonics with 1/n amplitude rolloff
  // Fourier: saw(x) = -(2/π) · Σ sin(n·x)/n, n=1 to ∞
  static float GenerateBandLimitedSaw(float phase, int maxHarmonics)
  {
    float sample = 0.0f;
    for (int h = 1; h <= maxHarmonics; h++)
    {
      sample -= std::sin(phase * kTwoPi * h) / static_cast<float>(h);
    }
    return sample * (2.0f / kPi);  // Normalize to [-1, 1]
  }

  // Square: odd harmonics only (1, 3, 5, ...) with 1/n amplitude
  // Fourier: square(x) = (4/π) · Σ sin(n·x)/n, n=1,3,5,...
  static float GenerateBandLimitedSquare(float phase, int maxHarmonics)
  {
    float sample = 0.0f;
    for (int h = 1; h <= maxHarmonics; h += 2)  // Odd harmonics only
    {
      sample += std::sin(phase * kTwoPi * h) / static_cast<float>(h);
    }
    return sample * (4.0f / kPi);  // Normalize to [-1, 1]
  }

  // Triangle: odd harmonics with 1/n² rolloff and alternating signs
  // Fourier: tri(x) = (8/π²) · Σ (-1)^k · sin(n·x)/n², n=1,3,5,..., k=(n-1)/2
  static float GenerateBandLimitedTriangle(float phase, int maxHarmonics)
  {
    float sample = 0.0f;
    for (int h = 1; h <= maxHarmonics; h += 2)  // Odd harmonics only
    {
      // Alternating sign: +1 for h=1, -1 for h=3, +1 for h=5, ...
      float sign = ((h - 1) / 2 % 2 == 0) ? 1.0f : -1.0f;
      sample += sign * std::sin(phase * kTwoPi * h) / static_cast<float>(h * h);
    }
    return sample * (8.0f / (kPi * kPi));  // Normalize to [-1, 1]
  }

  // ─────────────────────────────────────────────────────────────────────────────
  // WAVETABLE GENERATION
  // Creates a complete mipmapped wavetable with morphing between 4 shapes
  // ─────────────────────────────────────────────────────────────────────────────

  static const WavetableData& GenerateBasicShapes()
  {
    // STATIC ALLOCATION: Table lives in data segment, not stack!
    // This is critical for WASM where stack is limited to ~64KB
    static WavetableData table{};
    static bool generated = false;

    if (generated) return table;  // Return cached reference (no copy)
    generated = true;

    // Generate each mip level with appropriate harmonic content
    for (int mip = 0; mip < kNumMipLevels; mip++)
    {
      int maxHarmonics = GetMaxHarmonics(mip);

      for (int frame = 0; frame < kWavetableFrames; frame++)
      {
        // Map frame index to morph position [0, 1]
        float t = static_cast<float>(frame) / (kWavetableFrames - 1);

        for (int i = 0; i < kWavetableSize; i++)
        {
          float phase = static_cast<float>(i) / kWavetableSize;

          // Generate all 4 band-limited waveforms at this phase
          float sine = std::sin(phase * kTwoPi);  // Sine has no harmonics to limit
          float triangle = GenerateBandLimitedTriangle(phase, maxHarmonics);
          float saw = GenerateBandLimitedSaw(phase, maxHarmonics);
          float square = GenerateBandLimitedSquare(phase, maxHarmonics);

          // ─────────────────────────────────────────────────────────────────
          // DATA-DRIVEN MORPHING
          // Maps morph position t ∈ [0,1] to interpolation between 4 shapes:
          //   t = 0.00 → Sine
          //   t = 0.33 → Triangle
          //   t = 0.66 → Saw
          //   t = 1.00 → Square
          //
          // Using named constants instead of magic numbers for clarity
          // ─────────────────────────────────────────────────────────────────
          float shapePos = t * (kNumWaveShapes - 1);  // 0 to 3
          int shape0 = static_cast<int>(shapePos);
          int shape1 = std::min(shape0 + 1, kNumWaveShapes - 1);
          float blend = shapePos - shape0;

          // Get waveform values for the two shapes we're blending between
          float shapes[kNumWaveShapes] = {sine, triangle, saw, square};
          float sample = shapes[shape0] + blend * (shapes[shape1] - shapes[shape0]);

          table[mip][frame][i] = sample;
        }
      }
    }
    return table;
  }
};

// ═══════════════════════════════════════════════════════════════════════════════
// WAVEFORM TYPES
// ═══════════════════════════════════════════════════════════════════════════════
// Defines available oscillator modes. The first 4 use Q library's direct
// oscillators (computed mathematically). kWaveformWavetable uses our mipmapped
// wavetable with morphing between Sine→Triangle→Saw→Square.
//
// MORPH ORDER RATIONALE (Sine→Triangle→Saw→Square):
// Ordered by increasing harmonic richness and timbral complexity:
//   Sine:     1 harmonic  (fundamental only) - pure, clean
//   Triangle: Odd harmonics, 1/n² rolloff   - soft, flute-like
//   Saw:      All harmonics, 1/n rolloff    - rich, full (128 harmonics)
//   Square:   Odd harmonics, 1/n rolloff    - hollow, nasal (64 harmonics)
// Note: Saw has MORE harmonics than Square (all vs odd-only), but Square
// sounds harsher/buzzier due to its hollow spectrum. This order goes from
// pure → soft → rich → hollow, a natural timbral progression for sweeps.
// ═══════════════════════════════════════════════════════════════════════════════
enum EWaveform
{
  kWaveformSine = 0,    // Pure sine - 1 harmonic
  kWaveformSaw,         // Sawtooth - all harmonics
  kWaveformSquare,      // Square - odd harmonics (50% duty cycle)
  kWaveformTriangle,    // Triangle - odd harmonics, 1/n² rolloff
  kWaveformPulse,       // Pulse - variable duty cycle (PWM), PolyBLEP anti-aliased
  kWaveformFM,          // FM synthesis - modulator modulates carrier phase
  kWaveformWavetable,   // Morphing wavetable (Sine→Triangle→Saw→Square)
  kNumWaveforms
};

// ═══════════════════════════════════════════════════════════════════════════════
// PLUGIN DSP ENGINE
// ═══════════════════════════════════════════════════════════════════════════════
//
// ARCHITECTURE OVERVIEW:
// PluginInstanceDSP is the main DSP engine that manages synthesis and audio output.
// It uses iPlug2's MidiSynth infrastructure for voice allocation and MIDI handling.
//
// SIGNAL FLOW:
//   MIDI → MidiSynth → Voice[] → Mix → Master Gain → Output
//
//   1. MIDI events are queued via ProcessMidiMsg()
//   2. MidiSynth allocates/triggers voices (polyphonic, 8 voices default)
//   3. Each Voice generates: Oscillator → Filter → Envelope → output
//   4. Voice outputs are summed (accumulated) into the output buffer
//   5. Master gain is applied with smoothing for click-free automation
//
// VOICE ARCHITECTURE:
//   Each Voice contains independent instances of all DSP components,
//   allowing per-note filtering and envelope behavior. Voices accumulate
//   their output (+=) rather than overwriting, enabling polyphony.
//
// ═══════════════════════════════════════════════════════════════════════════════
template<typename T>
class PluginInstanceDSP
{
public:
  // ─────────────────────────────────────────────────────────────────────────────
  // VOICE - Single Polyphonic Voice
  // ─────────────────────────────────────────────────────────────────────────────
  // A Voice represents one "note" in the polyphonic synth. Each voice has:
  //   - Its own oscillator (phase, frequency, waveform selection)
  //   - Its own filter (cutoff/resonance applied per-note)
  //   - Its own ADSR envelope (attack/decay/sustain/release)
  //
  // Voices are managed by iPlug2's MidiSynth/VoiceAllocator. When a MIDI note-on
  // arrives, VoiceAllocator finds a free voice (or steals one) and calls Trigger().
  // When note-off arrives, it calls Release(). Voices accumulate their output
  // into the shared buffer, enabling polyphony.
  //
  // RETRIGGER BEHAVIOR:
  // When a voice is retriggered while still sounding (isRetrigger=true), we
  // preserve the oscillator phase for smooth legato, but reset the envelope
  // with a brief crossfade to avoid clicks. See Trigger() for details.
  // ─────────────────────────────────────────────────────────────────────────────
#pragma mark - Voice
  class Voice : public SynthVoice
  {
  public:
    Voice()
    {
      mEnvConfig = q::adsr_envelope_gen::config{
        q::duration{0.01f},   // 10ms attack
        q::duration{0.1f},    // 100ms decay
        q::lin_to_db(0.7f),   // 70% sustain level (in dB via lin_to_db)
        50_s,                 // Sustain hold time - how long to hold at sustain level
                              // before auto-release. 50s = effectively "forever" (until note-off)
        q::duration{0.2f}     // 200ms release
      };
    }

    // Set shared wavetable data (called once from DSP class)
    void SetWavetable(const WavetableOscillator::WavetableData* table)
    {
      mWavetableOsc.SetWavetable(table);
      mOsc2WavetableOsc.SetWavetable(table);  // Osc2 shares the same wavetable
    }

    bool GetBusy() const override
    {
      return mActive && !mEnv.in_idle_phase();
    }

    void Trigger(double level, bool isRetrigger) override
    {
      // Capture current envelope level BEFORE creating new envelope
      // This is needed for smooth retriggering (legato playing)
      float currentLevel = mActive ? mEnv.current() : 0.0f;

      mActive = true;
      mVelocity = static_cast<float>(level);

      if (!isRetrigger)
      {
        // New note - reset oscillator phase and filter for consistent attack
        mPhase = q::phase_iterator{};
        mWavetableOsc.Reset();
        mFilterL.Reset();  // Clear filter state to avoid artifacts from previous notes
        mFilterR.Reset();

        // Reset all per-unison-voice FM modulator phases for consistent FM timbre on attack
        for (int v = 0; v < kMaxUnisonVoices; v++)
        {
          mOsc1FMModulatorPhases[v] = 0.0f;
          mOsc2FMModulatorPhases[v] = 0.0f;
        }
      }

      // ─────────────────────────────────────────────────────────────────────────
      // VELOCITY → ENVELOPE TIME MODULATION
      // Higher velocity = faster envelope times (snappier, more expressive)
      // Scale factor ranges from 1.0 (vel=0) to 0.1 (vel=1, full sensitivity)
      // This gives a 10:1 range which is musically useful without being extreme.
      //
      // Formula: scaledTime = baseTime × (1 - sensitivity × velocity × 0.9)
      //   - At sensitivity=0: times unchanged (organ-like)
      //   - At sensitivity=1, velocity=1: times reduced to 10% (very snappy)
      //   - At sensitivity=0.5, velocity=0.5: times reduced to 77.5% (subtle)
      // ─────────────────────────────────────────────────────────────────────────
      float velScale = 1.0f - mEnvVelocitySensitivity * mVelocity * 0.9f;

      // Create velocity-scaled envelope config for this note
      q::adsr_envelope_gen::config velEnvConfig = mEnvConfig;
      velEnvConfig.attack_rate = q::duration{mBaseAttackMs * 0.001f * velScale};
      velEnvConfig.decay_rate = q::duration{mBaseDecayMs * 0.001f * velScale};
      velEnvConfig.release_rate = q::duration{mBaseReleaseMs * 0.001f * velScale};

      // Create fresh envelope generator with velocity-scaled times
      mEnv = q::adsr_envelope_gen{velEnvConfig, static_cast<float>(mSampleRate)};
      mEnv.attack();

      // ─────────────────────────────────────────────────────────────────────────
      // RETRIGGER SMOOTHING
      // When retriggering a voice that's still sounding, we need to smoothly
      // transition from the current level to avoid clicks. We use a fast
      // exponential decay (~5ms) to bridge the gap.
      // ─────────────────────────────────────────────────────────────────────────
      if (currentLevel > 0.01f)
      {
        mRetriggerOffset = currentLevel;
        // Calculate decay coefficient for ~5ms fade (sample-rate independent)
        mRetriggerDecay = 1.0f - calcSmoothingCoeff(0.005f, static_cast<float>(mSampleRate));
      }
      else
      {
        mRetriggerOffset = 0.0f;
        mRetriggerDecay = 1.0f;
      }
    }

    void Release() override
    {
      mEnv.release();
    }

    void ProcessSamplesAccumulating(T** inputs, T** outputs,
                                     int nInputs, int nOutputs,
                                     int startIdx, int nFrames) override
    {
      // ─────────────────────────────────────────────────────────────────────────
      // PITCH CALCULATION (1V/Octave Format)
      // iPlug2's MidiSynth provides pitch in "1V/octave" format where:
      //   pitch = 0 → A4 (440Hz)
      //   pitch = 1 → A5 (880Hz)  - one octave up
      //   pitch = -1 → A3 (220Hz) - one octave down
      //
      // The formula freq = 440 × 2^pitch converts this to Hz.
      // MIDI note 69 (A4) gives pitch=0, each semitone is pitch += 1/12.
      //
      // Pitch bend is added before exponentiation for correct tuning behavior.
      // ─────────────────────────────────────────────────────────────────────────
      double pitch = mInputs[kVoiceControlPitch].endValue;
      double pitchBend = mInputs[kVoiceControlPitchBend].endValue;

      // Convert to frequency: 440Hz × 2^(pitch + bend)
      double targetFreq = 440.0 * std::pow(2.0, pitch + pitchBend);

      // ─────────────────────────────────────────────────────────────────────────
      // PORTAMENTO (GLIDE)
      // ─────────────────────────────────────────────────────────────────────────
      // Portamento smoothly glides from the previous pitch to the new pitch.
      // This is a classic synth feature for expressive lead lines and basses.
      //
      // We glide in the logarithmic domain (pitch) rather than linear (Hz) because
      // pitch perception is logarithmic. This ensures equal glide time across octaves.
      //
      // When portamento is 0, we snap instantly to the new pitch.
      // When portamento is > 0, we exponentially approach the target.
      // ─────────────────────────────────────────────────────────────────────────
      double baseFreq = targetFreq;
      if (mPortamentoCoeff < 1.0f && mCurrentBaseFreq > 0.0)
      {
        // Exponential glide: multiply current by coefficient and add remainder of target
        // This creates a smooth asymptotic approach to the target frequency
        mCurrentBaseFreq += mPortamentoCoeff * (targetFreq - mCurrentBaseFreq);
        baseFreq = mCurrentBaseFreq;
      }
      else
      {
        // No portamento or first note - snap to target
        mCurrentBaseFreq = targetFreq;
      }

      // ─────────────────────────────────────────────────────────────────────────
      // OSCILLATOR 1 FREQUENCY CALCULATION
      // Osc1 now has octave and detune controls for full symmetry with Osc2.
      // Formula: osc1Freq = baseFreq × 2^(octave + cents/1200)
      // ─────────────────────────────────────────────────────────────────────────
      double osc1FreqShift = static_cast<double>(mOsc1Octave) + mOsc1Detune / 1200.0;
      double freq = baseFreq * std::pow(2.0, osc1FreqShift);

      // Update Q library phase iterator with new frequency
      mPhase.set(q::frequency{static_cast<float>(freq)}, static_cast<float>(mSampleRate));

      // Update wavetable oscillator frequency (for mip level calculation)
      mWavetableOsc.SetFrequency(static_cast<float>(freq), static_cast<float>(mSampleRate));

      // ─────────────────────────────────────────────────────────────────────────
      // OSCILLATOR 2 FREQUENCY CALCULATION
      // Osc2 can be shifted by octaves and detuned by cents for fat, layered sounds.
      // Formula: osc2Freq = baseFreq × 2^octave × 2^(cents/1200)
      //        = baseFreq × 2^(octave + cents/1200)
      //
      // Example at A4 (440Hz):
      //   Octave=0, Detune=+7 cents: 440 × 2^(7/1200) = 441.78Hz (+1.78Hz beating)
      //   Octave=-1, Detune=0: 440 × 2^(-1) = 220Hz (one octave down)
      //   Octave=+1, Detune=-12 cents: 440 × 2^(1-12/1200) = 877Hz (octave up, slightly flat)
      // ─────────────────────────────────────────────────────────────────────────
      double osc2FreqShift = static_cast<double>(mOsc2Octave) + mOsc2Detune / 1200.0;
      double osc2Freq = baseFreq * std::pow(2.0, osc2FreqShift);

      // Update Osc2 phase iterator (note: phase is NOT reset in Trigger for free-running)
      mOsc2Phase.set(q::frequency{static_cast<float>(osc2Freq)}, static_cast<float>(mSampleRate));

      // Update Osc2 wavetable frequency
      mOsc2WavetableOsc.SetFrequency(static_cast<float>(osc2Freq), static_cast<float>(mSampleRate));

      // ─────────────────────────────────────────────────────────────────────────
      // ─────────────────────────────────────────────────────────────────────────
      // PRE-COMPUTE UNISON VOICE DATA (Per-Oscillator - Serum-style)
      // ─────────────────────────────────────────────────────────────────────────
      // Each oscillator has INDEPENDENT unison settings. This allows combinations
      // like: thick supersaw lead (Osc1: 8 voices) + clean sub bass (Osc2: 1 voice)
      //
      // DETUNE: Symmetric distribution around center pitch using alternating +/-
      //   Voice 0 (center): no detune
      //   Voice 1: +spread, Voice 2: -spread, Voice 3: +more, Voice 4: -more, ...
      //
      // STEREO PANNING: Perfectly symmetric spread from -width to +width
      //   - For odd spread voices: includes center (0)
      //   - For even spread voices: straddles center (no voice at 0)
      //   This ensures LEFT and RIGHT have equal energy regardless of voice count.
      //
      // Example for 8 voices (7 spread voices, odd):
      //   Pans: -1.0, -0.67, -0.33, 0, +0.33, +0.67, +1.0  (symmetric!)
      //
      // Example for 7 voices (6 spread voices, even):
      //   Pans: -0.83, -0.5, -0.17, +0.17, +0.5, +0.83  (symmetric, straddles 0)
      // ─────────────────────────────────────────────────────────────────────────

      // OSC1 UNISON SETUP
      if (mOsc1UnisonVoices > 1)
      {
        float spreadCents = mOsc1UnisonDetune * kMaxUnisonDetuneCents;
        int numSpreadVoices = mOsc1UnisonVoices - 1;  // Voices 1 to N-1

        for (int v = 0; v < mOsc1UnisonVoices; v++)
        {
          if (v == 0)
          {
            // ─────────────────────────────────────────────────────────────────
            // CENTER VOICE: No detune, no panning
            // ─────────────────────────────────────────────────────────────────
            mOsc1UnisonDetuneOffsets[v] = 0.0f;
            mOsc1UnisonPans[v] = 0.0f;
          }
          else
          {
            // ─────────────────────────────────────────────────────────────────
            // SPREAD VOICES: Symmetric detune and panning
            // ─────────────────────────────────────────────────────────────────

            // DETUNE: Alternating +/- with increasing magnitude
            int spreadLevel = (v + 1) / 2;  // 1,1,2,2,3,3,4,4...
            float detunePosition = static_cast<float>(spreadLevel) / static_cast<float>((mOsc1UnisonVoices) / 2);
            float detuneSign = (v % 2 == 1) ? 1.0f : -1.0f;
            mOsc1UnisonDetuneOffsets[v] = detuneSign * detunePosition * spreadCents;

            // PANNING: Perfectly symmetric spread
            // Formula changes based on odd/even spread voice count
            float panPosition;
            float idx = static_cast<float>(v - 1);  // 0-indexed within spread voices

            if (numSpreadVoices % 2 == 1)
            {
              // Odd spread voices: linear from -width to +width (includes 0)
              // pan = (2*idx - (N-1)) / (N-1) * width
              panPosition = (2.0f * idx - (numSpreadVoices - 1)) / static_cast<float>(numSpreadVoices - 1);
            }
            else
            {
              // Even spread voices: straddle center (no voice at 0)
              // pan = (2*idx + 1 - N) / N * width
              panPosition = (2.0f * idx + 1.0f - numSpreadVoices) / static_cast<float>(numSpreadVoices);
            }
            mOsc1UnisonPans[v] = panPosition * mOsc1UnisonWidth;
          }
        }
      }

      // OSC2 UNISON SETUP (Independent from Osc1, same symmetric algorithm)
      if (mOsc2UnisonVoices > 1)
      {
        float spreadCents = mOsc2UnisonDetune * kMaxUnisonDetuneCents;
        int numSpreadVoices = mOsc2UnisonVoices - 1;  // Voices 1 to N-1

        for (int v = 0; v < mOsc2UnisonVoices; v++)
        {
          if (v == 0)
          {
            // CENTER VOICE: No detune, no panning
            mOsc2UnisonDetuneOffsets[v] = 0.0f;
            mOsc2UnisonPans[v] = 0.0f;
          }
          else
          {
            // SPREAD VOICES: Symmetric detune and panning

            // DETUNE: Alternating +/- with increasing magnitude
            int spreadLevel = (v + 1) / 2;
            float detunePosition = static_cast<float>(spreadLevel) / static_cast<float>((mOsc2UnisonVoices) / 2);
            float detuneSign = (v % 2 == 1) ? 1.0f : -1.0f;
            mOsc2UnisonDetuneOffsets[v] = detuneSign * detunePosition * spreadCents;

            // PANNING: Perfectly symmetric spread (same formula as Osc1)
            float panPosition;
            float idx = static_cast<float>(v - 1);

            if (numSpreadVoices % 2 == 1)
            {
              // Odd spread voices: includes center
              panPosition = (2.0f * idx - (numSpreadVoices - 1)) / static_cast<float>(numSpreadVoices - 1);
            }
            else
            {
              // Even spread voices: straddles center
              panPosition = (2.0f * idx + 1.0f - numSpreadVoices) / static_cast<float>(numSpreadVoices);
            }
            mOsc2UnisonPans[v] = panPosition * mOsc2UnisonWidth;
          }
        }
      }

      // ─────────────────────────────────────────────────────────────────────────
      // SET UP OSC1 UNISON VOICE FREQUENCIES
      // ─────────────────────────────────────────────────────────────────────────
      for (int v = 0; v < mOsc1UnisonVoices; v++)
      {
        float osc1TotalDetune = mOsc1Detune + mOsc1UnisonDetuneOffsets[v];
        double osc1UnisonFreqShift = static_cast<double>(mOsc1Octave) + osc1TotalDetune / 1200.0;
        double osc1UnisonFreq = baseFreq * std::pow(2.0, osc1UnisonFreqShift);
        mOsc1UnisonPhases[v].set(q::frequency{static_cast<float>(osc1UnisonFreq)}, static_cast<float>(mSampleRate));
        // Also set up wavetable phases for this unison voice
        mOsc1WavetablePhaseIncs[v] = static_cast<float>(osc1UnisonFreq / mSampleRate);
        mOsc1WavetableFreqs[v] = static_cast<float>(osc1UnisonFreq);
      }

      // ─────────────────────────────────────────────────────────────────────────
      // SET UP OSC2 UNISON VOICE FREQUENCIES (Independent from Osc1)
      // ─────────────────────────────────────────────────────────────────────────
      if (mOsc2Level > 0.001f)
      {
        for (int v = 0; v < mOsc2UnisonVoices; v++)
        {
          float osc2TotalDetune = mOsc2Detune + mOsc2UnisonDetuneOffsets[v];
          double osc2UnisonFreqShift = static_cast<double>(mOsc2Octave) + osc2TotalDetune / 1200.0;
          double osc2UnisonFreq = baseFreq * std::pow(2.0, osc2UnisonFreqShift);
          mOsc2UnisonPhases[v].set(q::frequency{static_cast<float>(osc2UnisonFreq)}, static_cast<float>(mSampleRate));
          // Also set up wavetable phases for this unison voice
          mOsc2WavetablePhaseIncs[v] = static_cast<float>(osc2UnisonFreq / mSampleRate);
          mOsc2WavetableFreqs[v] = static_cast<float>(osc2UnisonFreq);
        }
      }

      // Also update the main phase iterators (used when unison=1 or as voice 0)
      mPhase.set(q::frequency{static_cast<float>(freq)}, static_cast<float>(mSampleRate));
      mOsc2Phase.set(q::frequency{static_cast<float>(osc2Freq)}, static_cast<float>(mSampleRate));

      for (int i = startIdx; i < startIdx + nFrames; i++)
      {
        // Get envelope amplitude
        float envAmp = mEnv();

        // ─────────────────────────────────────────────────────────────────────────
        // RETRIGGER CROSSFADE
        // When a voice is retriggered, the new envelope starts from zero while
        // the previous note may have been at full amplitude. This causes a click.
        // Solution: We capture the previous level in mRetriggerOffset (see Trigger())
        // and crossfade from it to the new envelope over ~5ms.
        //
        // Logic: If envelope is below the offset, use the offset. The offset
        // decays exponentially, so once the new envelope catches up, we follow it.
        // ─────────────────────────────────────────────────────────────────────────
        if (mRetriggerOffset > 0.001f)
        {
          // Hold at previous level until new envelope catches up
          if (envAmp < mRetriggerOffset)
            envAmp = mRetriggerOffset;
          // Exponential decay towards zero (~5ms time constant)
          mRetriggerOffset *= mRetriggerDecay;
        }
        else
        {
          // Below threshold - clear to avoid denormals
          mRetriggerOffset = 0.0f;
        }

        // Clamp envelope
        if (envAmp < 0.0f) envAmp = 0.0f;
        if (envAmp > 1.0f) envAmp = 1.0f;

        // Check if voice should deactivate
        if (envAmp < 0.0001f && mEnv.in_idle_phase())
        {
          mActive = false;
          return;
        }

        // ─────────────────────────────────────────────────────────────────────────
        // SIGNAL CHAIN: OSC (with UNISON) → FILTER → AMP
        // This is the classic subtractive synthesis topology, enhanced with unison.
        //
        // When unison > 1, we generate multiple detuned copies of each oscillator
        // and mix them with stereo panning for the "massive" supersaw sound.
        // ─────────────────────────────────────────────────────────────────────────

        // Smooth pulse width for Osc1 (used by all unison voices)
        mPulseWidth += mPulseWidthSmoothCoeff * (mPulseWidthTarget - mPulseWidth);

        // Smooth FM parameters for Osc1
        mFMRatioTarget = mFMRatioCoarse * (1.0f + mFMRatioFine);
        mFMRatio += mFMSmoothCoeff * (mFMRatioTarget - mFMRatio);
        mFMDepth += mFMSmoothCoeff * (mFMDepthTarget - mFMDepth);

        // ─────────────────────────────────────────────────────────────────────────
        // PER-OSCILLATOR UNISON GENERATION (Serum-style)
        // ─────────────────────────────────────────────────────────────────────────
        // Each oscillator has INDEPENDENT unison settings. We generate Osc1 and
        // Osc2 in separate loops, each with their own voice count, detune, width,
        // and blend parameters.
        // ─────────────────────────────────────────────────────────────────────────
        float osc1Left = 0.0f;
        float osc1Right = 0.0f;
        float osc2Left = 0.0f;
        float osc2Right = 0.0f;

        // ─────────────────────────────────────────────────────────────────────────
        // OSC1 UNISON GENERATION
        // ─────────────────────────────────────────────────────────────────────────
        for (int v = 0; v < mOsc1UnisonVoices; v++)
        {
          float osc1Sample = 0.0f;
          switch (mWaveform)
          {
            case kWaveformSine:
              osc1Sample = q::sin(mOsc1UnisonPhases[v]++);
              break;
            case kWaveformSaw:
              osc1Sample = q::saw(mOsc1UnisonPhases[v]++);
              break;
            case kWaveformSquare:
              osc1Sample = q::square(mOsc1UnisonPhases[v]++);
              break;
            case kWaveformTriangle:
              osc1Sample = q::triangle(mOsc1UnisonPhases[v]++);
              break;
            case kWaveformPulse:
              mOsc1UnisonPulseOscs[v].width(mPulseWidth);
              osc1Sample = mOsc1UnisonPulseOscs[v](mOsc1UnisonPhases[v]++);
              break;
            case kWaveformFM:
            {
              // ─────────────────────────────────────────────────────────────────
              // FM SYNTHESIS WITH PER-UNISON-VOICE MODULATOR
              // Each unison voice has its own modulator phase, ensuring correct
              // FM behavior when combined with unison detuning.
              // ─────────────────────────────────────────────────────────────────
              float phaseIncRadians = static_cast<float>(mOsc1UnisonPhases[v]._step.rep) /
                                      static_cast<float>(0xFFFFFFFFu) * kTwoPi;

              // Use per-voice modulator phase (not shared across voices)
              mOsc1FMModulatorPhases[v] += phaseIncRadians * mFMRatio;
              while (mOsc1FMModulatorPhases[v] >= kTwoPi)
                mOsc1FMModulatorPhases[v] -= kTwoPi;

              float modulatorValue = std::sin(mOsc1FMModulatorPhases[v]);
              constexpr float kMaxModIndex = 4.0f * kPi;  // ~12.57 radians max modulation
              float velScaledDepth = mFMDepth * (0.3f + 0.7f * mVelocity);
              float phaseModulation = velScaledDepth * kMaxModIndex * modulatorValue;

              float carrierPhase = static_cast<float>(mOsc1UnisonPhases[v]._phase.rep) /
                                   static_cast<float>(0xFFFFFFFFu) * kTwoPi;
              osc1Sample = std::sin(carrierPhase + phaseModulation);
              mOsc1UnisonPhases[v]++;
              break;
            }
            case kWaveformWavetable:
              if (v == 0)
                osc1Sample = mWavetableOsc.Process();
              else
                osc1Sample = mWavetableOsc.ProcessAtPhase(mOsc1WavetablePhases[v], mOsc1WavetablePhaseIncs[v], mOsc1WavetableFreqs[v]);
              break;
            default:
              osc1Sample = q::sin(mOsc1UnisonPhases[v]++);
              break;
          }

          // ─────────────────────────────────────────────────────────────────
          // OSC1 STEREO PANNING
          // ─────────────────────────────────────────────────────────────────
          // For single voice (unison=1): Full amplitude to both channels
          // For unison: Constant-power panning using sin/cos to maintain
          // equal perceived loudness across the stereo field.
          //
          // Constant-power formula: L=cos(angle), R=sin(angle)
          // This gives: L²+R² = 1 at all pan positions
          // ─────────────────────────────────────────────────────────────────
          float leftGain, rightGain;
          if (mOsc1UnisonVoices == 1)
          {
            // Single voice: full amplitude to both channels (mono compatible)
            leftGain = 1.0f;
            rightGain = 1.0f;
          }
          else if (v == 0)
          {
            // Center voice in unison: equal power to both channels (1/sqrt(2))
            leftGain = kSqrtHalf;
            rightGain = kSqrtHalf;
          }
          else
          {
            // Spread voices: constant-power panning
            float pan = mOsc1UnisonPans[v];
            float angle = (pan + 1.0f) * kQuarterPi;
            leftGain = std::cos(angle);
            rightGain = std::sin(angle);
          }

          // ─────────────────────────────────────────────────────────────────
          // VOICE WEIGHT WITH POWER COMPENSATION
          // ─────────────────────────────────────────────────────────────────
          // When splitting signal across N voices, amplitude weights that sum
          // to 1.0 cause power (loudness) to DROP because power = amplitude².
          //
          // Example: 8 voices with blend=0.75
          //   Raw weights: 0.25² + 7×(0.107)² = 0.14 (only 14% power!)
          //
          // We compensate by calculating the power sum and scaling up:
          //   compensation = 1 / sqrt(powerSum)
          //
          // This maintains consistent perceived loudness regardless of
          // unison voice count or blend setting.
          // ─────────────────────────────────────────────────────────────────
          float voiceWeight = 1.0f;
          if (mOsc1UnisonVoices > 1)
          {
            float centerWeight = 1.0f - mOsc1UnisonBlend;
            float spreadWeight = mOsc1UnisonBlend / static_cast<float>(mOsc1UnisonVoices - 1);

            // Calculate power sum for compensation
            float powerSum = centerWeight * centerWeight +
                             (mOsc1UnisonVoices - 1) * spreadWeight * spreadWeight;
            float gainCompensation = 1.0f / std::sqrt(powerSum);

            if (v == 0)
              voiceWeight = centerWeight * gainCompensation;
            else
              voiceWeight = spreadWeight * gainCompensation;
          }

          osc1Left += osc1Sample * leftGain * voiceWeight;
          osc1Right += osc1Sample * rightGain * voiceWeight;
        }

        // ─────────────────────────────────────────────────────────────────────────
        // HARD SYNC DETECTION (for Osc2)
        // ─────────────────────────────────────────────────────────────────────────
        // Hard sync is a classic analog technique where Osc2's phase resets whenever
        // Osc1 completes a cycle. This creates aggressive, harmonically rich timbres
        // that change character as you detune Osc2 relative to Osc1.
        //
        // IMPLEMENTATION CHALLENGE:
        // We need to detect Osc1's zero-crossing regardless of waveform type:
        // - For standard waveforms: Use Q library phase_iterator (32-bit fixed point)
        // - For wavetable: Use our float phase (0.0-1.0)
        //
        // We track BOTH phase types and check which one is active based on waveform.
        // ─────────────────────────────────────────────────────────────────────────
        bool osc1CycleComplete = false;
        if (mOscSyncMode == 1)
        {
          bool cycleDetected = false;

          if (mWaveform == kWaveformWavetable)
          {
            // ─────────────────────────────────────────────────────────────────────
            // WAVETABLE SYNC DETECTION
            // The main wavetable oscillator tracks its own phase internally.
            // We use voice 0's wavetable phase for sync detection since that's
            // the phase being incremented by mWavetableOsc.Process().
            // ─────────────────────────────────────────────────────────────────────
            float currentWtPhase = mWavetableOsc.GetPhase();
            if (currentWtPhase < mPrevOsc1WavetablePhase)
            {
              cycleDetected = true;
            }
            mPrevOsc1WavetablePhase = currentWtPhase;
          }
          else
          {
            // ─────────────────────────────────────────────────────────────────────
            // STANDARD WAVEFORM SYNC DETECTION
            // Q library phase_iterator uses 32-bit fixed point. When it wraps
            // from 0xFFFFFFFF to 0x00000000, we detect it as current < previous.
            // ─────────────────────────────────────────────────────────────────────
            uint32_t currentPhase = mOsc1UnisonPhases[0]._phase.rep;
            if (currentPhase < mPrevOsc1PhaseRaw)
            {
              cycleDetected = true;
            }
            mPrevOsc1PhaseRaw = currentPhase;
          }

          if (cycleDetected)
          {
            osc1CycleComplete = true;

            // ─────────────────────────────────────────────────────────────────────
            // RESET ALL OSC2 PHASES
            // Reset both Q library phases AND wavetable phases to ensure sync
            // works regardless of Osc2's waveform type.
            // ─────────────────────────────────────────────────────────────────────
            for (int sv = 0; sv < mOsc2UnisonVoices; sv++)
            {
              // Reset Q library phase iterators (for standard waveforms)
              mOsc2UnisonPhases[sv]._phase.rep = 0;

              // Reset wavetable phases (for wavetable mode)
              mOsc2WavetablePhases[sv] = 0.0f;
            }

            // Also reset the main Osc2 wavetable oscillator's internal phase
            mOsc2WavetableOsc.ResetPhase();
          }
        }

        // ─────────────────────────────────────────────────────────────────────────
        // OSC2 UNISON GENERATION (Independent from Osc1)
        // ─────────────────────────────────────────────────────────────────────────
        if (mOsc2Level > 0.001f)
        {
          for (int v = 0; v < mOsc2UnisonVoices; v++)
          {
            float osc2Sample = 0.0f;
            switch (mOsc2Waveform)
            {
              case kWaveformSine:
                osc2Sample = q::sin(mOsc2UnisonPhases[v]++);
                break;
              case kWaveformSaw:
                osc2Sample = q::saw(mOsc2UnisonPhases[v]++);
                break;
              case kWaveformSquare:
                osc2Sample = q::square(mOsc2UnisonPhases[v]++);
                break;
              case kWaveformTriangle:
                osc2Sample = q::triangle(mOsc2UnisonPhases[v]++);
                break;
              case kWaveformPulse:
                mOsc2UnisonPulseOscs[v].width(mOsc2PulseWidth);
                osc2Sample = mOsc2UnisonPulseOscs[v](mOsc2UnisonPhases[v]++);
                break;
              case kWaveformFM:
              {
                // ─────────────────────────────────────────────────────────────────
                // OSC2 FM SYNTHESIS WITH PER-UNISON-VOICE MODULATOR
                // Same as Osc1: each unison voice gets its own modulator phase.
                // ─────────────────────────────────────────────────────────────────
                float phaseIncRadians = static_cast<float>(mOsc2UnisonPhases[v]._step.rep) /
                                        static_cast<float>(0xFFFFFFFFu) * kTwoPi;
                float osc2CombinedRatio = mOsc2FMRatio + mOsc2FMFine;

                // Use per-voice modulator phase (not shared across voices)
                mOsc2FMModulatorPhases[v] += phaseIncRadians * osc2CombinedRatio;
                while (mOsc2FMModulatorPhases[v] >= kTwoPi)
                  mOsc2FMModulatorPhases[v] -= kTwoPi;

                float modulatorValue = std::sin(mOsc2FMModulatorPhases[v]);
                constexpr float kMaxModIndex = 4.0f * kPi;  // ~12.57 radians max modulation
                float velScaledDepth = mOsc2FMDepth * (0.3f + 0.7f * mVelocity);
                float phaseModulation = velScaledDepth * kMaxModIndex * modulatorValue;

                float carrierPhase = static_cast<float>(mOsc2UnisonPhases[v]._phase.rep) /
                                     static_cast<float>(0xFFFFFFFFu) * kTwoPi;
                osc2Sample = std::sin(carrierPhase + phaseModulation);
                mOsc2UnisonPhases[v]++;
                break;
              }
              case kWaveformWavetable:
                if (v == 0)
                {
                  mOsc2WavetableOsc.SetPosition(mOsc2MorphPosition);
                  osc2Sample = mOsc2WavetableOsc.Process();
                }
                else
                  osc2Sample = mOsc2WavetableOsc.ProcessAtPhase(mOsc2WavetablePhases[v], mOsc2WavetablePhaseIncs[v], mOsc2WavetableFreqs[v]);
                break;
              default:
                osc2Sample = q::sin(mOsc2UnisonPhases[v]++);
                break;
            }

            // ─────────────────────────────────────────────────────────────────
            // OSC2 STEREO PANNING (same algorithm as Osc1)
            // ─────────────────────────────────────────────────────────────────
            float leftGain, rightGain;
            if (mOsc2UnisonVoices == 1)
            {
              // Single voice: full amplitude to both channels
              leftGain = 1.0f;
              rightGain = 1.0f;
            }
            else if (v == 0)
            {
              // Center voice in unison: equal power (1/sqrt(2))
              leftGain = kSqrtHalf;
              rightGain = kSqrtHalf;
            }
            else
            {
              // Spread voices: constant-power panning
              float pan = mOsc2UnisonPans[v];
              float angle = (pan + 1.0f) * kQuarterPi;
              leftGain = std::cos(angle);
              rightGain = std::sin(angle);
            }

            // VOICE WEIGHT WITH POWER COMPENSATION (same as Osc1)
            float voiceWeight = 1.0f;
            if (mOsc2UnisonVoices > 1)
            {
              float centerWeight = 1.0f - mOsc2UnisonBlend;
              float spreadWeight = mOsc2UnisonBlend / static_cast<float>(mOsc2UnisonVoices - 1);

              float powerSum = centerWeight * centerWeight +
                               (mOsc2UnisonVoices - 1) * spreadWeight * spreadWeight;
              float gainCompensation = 1.0f / std::sqrt(powerSum);

              if (v == 0)
                voiceWeight = centerWeight * gainCompensation;
              else
                voiceWeight = spreadWeight * gainCompensation;
            }

            osc2Left += osc2Sample * leftGain * voiceWeight;
            osc2Right += osc2Sample * rightGain * voiceWeight;
          }
        }

        // ─────────────────────────────────────────────────────────────────────────
        // VOLUME NORMALIZATION
        // ─────────────────────────────────────────────────────────────────────────
        // With the corrected blend weights, total voice weights sum to ~1.0:
        //   center_weight + sum(detuned_weights) = (1-blend) + blend = 1.0
        //
        // However, panned voices contribute less per-channel (due to stereo spread).
        // We compensate slightly to maintain perceived loudness across unison counts.
        //
        // sqrt(N) normalization is removed because:
        //   1. Blend weights already sum to 1.0
        //   2. Center voice at full amplitude maintains baseline loudness
        //   3. Adding detuned voices should make sound "bigger", not same volume
        // ─────────────────────────────────────────────────────────────────────────
        // No normalization needed - blend weights handle energy distribution

        // ─────────────────────────────────────────────────────────────────────────
        // MIX OSCILLATORS (stereo)
        // ─────────────────────────────────────────────────────────────────────────
        float mixedLeft = mOsc1Level * osc1Left + mOsc2Level * osc2Left;
        float mixedRight = mOsc1Level * osc1Right + mOsc2Level * osc2Right;

        // ─────────────────────────────────────────────────────────────────────────
        // FILTER (stereo - separate filters for L/R to preserve stereo image)
        // ─────────────────────────────────────────────────────────────────────────
        float filteredLeft = mFilterL.Process(mixedLeft);
        float filteredRight = mFilterR.Process(mixedRight);

        // ─────────────────────────────────────────────────────────────────────────
        // DC BLOCKER (high-pass at ~10Hz, sample-rate independent)
        // ─────────────────────────────────────────────────────────────────────────
        // FM synthesis, wavetable morphing, and filter resonance can all generate
        // DC offset. This simple one-pole high-pass filter removes it:
        //   y[n] = x[n] - x[n-1] + coeff * y[n-1]
        //
        // Coefficient is calculated in SetSampleRateAndBlockSize using:
        //   α = e^(-2πfc/fs) for -3dB at fc Hz
        // At any sample rate, -3dB at ~10Hz, transparent above 30Hz.
        // This keeps woofers happy and prevents clipping from DC accumulation.
        // ─────────────────────────────────────────────────────────────────────────
        float dcFreeLeft = filteredLeft - mDCBlockerPrevInL + mDCBlockerCoeff * mDCBlockerPrevOutL;
        float dcFreeRight = filteredRight - mDCBlockerPrevInR + mDCBlockerCoeff * mDCBlockerPrevOutR;
        mDCBlockerPrevInL = filteredLeft;
        mDCBlockerPrevInR = filteredRight;
        mDCBlockerPrevOutL = dcFreeLeft;
        mDCBlockerPrevOutR = dcFreeRight;

        // STEP 3: Apply envelope and velocity (shape the amplitude)
        float sampleLeft = dcFreeLeft * envAmp * mVelocity;
        float sampleRight = dcFreeRight * envAmp * mVelocity;

        // Accumulate to outputs (don't overwrite - other voices add here too)
        outputs[0][i] += sampleLeft;
        if (nOutputs > 1)
          outputs[1][i] += sampleRight;
      }
    }

    void SetSampleRateAndBlockSize(double sampleRate, int blockSize) override
    {
      mSampleRate = sampleRate;
      mEnv = q::adsr_envelope_gen{mEnvConfig, static_cast<float>(sampleRate)};
      mWavetableOsc.SetSampleRate(static_cast<float>(sampleRate));
      mOsc2WavetableOsc.SetSampleRate(static_cast<float>(sampleRate));  // Osc2 wavetable
      mFilterL.SetSampleRate(static_cast<float>(sampleRate));
      mFilterR.SetSampleRate(static_cast<float>(sampleRate));

      // Pulse width smoothing: ~5ms for responsive but click-free modulation
      mPulseWidthSmoothCoeff = calcSmoothingCoeff(0.005f, static_cast<float>(sampleRate));

      // FM parameter smoothing: ~5ms for smooth ratio/depth changes
      mFMSmoothCoeff = calcSmoothingCoeff(0.005f, static_cast<float>(sampleRate));

      // DC blocker coefficient: high-pass at ~10Hz (sample-rate independent)
      // Formula: α = e^(-2πfc/fs) gives -3dB at fc Hz
      // 10Hz is below audible range but above subsonic rumble from speakers
      constexpr float kDCBlockerCutoffHz = 10.0f;
      mDCBlockerCoeff = std::exp(-kTwoPi * kDCBlockerCutoffHz / static_cast<float>(sampleRate));
    }

    // Parameter setters called from DSP class
    void SetWaveform(int waveform) { mWaveform = waveform; }
    void SetWavetablePosition(float pos) { mWavetableOsc.SetPosition(pos); }

    void SetAttack(float ms)
    {
      mBaseAttackMs = ms;
      mEnvConfig.attack_rate = q::duration{ms * 0.001f};
    }

    void SetDecay(float ms)
    {
      mBaseDecayMs = ms;
      mEnvConfig.decay_rate = q::duration{ms * 0.001f};
    }

    void SetSustain(float level)
    {
      // Clamp to tiny positive value to avoid -infinity dB from lin_to_db(0)
      // At 0.001 (-60dB), the sound is effectively silent but math remains stable
      level = std::max(0.001f, level);
      mEnvConfig.sustain_level = q::lin_to_db(level);
    }

    void SetRelease(float ms)
    {
      mBaseReleaseMs = ms;
      mEnvConfig.release_rate = q::duration{ms * 0.001f};
    }

    // Velocity → envelope time modulation (0.0-1.0)
    // Higher values = velocity has more effect on envelope times
    void SetEnvVelocitySensitivity(float amount) { mEnvVelocitySensitivity = amount; }

    // Filter setters (stereo - both L/R filters)
    void SetFilterCutoff(float freqHz) { mFilterL.SetCutoff(freqHz); mFilterR.SetCutoff(freqHz); }
    void SetFilterResonance(float resonance) { mFilterL.SetResonance(resonance); mFilterR.SetResonance(resonance); }
    void SetFilterType(int type) { mFilterL.SetType(static_cast<FilterType>(type)); mFilterR.SetType(static_cast<FilterType>(type)); }

    // Pulse width setter (0.0-1.0, where 0.5 = square wave)
    // Sets target; actual value is smoothed per-sample to prevent clicks
    void SetPulseWidth(float width) { mPulseWidthTarget = width; }

    // FM synthesis setters (DX7-style coarse + fine)
    // Coarse ratio: harmonic values (0.5, 1, 2, 3, etc.)
    void SetFMRatioCoarse(float ratio) { mFMRatioCoarse = ratio; }
    // Fine ratio: percentage offset (-0.5 to +0.5) for inharmonic sounds
    void SetFMRatioFine(float fine) { mFMRatioFine = fine; }
    // Depth: modulation intensity (0.0-1.0)
    void SetFMDepth(float depth) { mFMDepthTarget = depth; }
    // Osc1 level for balancing with Osc2
    void SetOsc1Level(float level) { mOsc1Level = level; }            // 0.0-1.0
    // Osc1 octave and detune (for full symmetry with Osc2)
    void SetOsc1Octave(int octave) { mOsc1Octave = octave; }          // -2 to +2
    void SetOsc1Detune(float cents) { mOsc1Detune = cents; }          // -100 to +100 cents

    // ─────────────────────────────────────────────────────────────────────────
    // OSCILLATOR 2 SETTERS (fully independent like Serum)
    // ─────────────────────────────────────────────────────────────────────────
    void SetOsc2Waveform(int waveform) { mOsc2Waveform = waveform; }
    void SetOsc2Octave(int octave) { mOsc2Octave = octave; }       // -2 to +2
    void SetOsc2Detune(float cents) { mOsc2Detune = cents; }       // -100 to +100 cents
    void SetOsc2Level(float level) { mOsc2Level = level; }         // 0.0-1.0
    // Osc2 independent waveform-specific parameters
    void SetOsc2Morph(float position) { mOsc2MorphPosition = position; }  // 0.0-1.0
    void SetOsc2PulseWidth(float width) { mOsc2PulseWidth = width; }      // 0.05-0.95
    void SetOsc2FMRatio(float ratio) { mOsc2FMRatio = ratio; }            // Combined ratio
    void SetOsc2FMFine(float fine) { mOsc2FMFine = fine; }                // -0.5 to +0.5
    void SetOsc2FMDepth(float depth) { mOsc2FMDepth = depth; }            // 0.0-1.0

    // ─────────────────────────────────────────────────────────────────────────
    // OSC1 UNISON SETTERS
    // Each oscillator has independent unison for Serum-style sound design.
    // ─────────────────────────────────────────────────────────────────────────
    void SetOsc1UnisonVoices(int voices)
    {
      mOsc1UnisonVoices = std::max(1, std::min(kMaxUnisonVoices, voices));
    }
    void SetOsc1UnisonDetune(float detune) { mOsc1UnisonDetune = detune; }  // 0.0-1.0
    void SetOsc1UnisonWidth(float width) { mOsc1UnisonWidth = width; }      // 0.0-1.0
    void SetOsc1UnisonBlend(float blend) { mOsc1UnisonBlend = blend; }      // 0.0-1.0

    // ─────────────────────────────────────────────────────────────────────────
    // OSC2 UNISON SETTERS (Independent from Osc1)
    // ─────────────────────────────────────────────────────────────────────────
    void SetOsc2UnisonVoices(int voices)
    {
      mOsc2UnisonVoices = std::max(1, std::min(kMaxUnisonVoices, voices));
    }
    void SetOsc2UnisonDetune(float detune) { mOsc2UnisonDetune = detune; }  // 0.0-1.0
    void SetOsc2UnisonWidth(float width) { mOsc2UnisonWidth = width; }      // 0.0-1.0
    void SetOsc2UnisonBlend(float blend) { mOsc2UnisonBlend = blend; }      // 0.0-1.0

    // ─────────────────────────────────────────────────────────────────────────
    // OSCILLATOR SYNC SETTER
    // See the OSCILLATOR SYNC documentation at the top of this file for details.
    // ─────────────────────────────────────────────────────────────────────────
    void SetOscSync(int mode) { mOscSyncMode = mode; }                    // 0=Off, 1=Hard

    // ─────────────────────────────────────────────────────────────────────────
    // PORTAMENTO (GLIDE) SETTER
    // ─────────────────────────────────────────────────────────────────────────
    // Sets the portamento time. The coefficient is calculated per-sample:
    //   coeff = 1 - e^(-1 / (time × sampleRate))
    // At coeff=1.0, portamento is disabled (instant pitch change).
    // ─────────────────────────────────────────────────────────────────────────
    void SetPortamentoTime(float timeMs)
    {
      if (timeMs < 1.0f)
      {
        // Portamento off - instant pitch change
        mPortamentoCoeff = 1.0f;
      }
      else
      {
        // Calculate per-sample glide coefficient
        // Using calcSmoothingCoeff for consistent time constant behavior
        mPortamentoCoeff = calcSmoothingCoeff(timeMs * 0.001f, static_cast<float>(mSampleRate));
      }
    }

  private:
    // ─────────────────────────────────────────────────────────────────────────
    // PHASE ITERATOR - Q library's oscillator driver
    // Fixed-point 1.31 format: 32-bit unsigned int where full cycle = 2^32
    // - _phase: Current position in cycle (0 to 2^32-1)
    // - _step: Phase increment per sample = (2^32 × freq) / sampleRate
    // Natural wraparound via integer overflow eliminates modulo operations.
    // Usage: q::sin(mPhase++) returns sine value and advances phase.
    // ─────────────────────────────────────────────────────────────────────────
    q::phase_iterator mPhase;

    // Pulse oscillator - PolyBLEP with variable duty cycle
    // Width: 0.0-1.0 where 0.5 = square wave, <0.5 = narrow pulse, >0.5 = wide pulse
    q::pulse_osc mPulseOsc{0.5f};

    // ─────────────────────────────────────────────────────────────────────────
    // PULSE WIDTH SMOOTHING
    // When pulse width changes suddenly, the falling edge position jumps,
    // causing a discontinuity (click). We smooth the pulse width over ~5ms
    // to prevent this. Faster than gain smoothing for responsive modulation.
    // ─────────────────────────────────────────────────────────────────────────
    float mPulseWidth = 0.5f;           // Current smoothed pulse width
    float mPulseWidthTarget = 0.5f;     // Target from parameter
    float mPulseWidthSmoothCoeff = 0.01f;  // ~5ms smoothing, set in SetSampleRateAndBlockSize

    // ─────────────────────────────────────────────────────────────────────────
    // FM SYNTHESIS PARAMETERS (DX7-style)
    // The modulator is a hidden sine wave that modulates the carrier's phase.
    // Coarse ratio sets harmonic relationship, Fine allows inharmonic detuning.
    // NOTE: Modulator phases are now per-unison-voice (see mOsc1FMModulatorPhases[])
    // ─────────────────────────────────────────────────────────────────────────
    float mFMRatioCoarse = 2.0f;          // Coarse ratio (0.5, 1, 2, 3, etc.)
    float mFMRatioFine = 0.0f;            // Fine offset (-0.5 to +0.5)
    float mFMRatio = 2.0f;                // Current smoothed combined ratio
    float mFMRatioTarget = 2.0f;          // Target combined ratio
    float mFMDepth = 0.5f;                // Current smoothed depth (0-1)
    float mFMDepthTarget = 0.5f;          // Target depth from parameter
    float mFMSmoothCoeff = 0.01f;         // ~5ms smoothing for FM params

    // Osc1 level for balancing with Osc2
    float mOsc1Level = 1.0f;              // Mix level (1.0 = full, default)
    // Osc1 octave and detune (for full symmetry with Osc2)
    int mOsc1Octave = 0;                  // Octave offset: -2 to +2 (0 = no shift)
    float mOsc1Detune = 0.0f;             // Detune in cents (0 = no detune)

    // ADSR envelope generator - controls amplitude over note lifetime
    q::adsr_envelope_gen mEnv{q::adsr_envelope_gen::config{}, 44100.0f};
    q::adsr_envelope_gen::config mEnvConfig;

    // ─────────────────────────────────────────────────────────────────────────
    // VELOCITY → ENVELOPE MODULATION
    // Base times are stored separately so we can apply velocity scaling in Trigger().
    // Higher velocity = faster attack/decay/release for more expressive playing.
    // ─────────────────────────────────────────────────────────────────────────
    float mBaseAttackMs = 10.0f;          // Base attack time from parameter
    float mBaseDecayMs = 100.0f;          // Base decay time from parameter
    float mBaseReleaseMs = 200.0f;        // Base release time from parameter
    float mEnvVelocitySensitivity = 0.5f; // How much velocity affects times (0-1)

    WavetableOscillator mWavetableOsc;  // Wavetable oscillator (mipmapped, morphable)
    ResonantFilter mFilterL;             // Left channel filter (stereo processing)
    ResonantFilter mFilterR;             // Right channel filter (stereo processing)

    // ─────────────────────────────────────────────────────────────────────────
    // DC BLOCKER - Removes DC offset from FM/wavetable/filter
    // ─────────────────────────────────────────────────────────────────────────
    // Simple one-pole high-pass: y[n] = x[n] - x[n-1] + coeff * y[n-1]
    // Coefficient is calculated in SetSampleRateAndBlockSize for ~10Hz cutoff.
    // ─────────────────────────────────────────────────────────────────────────
    float mDCBlockerPrevInL = 0.0f;       // Previous input sample (left)
    float mDCBlockerPrevInR = 0.0f;       // Previous input sample (right)
    float mDCBlockerPrevOutL = 0.0f;      // Previous output sample (left)
    float mDCBlockerPrevOutR = 0.0f;      // Previous output sample (right)
    float mDCBlockerCoeff = 0.9987f;      // Default for 48kHz, recalculated per sample rate
    double mSampleRate = 44100.0;
    float mVelocity = 0.0f;              // MIDI velocity (0-1)
    bool mActive = false;                // Voice is currently sounding
    int mWaveform = kWaveformSine;       // Current waveform selection

    // ─────────────────────────────────────────────────────────────────────────
    // RETRIGGER SMOOTHING
    // When a voice is retriggered while still sounding, we crossfade from
    // the previous amplitude to avoid clicks. mRetriggerOffset holds the
    // level to fade from, mRetriggerDecay controls the ~5ms fade speed.
    // ─────────────────────────────────────────────────────────────────────────
    float mRetriggerOffset = 0.0f;
    float mRetriggerDecay = 1.0f;

    // ─────────────────────────────────────────────────────────────────────────
    // OSCILLATOR 2 - Secondary oscillator for fat, layered sounds
    // FREE-RUNNING: Phase is NOT reset on note trigger for natural beating.
    // This creates the characteristic "fatness" of dual-oscillator synths.
    // ─────────────────────────────────────────────────────────────────────────
    q::phase_iterator mOsc2Phase;           // Free-running phase (not reset on trigger)
    q::pulse_osc mOsc2PulseOsc{0.5f};       // Osc2's pulse oscillator
    WavetableOscillator mOsc2WavetableOsc;  // Osc2's wavetable oscillator
    // NOTE: Osc2 FM modulator phases are now per-unison-voice (see mOsc2FMModulatorPhases[])

    int mOsc2Waveform = kWaveformSaw;       // Default: Saw (classic dual-osc combo)
    int mOsc2Octave = 0;                    // Octave offset: -2 to +2 (0 = unison)
    float mOsc2Detune = 7.0f;               // Detune in cents (+7 = classic fat)
    float mOsc2Level = 0.5f;                // Mix level (0.5 = equal with Osc1)

    // OSC2 INDEPENDENT PARAMETERS (Serum-style full independence)
    // Each oscillator has its own waveform-specific controls for maximum flexibility.
    // ─────────────────────────────────────────────────────────────────────────
    float mOsc2MorphPosition = 0.0f;        // Wavetable morph position (0.0-1.0)
    float mOsc2PulseWidth = 0.5f;           // Pulse width (0.05-0.95), 0.5 = square
    float mOsc2FMRatio = 2.0f;              // FM frequency ratio (carrier/modulator)
    float mOsc2FMFine = 0.0f;               // FM fine ratio offset (-0.5 to +0.5)
    float mOsc2FMDepth = 0.5f;              // FM modulation depth (0.0-1.0)

    // ─────────────────────────────────────────────────────────────────────────
    // PER-OSCILLATOR UNISON ENGINE (Serum-style)
    // ─────────────────────────────────────────────────────────────────────────
    // Each oscillator has its own independent unison with separate:
    //   - Voice count (1-8)
    //   - Detune spread
    //   - Stereo width
    //   - Blend (center vs detuned mix)
    //
    // This allows powerful combinations like:
    //   - Thick supersaw lead (Osc1: 8 voices) + clean sub bass (Osc2: 1 voice)
    //   - Both oscillators with different widths for depth
    //   - One narrow for focus, one wide for atmosphere
    //
    // PHASE ACCUMULATORS:
    // - mOsc1UnisonPhases[]: Phase iterators for Osc1 unison voices
    // - mOsc2UnisonPhases[]: Phase iterators for Osc2 unison voices
    // - Both are FREE-RUNNING (not reset on note trigger) for natural beating
    // ─────────────────────────────────────────────────────────────────────────
    std::array<q::phase_iterator, kMaxUnisonVoices> mOsc1UnisonPhases;     // Osc1 unison phases
    std::array<q::phase_iterator, kMaxUnisonVoices> mOsc2UnisonPhases;     // Osc2 unison phases
    std::array<q::pulse_osc, kMaxUnisonVoices> mOsc1UnisonPulseOscs;       // Osc1 pulse oscillators
    std::array<q::pulse_osc, kMaxUnisonVoices> mOsc2UnisonPulseOscs;       // Osc2 pulse oscillators

    // ─────────────────────────────────────────────────────────────────────────
    // PER-UNISON-VOICE FM MODULATOR PHASES
    // ─────────────────────────────────────────────────────────────────────────
    // Each unison voice needs its own FM modulator phase. Without this, all
    // unison voices would share a single modulator that gets advanced multiple
    // times per sample (once per voice), causing incorrect FM behavior.
    //
    // With independent phases, each unison voice generates its own FM timbre
    // that's correctly detuned, creating the expected thick FM+unison sound.
    // ─────────────────────────────────────────────────────────────────────────
    std::array<float, kMaxUnisonVoices> mOsc1FMModulatorPhases{};         // Osc1 FM modulator phases (radians)
    std::array<float, kMaxUnisonVoices> mOsc2FMModulatorPhases{};         // Osc2 FM modulator phases (radians)

    // Osc1 Unison parameters
    int mOsc1UnisonVoices = 1;              // Number of unison voices (1 = off, 2-8 = enabled)
    float mOsc1UnisonDetune = 0.25f;        // Detune spread (0.0-1.0), mapped to cents
    float mOsc1UnisonWidth = 0.8f;          // Stereo width (0.0-1.0)
    float mOsc1UnisonBlend = 0.75f;         // Center vs detuned mix (0.0-1.0)

    // Osc2 Unison parameters (independent from Osc1)
    int mOsc2UnisonVoices = 1;              // Number of unison voices (1 = off, 2-8 = enabled)
    float mOsc2UnisonDetune = 0.25f;        // Detune spread (0.0-1.0), mapped to cents
    float mOsc2UnisonWidth = 0.8f;          // Stereo width (0.0-1.0)
    float mOsc2UnisonBlend = 0.75f;         // Center vs detuned mix (0.0-1.0)

    // Pre-computed unison voice data for Osc1 (updated when parameters change)
    std::array<float, kMaxUnisonVoices> mOsc1UnisonDetuneOffsets{};  // Cents offset per voice
    std::array<float, kMaxUnisonVoices> mOsc1UnisonPans{};           // Pan position (-1=L, 0=C, +1=R)

    // Pre-computed unison voice data for Osc2 (independent from Osc1)
    std::array<float, kMaxUnisonVoices> mOsc2UnisonDetuneOffsets{};  // Cents offset per voice
    std::array<float, kMaxUnisonVoices> mOsc2UnisonPans{};           // Pan position (-1=L, 0=C, +1=R)

    // ─────────────────────────────────────────────────────────────────────────
    // WAVETABLE UNISON PHASES
    // ─────────────────────────────────────────────────────────────────────────
    // Wavetable oscillators use their own phase format (0.0-1.0 float) rather
    // than the Q library's phase_iterator. Each unison voice needs its own
    // phase and phase increment to properly track detuned wavetable positions.
    // ─────────────────────────────────────────────────────────────────────────
    std::array<float, kMaxUnisonVoices> mOsc1WavetablePhases{};      // Osc1 wavetable phases (0.0-1.0)
    std::array<float, kMaxUnisonVoices> mOsc1WavetablePhaseIncs{};   // Osc1 phase increment per sample
    std::array<float, kMaxUnisonVoices> mOsc1WavetableFreqs{};       // Osc1 frequency for mip calculation
    std::array<float, kMaxUnisonVoices> mOsc2WavetablePhases{};      // Osc2 wavetable phases (0.0-1.0)
    std::array<float, kMaxUnisonVoices> mOsc2WavetablePhaseIncs{};   // Osc2 phase increment per sample
    std::array<float, kMaxUnisonVoices> mOsc2WavetableFreqs{};       // Osc2 frequency for mip calculation

    // ─────────────────────────────────────────────────────────────────────────
    // OSCILLATOR SYNC
    // ─────────────────────────────────────────────────────────────────────────
    // Hard sync resets Osc2's phase when Osc1 completes a cycle. We detect
    // zero-crossings by checking if current phase < previous phase (wrap-around).
    //
    // DUAL TRACKING: We track BOTH phase types because Osc1 could be:
    // - Standard waveform: Uses Q library phase_iterator (32-bit fixed point)
    // - Wavetable: Uses float phase (0.0-1.0)
    //
    // The sync detection code checks which type Osc1 is using and reads the
    // appropriate previous phase value.
    // ─────────────────────────────────────────────────────────────────────────
    int mOscSyncMode = 0;                   // 0 = Off, 1 = Hard Sync
    uint32_t mPrevOsc1PhaseRaw = 0;         // Previous Osc1 phase (Q library, 32-bit fixed)
    float mPrevOsc1WavetablePhase = 0.0f;   // Previous Osc1 phase (wavetable, 0.0-1.0)

    // ─────────────────────────────────────────────────────────────────────────
    // PORTAMENTO (GLIDE)
    // ─────────────────────────────────────────────────────────────────────────
    // Portamento smoothly glides between pitches instead of jumping instantly.
    // Classic synth feature for expressive leads and basses.
    //
    // mPortamentoCoeff: Per-sample smoothing coefficient (0-1)
    //   1.0 = instant (no portamento)
    //   0.001 = slow glide (~500ms)
    //
    // mCurrentBaseFreq: The current (gliding) base frequency
    //   Updated each sample towards the target frequency
    // ─────────────────────────────────────────────────────────────────────────
    float mPortamentoCoeff = 1.0f;          // 1.0 = off (instant), lower = slower glide
    double mCurrentBaseFreq = 0.0;          // Current gliding frequency (Hz)
  };

public:
#pragma mark - DSP
  PluginInstanceDSP(int nVoices = 8)
  {
    // Get pointer to static wavetable (no copy!)
    mWavetable = &WavetableGenerator::GenerateBasicShapes();

    for (int i = 0; i < nVoices; i++)
    {
      Voice* voice = new Voice();
      voice->SetWavetable(mWavetable);  // Share wavetable data
      mSynth.AddVoice(voice, 0);
    }
  }

  void ProcessBlock(T** inputs, T** outputs, int nOutputs, int nFrames)
  {
    // Clear outputs first
    for (int c = 0; c < nOutputs; c++)
    {
      memset(outputs[c], 0, nFrames * sizeof(T));
    }

    // MidiSynth processes MIDI queue and calls voice ProcessSamplesAccumulating
    mSynth.ProcessBlock(inputs, outputs, 0, nOutputs, nFrames);

    // ─────────────────────────────────────────────────────────────────────────
    // MASTER GAIN WITH SMOOTHING
    // We use a one-pole lowpass filter to smooth gain changes, preventing
    // clicks when the user moves the gain knob. The smoothing coefficient
    // is calculated in Reset() to be sample-rate independent (~20ms time).
    // ─────────────────────────────────────────────────────────────────────────
    for (int s = 0; s < nFrames; s++)
    {
      mGainSmoothed += mGainSmoothCoeff * (mGain - mGainSmoothed);

      // Polyphony headroom compensation: reduces amplitude to manage clipping
      // Worst-case (8 voices × full velocity × full envelope): 8.0 amplitude
      // With kPolyScale=0.35: 8 × 0.35 = 2.8, exceeds 1.0 so will hard-clip.
      // Typical use (3-4 voices, varied velocities): ~1.0-1.5, minimal clipping.
      // The safety clamp below catches peaks; per-voice filter saturation also helps.
      constexpr float kPolyScale = 0.35f;
      float gainScale = kPolyScale * mGainSmoothed;

      for (int c = 0; c < nOutputs; c++)
      {
        outputs[c][s] *= gainScale;

        // ───────────────────────────────────────────────────────────────────────
        // SOFT SATURATION WITH SMOOTH BLEND
        // ───────────────────────────────────────────────────────────────────────
        // Instead of harsh digital clipping (which creates odd harmonics and
        // sounds brittle), we use soft saturation via a fast tanh approximation.
        //
        // SMOOTH BLEND: Unlike a hard threshold (which creates a discontinuity
        // in the transfer function at the threshold), we smoothly crossfade
        // from dry to saturated starting at 0.5. This preserves dynamics at
        // low levels while providing transparent limiting at high levels.
        //
        //   Blend curve:  0 at |x|=0.5, 1 at |x|=1.0, smoothly interpolated
        //
        // Benefits:
        //   - No discontinuity in the transfer function
        //   - More transparent limiting
        //   - Warmer, more "analog" sound when driven hard
        //   - Graceful degradation instead of sudden distortion
        // ───────────────────────────────────────────────────────────────────────
        float& sample = outputs[c][s];
        float absVal = std::abs(sample);
        if (absVal > 0.5f)
        {
          // Smooth blend from dry to saturated: 0% at 0.5, 100% at 1.0+
          float blend = (absVal - 0.5f) * 2.0f;  // 0→1 as |sample| goes 0.5→1.0
          blend = std::min(1.0f, blend);         // Clamp to 1.0 for signals > 1.0

          // Apply saturation with gentle drive
          float saturated = softSaturate(sample * 1.2f);

          // Crossfade: dry + blend × (wet - dry) = dry × (1-blend) + wet × blend
          sample = sample + blend * (saturated - sample);
        }
      }
    }
  }

  void Reset(double sampleRate, int blockSize)
  {
    mSynth.SetSampleRateAndBlockSize(sampleRate, blockSize);
    mSynth.Reset();
    mGainSmoothed = mGain;

    // Calculate gain smoothing coefficient for ~20ms time constant
    // This ensures consistent smoothing behavior across all sample rates
    mGainSmoothCoeff = calcSmoothingCoeff(0.02f, static_cast<float>(sampleRate));
  }

  void ProcessMidiMsg(const IMidiMsg& msg)
  {
    mSynth.AddMidiMsgToQueue(msg);
  }

  void SetParam(int paramIdx, double value)
  {
    switch (paramIdx)
    {
      case kParamGain:
        mGain = static_cast<float>(value / 100.0);
        break;

      case kParamWaveform:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          dynamic_cast<Voice&>(voice).SetWaveform(static_cast<int>(value));
        });
        break;

      case kParamWavetablePosition:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          dynamic_cast<Voice&>(voice).SetWavetablePosition(static_cast<float>(value / 100.0));
        });
        break;

      case kParamAttack:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          dynamic_cast<Voice&>(voice).SetAttack(static_cast<float>(value));
        });
        break;

      case kParamDecay:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          dynamic_cast<Voice&>(voice).SetDecay(static_cast<float>(value));
        });
        break;

      case kParamSustain:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          dynamic_cast<Voice&>(voice).SetSustain(static_cast<float>(value / 100.0));
        });
        break;

      case kParamRelease:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          dynamic_cast<Voice&>(voice).SetRelease(static_cast<float>(value));
        });
        break;

      case kParamEnvVelocity:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          // Convert 0-100% to 0.0-1.0
          dynamic_cast<Voice&>(voice).SetEnvVelocitySensitivity(static_cast<float>(value / 100.0));
        });
        break;

      // Filter parameters
      case kParamFilterCutoff:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          dynamic_cast<Voice&>(voice).SetFilterCutoff(static_cast<float>(value));
        });
        break;

      case kParamFilterResonance:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          dynamic_cast<Voice&>(voice).SetFilterResonance(static_cast<float>(value / 100.0));
        });
        break;

      case kParamFilterType:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          dynamic_cast<Voice&>(voice).SetFilterType(static_cast<int>(value));
        });
        break;

      // Pulse width modulation
      case kParamPulseWidth:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          // Convert 5-95% to 0.05-0.95 for Q library pulse_osc
          dynamic_cast<Voice&>(voice).SetPulseWidth(static_cast<float>(value / 100.0));
        });
        break;

      // FM synthesis parameters (DX7-style coarse + fine)
      case kParamFMRatio:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          // Convert enum index to coarse ratio: 0→0.5, 1→1, 2→2, 3→3, etc.
          int idx = static_cast<int>(value);
          float ratio = (idx == 0) ? 0.5f : static_cast<float>(idx);
          dynamic_cast<Voice&>(voice).SetFMRatioCoarse(ratio);
        });
        break;

      case kParamFMFine:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          // Convert -50% to +50% to -0.5 to +0.5
          dynamic_cast<Voice&>(voice).SetFMRatioFine(static_cast<float>(value / 100.0));
        });
        break;

      case kParamFMDepth:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          // Convert 0-100% to 0.0-1.0
          dynamic_cast<Voice&>(voice).SetFMDepth(static_cast<float>(value / 100.0));
        });
        break;

      case kParamOsc1Level:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          // Convert 0-100% to 0.0-1.0
          dynamic_cast<Voice&>(voice).SetOsc1Level(static_cast<float>(value / 100.0));
        });
        break;

      case kParamOsc1Octave:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          // Convert enum index (0-4) to octave offset (-2 to +2)
          int octave = static_cast<int>(value) - 2;
          dynamic_cast<Voice&>(voice).SetOsc1Octave(octave);
        });
        break;

      case kParamOsc1Detune:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          // Value is already in cents (-100 to +100)
          dynamic_cast<Voice&>(voice).SetOsc1Detune(static_cast<float>(value));
        });
        break;

      // Oscillator 2 parameters
      case kParamOsc2Waveform:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          dynamic_cast<Voice&>(voice).SetOsc2Waveform(static_cast<int>(value));
        });
        break;

      case kParamOsc2Octave:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          // Convert enum index (0-4) to octave offset (-2 to +2)
          int octave = static_cast<int>(value) - 2;
          dynamic_cast<Voice&>(voice).SetOsc2Octave(octave);
        });
        break;

      case kParamOsc2Detune:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          // Value is already in cents (-100 to +100)
          dynamic_cast<Voice&>(voice).SetOsc2Detune(static_cast<float>(value));
        });
        break;

      case kParamOsc2Level:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          // Convert 0-100% to 0.0-1.0
          dynamic_cast<Voice&>(voice).SetOsc2Level(static_cast<float>(value / 100.0));
        });
        break;

      // Osc2 independent waveform-specific parameters (Serum-style)
      case kParamOsc2Morph:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          // Convert 0-100% to 0.0-1.0
          dynamic_cast<Voice&>(voice).SetOsc2Morph(static_cast<float>(value / 100.0));
        });
        break;

      case kParamOsc2PulseWidth:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          // Convert 5-95% to 0.05-0.95
          dynamic_cast<Voice&>(voice).SetOsc2PulseWidth(static_cast<float>(value / 100.0));
        });
        break;

      case kParamOsc2FMRatio:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          // Enum index to ratio: 0=0.5, 1=1, 2=2, 3=3, 4=4, 5=5, 6=6, 7=7, 8=8
          static const float kRatios[] = {0.5f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f};
          int idx = static_cast<int>(value);
          float ratio = (idx >= 0 && idx < 9) ? kRatios[idx] : 2.0f;
          dynamic_cast<Voice&>(voice).SetOsc2FMRatio(ratio);
        });
        break;

      case kParamOsc2FMFine:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          // Convert -50% to +50% → -0.5 to +0.5 offset
          dynamic_cast<Voice&>(voice).SetOsc2FMFine(static_cast<float>(value / 100.0));
        });
        break;

      case kParamOsc2FMDepth:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          // Convert 0-100% to 0.0-1.0
          dynamic_cast<Voice&>(voice).SetOsc2FMDepth(static_cast<float>(value / 100.0));
        });
        break;

      // ─────────────────────────────────────────────────────────────────────────
      // OSC1 UNISON PARAMETERS
      // Per-oscillator unison for Serum-style sound design flexibility.
      // ─────────────────────────────────────────────────────────────────────────
      case kParamOsc1UnisonVoices:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          // Enum index directly maps to voice count: 0=1, 1=2, ..., 7=8
          int voices = static_cast<int>(value) + 1;
          dynamic_cast<Voice&>(voice).SetOsc1UnisonVoices(voices);
        });
        break;

      case kParamOsc1UnisonDetune:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          // Convert 0-100% to 0.0-1.0 (spreads up to kMaxUnisonDetuneCents)
          dynamic_cast<Voice&>(voice).SetOsc1UnisonDetune(static_cast<float>(value / 100.0));
        });
        break;

      case kParamOsc1UnisonWidth:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          // Convert 0-100% to 0.0-1.0 (stereo spread)
          dynamic_cast<Voice&>(voice).SetOsc1UnisonWidth(static_cast<float>(value / 100.0));
        });
        break;

      case kParamOsc1UnisonBlend:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          // Convert 0-100% to 0.0-1.0 (center vs detuned mix)
          dynamic_cast<Voice&>(voice).SetOsc1UnisonBlend(static_cast<float>(value / 100.0));
        });
        break;

      // ─────────────────────────────────────────────────────────────────────────
      // OSC2 UNISON PARAMETERS (Independent from Osc1)
      // ─────────────────────────────────────────────────────────────────────────
      case kParamOsc2UnisonVoices:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          int voices = static_cast<int>(value) + 1;
          dynamic_cast<Voice&>(voice).SetOsc2UnisonVoices(voices);
        });
        break;

      case kParamOsc2UnisonDetune:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          dynamic_cast<Voice&>(voice).SetOsc2UnisonDetune(static_cast<float>(value / 100.0));
        });
        break;

      case kParamOsc2UnisonWidth:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          dynamic_cast<Voice&>(voice).SetOsc2UnisonWidth(static_cast<float>(value / 100.0));
        });
        break;

      case kParamOsc2UnisonBlend:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          dynamic_cast<Voice&>(voice).SetOsc2UnisonBlend(static_cast<float>(value / 100.0));
        });
        break;

      // ─────────────────────────────────────────────────────────────────────────
      // OSCILLATOR SYNC
      // Classic analog sync - Osc2 resets when Osc1 completes a cycle.
      // ─────────────────────────────────────────────────────────────────────────
      case kParamOscSync:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          // Enum: 0=Off, 1=Hard
          dynamic_cast<Voice&>(voice).SetOscSync(static_cast<int>(value));
        });
        break;

      default:
        break;
    }
  }

private:
  MidiSynth mSynth{VoiceAllocator::kPolyModePoly};
  const WavetableOscillator::WavetableData* mWavetable = nullptr;  // Pointer to static table
  float mGain = 0.8f;
  float mGainSmoothed = 0.8f;
  float mGainSmoothCoeff = 0.001f;  // Sample-rate aware, set in Reset()
};
