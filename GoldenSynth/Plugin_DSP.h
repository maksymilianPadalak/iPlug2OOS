#pragma once

// ╔═══════════════════════════════════════════════════════════════════════════════╗
// ║                    ⚠️  REAL-TIME AUDIO DSP CODE  ⚠️                            ║
// ╠═══════════════════════════════════════════════════════════════════════════════╣
// ║  ProcessBlock() runs on the audio thread with a strict deadline (~1-5ms).     ║
// ║  Violating real-time constraints causes audio glitches and dropouts.          ║
// ║                                                                               ║
// ║  🚫 NEVER DO THIS IN AUDIO THREAD (ProcessBlock, ProcessSamplesAccumulating): ║
// ║     • Memory allocation: new, delete, malloc, free, std::vector, std::string  ║
// ║     • Blocking: std::mutex, locks, file I/O, network, console output          ║
// ║     • Unbounded loops or recursion                                            ║
// ║                                                                               ║
// ║  ✅ SAFE FOR AUDIO THREAD:                                                    ║
// ║     • Fixed-size stack arrays, std::array<T,N>, pre-allocated buffers         ║
// ║     • std::atomic for thread-safe flags                                       ║
// ║     • Simple math, table lookups, bounded loops                               ║
// ╚═══════════════════════════════════════════════════════════════════════════════╝

#include <vector>      // OK for member variables, NEVER in ProcessBlock!
#include <atomic>

// DSP Utilities (DenormalGuard, math constants, fast math functions)
#include "dsp_utilities.h"

// Wavetable Oscillator (WavetableOscillator, WavetableGenerator)
#include "wavetable.h"

// Resonant Filter (Cytomic SVF)
#include "resonant_filter.h"

// Stereo Delay (Hermite interpolation, DC blocking, tempo sync)
#include "stereo_delay.h"

// LFO (Low Frequency Oscillator - Q library based)
#include "lfo.h"

// Dattorro Plate Reverb (professional-quality reverb effect)
#include "dattorro_reverb.h"

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
#include <q/synth/noise_gen.hpp>
#include <q/fx/dc_block.hpp>
#include <q/fx/delay.hpp>
// Note: biquad.hpp removed - using Cytomic SVF instead (stable under modulation)
#include <q/fx/envelope.hpp>
#include <q/fx/dynamic.hpp>

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
// - Sustain: Holds level indefinitely (sustain_rate set to ~infinite)
// - Release: Exponential fall (natural decay to silence)
//
// NOTE: Q library ADSR is actually ADBDR (piano-like) with optional sustain decay.
// We disable this by setting sustain_rate to ~100000 seconds for synth behavior.
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
// ═══════════════════════════════════════════════════════════════════════════════
// COMPLETE SIGNAL FLOW (with effects chain)
// ═══════════════════════════════════════════════════════════════════════════════
//
//   ┌─────────────────────────────────────────────────────────────────────────────┐
//   │  MIDI → MidiSynth → Voice[] → Mix → Master Gain → EFFECTS → Limiter → Out  │
//   └─────────────────────────────────────────────────────────────────────────────┘
//
//   DETAILED BREAKDOWN:
//
//   1. MIDI INPUT
//      └─ MIDI events queued via ProcessMidiMsg()
//
//   2. VOICE SYNTHESIS (per-voice processing)
//      └─ Each Voice: OSC (with Unison) → Filter → Amp Envelope → Soft Clip
//      └─ Voices accumulated (+=) into output buffer for polyphony
//
//   3. MASTER SECTION
//      └─ Smoothed master gain applied (click-free automation)
//      └─ Post-gain soft clipping (fastTanh) to tame transient peaks
//
//   4. EFFECTS CHAIN (processed in series, each with bypass)
//      ┌──────────────────────────────────────────────────────────────────────────┐
//      │  INPUT → [DELAY] → [REVERB] → [future effects here] → OUTPUT            │
//      └──────────────────────────────────────────────────────────────────────────┘
//      │
//      ├─ DELAY (StereoDelay) - Tempo-syncable stereo/ping-pong delay
//      │   └─ Controlled by: mDelayEnable, mDelay member variable
//      │   └─ Processing: Plugin_DSP.h line ~3045 (search "STEREO DELAY EFFECT")
//      │
//      └─ REVERB (DattorroReverb) - Dattorro plate reverb algorithm
//          └─ Controlled by: mReverbEnable, mReverb member variable
//          └─ Processing: Plugin_DSP.h line ~3065 (search "REVERB EFFECT")
//
//   5. OUTPUT LIMITER
//      └─ Q library soft-knee compressor (brickwall limiting)
//      └─ Final fastTanh soft clip + NaN/Inf protection
//
// ═══════════════════════════════════════════════════════════════════════════════
// HOW TO ADD A NEW EFFECT
// ═══════════════════════════════════════════════════════════════════════════════
//
// Follow this checklist to add a new effect (e.g., chorus, phaser, distortion):
//
// 1. CREATE THE EFFECT MODULE (optional - can use inline code for simple effects)
//    └─ Create myeffect.h in the plugin folder (see dattorro_reverb.h as example)
//    └─ Class should have: SetSampleRate(), Reset(), Process(left, right)
//
// 2. ADD INCLUDE (Plugin_DSP.h, near other effect includes ~line 38)
//    └─ #include "myeffect.h"
//
// 3. ADD PARAMETERS (Plugin_Params.h, in EParams enum)
//    └─ Add after existing effects section:
//       kParamMyEffectEnable,    // On/off toggle
//       kParamMyEffectParam1,    // Effect parameter 1
//       kParamMyEffectParam2,    // etc.
//
// 4. ADD MEMBER VARIABLES (Plugin_DSP.h, near mDelay/mReverb ~line 4240)
//    └─ MyEffect mMyEffect;
//    └─ bool mMyEffectEnable = false;  // Default OFF for effects
//
// 5. ADD RESET CALL (Plugin_DSP.h, in Reset() function ~line 3205)
//    └─ mMyEffect.SetSampleRate(static_cast<float>(sampleRate));
//    └─ mMyEffect.Reset();
//    └─ Also add in transport restart section if effect has delay lines
//
// 6. ADD PROCESSING LOOP (Plugin_DSP.h, after reverb, before limiter ~line 3100)
//    └─ Pattern:
//       if (nOutputs >= 2 && mMyEffectEnable)
//       {
//         for (int s = 0; s < nFrames; s++)
//         {
//           float left = static_cast<float>(outputs[0][s]);
//           float right = static_cast<float>(outputs[1][s]);
//           mMyEffect.Process(left, right);  // Process in-place
//           left = fastTanh(left);           // Post-effect soft clip
//           right = fastTanh(right);
//           outputs[0][s] = static_cast<T>(left);
//           outputs[1][s] = static_cast<T>(right);
//         }
//       }
//
// 7. ADD SETPARAM HANDLING (Plugin_DSP.h, in SetParam() switch ~line 4080)
//    └─ case kParamMyEffectEnable:
//         mMyEffectEnable = value > 0.5;
//         break;
//       case kParamMyEffectParam1:
//         mMyEffect.SetParam1(static_cast<float>(value / 100.0));
//         break;
//
// 8. INITIALIZE PARAMETERS (Plugin.cpp constructor)
//    └─ GetParam(kParamMyEffectEnable)->InitBool("MyEffect", false);
//    └─ GetParam(kParamMyEffectParam1)->InitDouble("Param1", 50., 0., 100., 0.1, "%");
//
// ═══════════════════════════════════════════════════════════════════════════════
// EFFECT PROCESSING ORDER
// ═══════════════════════════════════════════════════════════════════════════════
//
// Effects are processed in SERIES (output of one feeds input of next):
//
//   Dry Signal → Delay → Reverb → [Your Effect] → Limiter → Output
//
// Recommended order for common effects:
//   1. Compressor/Gate (dynamics - applied to dry signal)
//   2. EQ (tone shaping)
//   3. Distortion/Saturation (waveshaping)
//   4. Chorus/Flanger/Phaser (modulation)
//   5. Delay (time-based, benefits from modulated input)
//   6. Reverb (space, should be LAST time-based effect)
//   7. Limiter (always last - prevents clipping)
//
// Each effect has its own enable flag for true bypass (zero CPU when off).
//
// ═══════════════════════════════════════════════════════════════════════════════
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
        q::duration{100000.0f}, // Sustain rate: ~infinite (no decay during sustain)
                              // Q library ADSR is actually ADBDR (piano-like) where sustain
                              // slowly decays. Set to ~100000 seconds to effectively disable.
        q::duration{0.2f}     // 200ms release
      };

      // Filter envelope config - default to 0% sustain for classic "pluck" behavior
      // where filter opens on attack and closes during decay
      mFilterEnvConfig = q::adsr_envelope_gen::config{
        q::duration{0.01f},     // 10ms attack
        q::duration{0.1f},      // 100ms decay
        q::lin_to_db(0.001f),   // ~0% sustain (filter closes after decay)
        q::duration{100000.0f}, // Sustain rate disabled
        q::duration{0.2f}       // 200ms release
      };

      // ─────────────────────────────────────────────────────────────────────────
      // INITIALIZE UNISON PHASE ITERATORS
      // Without explicit initialization, phase_iterator arrays contain garbage
      // values that can cause clicks or undefined behavior on the first note.
      // We initialize them with a default frequency (will be overwritten in
      // ProcessSamplesAccumulating, but ensures they're in a valid state).
      // ─────────────────────────────────────────────────────────────────────────
      constexpr float kDefaultFreq = 440.0f;
      constexpr float kDefaultSampleRate = 48000.0f;
      for (int v = 0; v < kMaxUnisonVoices; v++)
      {
        mOsc1UnisonPhases[v].set(q::frequency{kDefaultFreq}, kDefaultSampleRate);
        mOsc2UnisonPhases[v].set(q::frequency{kDefaultFreq}, kDefaultSampleRate);
        mOsc1UnisonPulseOscs[v] = q::pulse_osc{0.5f};
        mOsc2UnisonPulseOscs[v] = q::pulse_osc{0.5f};
        // Initialize wavetable phases to 0
        mOsc1WavetablePhases[v] = 0.0f;
        mOsc2WavetablePhases[v] = 0.0f;
        mOsc1WavetablePhaseIncs[v] = 0.0f;
        mOsc2WavetablePhaseIncs[v] = 0.0f;
        mOsc1WavetableFreqs[v] = kDefaultFreq;
        mOsc2WavetableFreqs[v] = kDefaultFreq;
      }
    }

    // Set shared wavetable data (called once from DSP class)
    void SetWavetable(const WavetableOscillator::WavetableData* table)
    {
      mWavetableOsc.SetWavetable(table);
      mOsc2WavetableOsc.SetWavetable(table);  // Osc2 shares the same wavetable
    }

    // Set parent DSP class pointer (for accessing global LFO buffers)
    void SetParent(PluginInstanceDSP* parent) { mParent = parent; }

    bool GetBusy() const override
    {
      // If marked for recycling, report as not busy so allocator picks this voice
      // Using memory_order_acquire ensures we see the latest write from MarkForRecycle()
      if (mForceRecycle.load(std::memory_order_acquire)) return false;
      return mActive && !mEnv.in_idle_phase();
    }

    // Check if this voice is a good candidate for stealing (releasing but still sounding)
    bool IsReleasingCandidate() const
    {
      return mIsReleasing.load(std::memory_order_acquire) &&
             mActive &&
             !mEnv.in_idle_phase() &&
             !mForceRecycle.load(std::memory_order_acquire);
    }

    // Get current envelope level (0.0 = silent, 1.0 = peak)
    // Used to select the best voice to steal (prefer lowest level = closest to silent)
    float GetEnvelopeLevel() const
    {
      return mEnv.current();
    }

    // Mark this voice for recycling - GetBusy() will return false
    // Using memory_order_release ensures the write is visible to GetBusy()
    void MarkForRecycle()
    {
      mForceRecycle.store(true, std::memory_order_release);
    }

    // ─────────────────────────────────────────────────────────────────────────
    // VOICE STEALING WITH CROSSFADE
    // Initiates a quick fade-out (~1ms) before killing the voice.
    // This prevents clicks when voices are force-killed due to voice cap.
    // Uses sample rate to calculate proper fade duration.
    // ─────────────────────────────────────────────────────────────────────────
    void StartStealFade()
    {
      if (mStealFadeCounter == 0)  // Only start if not already fading
      {
        // Calculate ~1ms fade at current sample rate
        mStealFadeCounter = static_cast<int>(mSampleRate * 0.001);
        mStealFadeCounter = std::max(16, mStealFadeCounter);  // At least 16 samples
        mStealFadeDecrement = 1.0f / static_cast<float>(mStealFadeCounter);
        mStealFadeGain = 1.0f;
      }
    }

    // Check if this voice is currently fading out due to stealing
    bool IsBeingStolen() const
    {
      return mStealFadeCounter > 0;
    }

    // Set dynamic release multiplier (called from ProcessBlock based on voice count)
    void SetReleaseSpeedMultiplier(float multiplier)
    {
      mReleaseSpeedMultiplier = multiplier;
    }

    void Trigger(double level, bool isRetrigger) override
    {
      // Capture current envelope level BEFORE creating new envelope
      // This is needed for smooth retriggering (legato playing)
      float currentLevel = mActive ? mEnv.current() : 0.0f;

      mActive = true;

      // ─────────────────────────────────────────────────────────────────────────
      // VELOCITY SMOOTHING - Prevent clicks from instant velocity changes
      // ─────────────────────────────────────────────────────────────────────────
      // Problem: If velocity changes instantly on retrigger (e.g., 1.0 → 0.5),
      // the audio amplitude halves immediately, causing a click. The envelope
      // crossfade only handles envelope level, not velocity scaling.
      //
      // Solution: On retrigger, smooth velocity over ~5ms. On first note,
      // set velocity instantly (no previous audio to click against).
      // ─────────────────────────────────────────────────────────────────────────
      mTargetVelocity = static_cast<float>(level);
      mVelocitySmoothCoeff = calcSmoothingCoeff(0.005f, static_cast<float>(mSampleRate));

      if (!isRetrigger)
      {
        // First note: set velocity instantly (no click risk - voice was silent)
        mVelocity = mTargetVelocity;
      }
      // On retrigger: mVelocity stays at current value, smooths to mTargetVelocity in ProcessSamples

      // Reset voice stealing flags - this voice is now actively held
      // Using memory_order_release ensures these writes are visible to other threads
      mIsReleasing.store(false, std::memory_order_release);
      mForceRecycle.store(false, std::memory_order_release);

      // Reset steal fade (voice is now actively triggered)
      mStealFadeGain = 1.0f;
      mStealFadeCounter = 0;

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
      else
      {
        // ─────────────────────────────────────────────────────────────────────────
        // RETRIGGER: Preserve oscillator phase and filter state
        // ─────────────────────────────────────────────────────────────────────────
        // DO NOT reset filter here. Resetting SVF integrators mid-note causes
        // clicks because the filter state holds signal that gets zeroed instantly.
        // The envelope crossfade (mRetriggerOffset) handles amplitude smoothing,
        // but can't fix discontinuities in the filter's internal state.
        //
        // Tradeoff: Some resonance tail may bleed into the new note, but this is
        // far less audible than a hard click from filter reset.
        // ─────────────────────────────────────────────────────────────────────────
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

      // Filter envelope - same velocity scaling pattern as amp envelope
      q::adsr_envelope_gen::config velFilterEnvConfig = mFilterEnvConfig;
      velFilterEnvConfig.attack_rate = q::duration{mBaseFilterAttackMs * 0.001f * velScale};
      velFilterEnvConfig.decay_rate = q::duration{mBaseFilterDecayMs * 0.001f * velScale};
      velFilterEnvConfig.release_rate = q::duration{mBaseFilterReleaseMs * 0.001f * velScale};
      mFilterEnv = q::adsr_envelope_gen{velFilterEnvConfig, static_cast<float>(mSampleRate)};
      mFilterEnv.attack();

      // NOTE: LFO retrigger is now handled at the global level in ProcessMidiMsg.
      // Global LFOs are shared across all voices, so retrigger affects all notes.

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
      mFilterEnv.release();
      // Mark as releasing for smart voice stealing
      // Using memory_order_release ensures this write is visible to ProcessMidiMsg
      mIsReleasing.store(true, std::memory_order_release);
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
      double targetPitch = mInputs[kVoiceControlPitch].endValue;
      double pitchBend = mInputs[kVoiceControlPitchBend].endValue;

      // ─────────────────────────────────────────────────────────────────────────
      // PORTAMENTO (GLIDE) - LINEAR INTERPOLATION in PITCH domain
      // ─────────────────────────────────────────────────────────────────────────
      // Glide in pitch domain (1V/octave) ensures equal time across all octaves:
      //   - C2→C3 (1 octave) takes same time as C5→C6 (1 octave)
      //   - Frequency domain would make high notes glide slower
      //
      // Linear interpolation: glide completes in EXACTLY the specified time.
      // targetPitch = mInputs[kVoiceControlPitch].endValue (single source of truth)
      //
      // BLOCK PROCESSING: We advance the glide by nFrames samples per block.
      // This ensures correct timing while being efficient (one freq update per block).
      // The slight stepping between blocks is inaudible at typical block sizes (64-512).
      // ─────────────────────────────────────────────────────────────────────────
      double glidingPitch;

      if (mGlideSamplesRemaining > 0 && mCurrentPitch > kPitchInitThreshold)
      {
        // Calculate how many samples to advance this block
        int samplesToAdvance = std::min(mGlideSamplesRemaining, nFrames);

        // Advance pitch by the block's worth of steps
        mCurrentPitch += mGlideStepPerSample * samplesToAdvance;
        mGlideSamplesRemaining -= samplesToAdvance;

        // On completion, snap to exact target (prevents floating point drift)
        if (mGlideSamplesRemaining <= 0)
        {
          mCurrentPitch = targetPitch;  // targetPitch from mInputs (single source of truth)
        }
        glidingPitch = mCurrentPitch;
      }
      else
      {
        // No glide - use target pitch directly (poly mode or glide disabled)
        glidingPitch = targetPitch;
        mCurrentPitch = targetPitch;  // Keep in sync for future glides
      }

      // Convert pitch to frequency: 440Hz × 2^(pitch + bend)
      double baseFreq = 440.0 * std::pow(2.0, glidingPitch + pitchBend);

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

        // ─────────────────────────────────────────────────────────────────────
        // SPECIAL CASE: 2 VOICES - Symmetric spread (no center voice)
        // ─────────────────────────────────────────────────────────────────────
        // For 2 voices, blend controls BOTH pitch spread AND stereo spread:
        //   blend = 0%: Both voices at center pitch AND center position (like 1 voice)
        //   blend = 100%: Full detune spread AND full stereo spread
        // This makes blend=0 sound identical to 1 voice, matching 3+ voice behavior.
        // ─────────────────────────────────────────────────────────────────────
        if (mOsc1UnisonVoices == 2)
        {
          // Apply blend to detune spread (blend=0 means no detune, like 1 voice)
          float blendedSpreadCents = spreadCents * mOsc1UnisonBlend;
          // Voice 0: left, negative detune
          mOsc1UnisonDetuneOffsets[0] = -blendedSpreadCents;
          mOsc1UnisonPans[0] = -mOsc1UnisonWidth;
          // Voice 1: right, positive detune
          mOsc1UnisonDetuneOffsets[1] = blendedSpreadCents;
          mOsc1UnisonPans[1] = mOsc1UnisonWidth;
        }
        else
        {
          // 3+ VOICES: Center voice + symmetric spread voices
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
              float panPosition;
              float idx = static_cast<float>(v - 1);  // 0-indexed within spread voices

              if (numSpreadVoices % 2 == 1)
              {
                // Odd spread voices (3,5,7): linear from -width to +width (includes 0)
                panPosition = (2.0f * idx - (numSpreadVoices - 1)) / static_cast<float>(numSpreadVoices - 1);
              }
              else
              {
                // Even spread voices (2,4,6): straddle center (no voice at 0)
                panPosition = (2.0f * idx + 1.0f - numSpreadVoices) / static_cast<float>(numSpreadVoices);
              }
              mOsc1UnisonPans[v] = panPosition * mOsc1UnisonWidth;
            }
          }
        }
      }

      // OSC2 UNISON SETUP (Independent from Osc1, same symmetric algorithm)
      if (mOsc2UnisonVoices > 1)
      {
        float spreadCents = mOsc2UnisonDetune * kMaxUnisonDetuneCents;

        // SPECIAL CASE: 2 VOICES - Symmetric spread (no center voice)
        // For 2 voices, blend controls BOTH pitch spread AND stereo spread:
        //   blend = 0%: Both voices at center pitch AND center position (like 1 voice)
        //   blend = 100%: Full detune spread AND full stereo spread
        if (mOsc2UnisonVoices == 2)
        {
          // Apply blend to detune spread (blend=0 means no detune, like 1 voice)
          float blendedSpreadCents = spreadCents * mOsc2UnisonBlend;
          // Voice 0: left, negative detune
          mOsc2UnisonDetuneOffsets[0] = -blendedSpreadCents;
          mOsc2UnisonPans[0] = -mOsc2UnisonWidth;
          // Voice 1: right, positive detune
          mOsc2UnisonDetuneOffsets[1] = blendedSpreadCents;
          mOsc2UnisonPans[1] = mOsc2UnisonWidth;
        }
        else
        {
          // 3+ VOICES: Center voice + symmetric spread voices
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

              // PANNING: Perfectly symmetric spread
              float panPosition;
              float idx = static_cast<float>(v - 1);

              if (numSpreadVoices % 2 == 1)
              {
                // Odd spread voices (3,5,7): includes center
                panPosition = (2.0f * idx - (numSpreadVoices - 1)) / static_cast<float>(numSpreadVoices - 1);
              }
              else
              {
                // Even spread voices (2,4,6): straddles center
                panPosition = (2.0f * idx + 1.0f - numSpreadVoices) / static_cast<float>(numSpreadVoices);
              }
              mOsc2UnisonPans[v] = panPosition * mOsc2UnisonWidth;
            }
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

      // ─────────────────────────────────────────────────────────────────────────
      // SUB OSCILLATOR FREQUENCY CALCULATION
      // ─────────────────────────────────────────────────────────────────────────
      // Sub oscillator is always 1-2 octaves below the base pitch (before Osc1/Osc2
      // octave shifts). This ensures consistent sub bass regardless of oscillator
      // tuning settings. mSubOscOctave is -1 or -2.
      // ─────────────────────────────────────────────────────────────────────────
      if (mSubOscEnable)
      {
        double subFreq = baseFreq * std::pow(2.0, mSubOscOctave);
        mSubOscPhase.set(q::frequency{static_cast<float>(subFreq)}, static_cast<float>(mSampleRate));
      }

      // Cache atomic flag OUTSIDE the sample loop - avoid atomic load per sample
      const bool isReleasing = mIsReleasing.load(std::memory_order_acquire);
      const bool applyDynamicRelease = (mReleaseSpeedMultiplier > 1.0f) && isReleasing;

      for (int i = startIdx; i < startIdx + nFrames; i++)
      {
        // Get envelope amplitude
        float envAmp = mEnv();

        // ─────────────────────────────────────────────────────────────────────────
        // DYNAMIC RELEASE SCALING
        // When voice count is high, apply additional envelope decay to speed up
        // the release phase. This prevents voice explosion from unison + long release.
        //
        // Formula: envAmp *= (1 - multiplier * 0.002)^(multiplier)
        // At 48kHz with multiplier=16: decays to 0.01 in ~150 samples (~3ms)
        // ─────────────────────────────────────────────────────────────────────────
        if (applyDynamicRelease)
        {
          // More aggressive decay for higher multipliers
          // multiplier=2: decay = 0.996 per sample (~750 samples to 0.05)
          // multiplier=4: decay = 0.992 per sample (~375 samples to 0.05)
          // multiplier=8: decay = 0.984 per sample (~185 samples to 0.05)
          // multiplier=16: decay = 0.968 per sample (~90 samples to 0.05)
          float extraDecay = 1.0f - mReleaseSpeedMultiplier * 0.002f;
          extraDecay = std::max(0.95f, extraDecay);  // Min 0.95 to prevent instant kill
          envAmp *= extraDecay;

          // If envelope is very quiet, just kill the voice
          if (envAmp < 0.01f)
          {
            mActive = false;
            return;
          }
        }

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

        // ─────────────────────────────────────────────────────────────────────────
        // VELOCITY SMOOTHING
        // Smooth velocity towards target to prevent clicks on retrigger.
        // Uses same ~5ms time constant as envelope crossfade for consistency.
        // ─────────────────────────────────────────────────────────────────────────
        mVelocity += mVelocitySmoothCoeff * (mTargetVelocity - mVelocity);

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
        // GLOBAL LFO MODULATION - Read pre-computed values from parent DSP
        // ─────────────────────────────────────────────────────────────────────────
        // LFO values are pre-computed in ProcessBlock for the entire buffer.
        // This ensures all voices see the same LFO phase (Serum/Vital-style global LFOs).
        // The buffer values are already mapped from [-1,+1] to [Low,High].
        // ─────────────────────────────────────────────────────────────────────────

        // Initialize modulation accumulators
        // GLOBAL modulations (affect both oscillators)
        float filterMod = 0.0f;        // Octaves of filter modulation
        float ampMod = 0.0f;           // Amplitude modulation factor

        // PER-OSCILLATOR modulations (independent for Osc1 and Osc2)
        float osc1PitchMod = 0.0f;     // Osc1 pitch in semitones
        float osc2PitchMod = 0.0f;     // Osc2 pitch in semitones
        float osc1PwMod = 0.0f;        // Osc1 pulse width offset
        float osc2PwMod = 0.0f;        // Osc2 pulse width offset
        float osc1FmDepthMod = 0.0f;   // Osc1 FM depth offset
        float osc2FmDepthMod = 0.0f;   // Osc2 FM depth offset
        float osc1WtPosMod = 0.0f;     // Osc1 wavetable position offset
        float osc2WtPosMod = 0.0f;     // Osc2 wavetable position offset

        // Read pre-computed LFO values from global buffers (already scaled by Low/High)
        float lfo1ModAmount = mParent->mLFO1Buffer[i];
        float lfo2ModAmount = mParent->mLFO2Buffer[i];

        // ─────────────────────────────────────────────────────────────────────────
        // LFO1 MODULATION ROUTING
        // ─────────────────────────────────────────────────────────────────────────
        switch (mParent->mLFO1Destination)
        {
          // GLOBAL DESTINATIONS
          case LFODestination::Filter:
            filterMod += lfo1ModAmount * 4.0f;  // ±4 octaves at ±100%
            break;
          case LFODestination::Pitch:
            // Global pitch affects BOTH oscillators
            osc1PitchMod += lfo1ModAmount * 24.0f;  // ±24 semitones at ±100%
            osc2PitchMod += lfo1ModAmount * 24.0f;
            break;
          case LFODestination::PulseWidth:
            // Global PW affects BOTH oscillators
            osc1PwMod += lfo1ModAmount * 0.45f;  // ±45% PW at ±100%
            osc2PwMod += lfo1ModAmount * 0.45f;
            break;
          case LFODestination::Amplitude:
            ampMod += lfo1ModAmount;
            break;
          case LFODestination::FMDepth:
            // Global FM affects BOTH oscillators
            osc1FmDepthMod += lfo1ModAmount;
            osc2FmDepthMod += lfo1ModAmount;
            break;
          case LFODestination::WavetablePos:
            // Global WT position affects BOTH oscillators
            osc1WtPosMod += lfo1ModAmount * 0.5f;  // ±50% WT at ±100%
            osc2WtPosMod += lfo1ModAmount * 0.5f;
            break;

          // PER-OSCILLATOR DESTINATIONS
          case LFODestination::Osc1Pitch:
            osc1PitchMod += lfo1ModAmount * 24.0f;
            break;
          case LFODestination::Osc2Pitch:
            osc2PitchMod += lfo1ModAmount * 24.0f;
            break;
          case LFODestination::Osc1PulseWidth:
            osc1PwMod += lfo1ModAmount * 0.45f;
            break;
          case LFODestination::Osc2PulseWidth:
            osc2PwMod += lfo1ModAmount * 0.45f;
            break;
          case LFODestination::Osc1FMDepth:
            osc1FmDepthMod += lfo1ModAmount;
            break;
          case LFODestination::Osc2FMDepth:
            osc2FmDepthMod += lfo1ModAmount;
            break;
          case LFODestination::Osc1WTPos:
            osc1WtPosMod += lfo1ModAmount * 0.5f;
            break;
          case LFODestination::Osc2WTPos:
            osc2WtPosMod += lfo1ModAmount * 0.5f;
            break;

          case LFODestination::Off:
          default:
            break;
        }

        // ─────────────────────────────────────────────────────────────────────────
        // LFO2 MODULATION ROUTING (additive with LFO1)
        // ─────────────────────────────────────────────────────────────────────────
        switch (mParent->mLFO2Destination)
        {
          // GLOBAL DESTINATIONS
          case LFODestination::Filter:
            filterMod += lfo2ModAmount * 4.0f;
            break;
          case LFODestination::Pitch:
            osc1PitchMod += lfo2ModAmount * 24.0f;
            osc2PitchMod += lfo2ModAmount * 24.0f;
            break;
          case LFODestination::PulseWidth:
            osc1PwMod += lfo2ModAmount * 0.45f;
            osc2PwMod += lfo2ModAmount * 0.45f;
            break;
          case LFODestination::Amplitude:
            ampMod += lfo2ModAmount;
            break;
          case LFODestination::FMDepth:
            osc1FmDepthMod += lfo2ModAmount;
            osc2FmDepthMod += lfo2ModAmount;
            break;
          case LFODestination::WavetablePos:
            osc1WtPosMod += lfo2ModAmount * 0.5f;
            osc2WtPosMod += lfo2ModAmount * 0.5f;
            break;

          // PER-OSCILLATOR DESTINATIONS
          case LFODestination::Osc1Pitch:
            osc1PitchMod += lfo2ModAmount * 24.0f;
            break;
          case LFODestination::Osc2Pitch:
            osc2PitchMod += lfo2ModAmount * 24.0f;
            break;
          case LFODestination::Osc1PulseWidth:
            osc1PwMod += lfo2ModAmount * 0.45f;
            break;
          case LFODestination::Osc2PulseWidth:
            osc2PwMod += lfo2ModAmount * 0.45f;
            break;
          case LFODestination::Osc1FMDepth:
            osc1FmDepthMod += lfo2ModAmount;
            break;
          case LFODestination::Osc2FMDepth:
            osc2FmDepthMod += lfo2ModAmount;
            break;
          case LFODestination::Osc1WTPos:
            osc1WtPosMod += lfo2ModAmount * 0.5f;
            break;
          case LFODestination::Osc2WTPos:
            osc2WtPosMod += lfo2ModAmount * 0.5f;
            break;

          case LFODestination::Off:
          default:
            break;
        }

        // ─────────────────────────────────────────────────────────────────────────
        // SIGNAL CHAIN: OSC (with UNISON) → FILTER → AMP
        // This is the classic subtractive synthesis topology, enhanced with unison.
        //
        // When unison > 1, we generate multiple detuned copies of each oscillator
        // and mix them with stereo panning for the "massive" supersaw sound.
        // ─────────────────────────────────────────────────────────────────────────

        // Apply LFO pitch modulation via phase increment scaling
        // Per-oscillator pitch modulation allows independent vibrato on each osc
        // Using fastExp2 instead of std::pow for ~3-5x speedup
        float osc1PitchModRatio = fastExp2(osc1PitchMod / 12.0f);
        float osc2PitchModRatio = fastExp2(osc2PitchMod / 12.0f);

        // Smooth pulse width for Osc1 (used by all unison voices)
        // Apply per-oscillator LFO pulse width modulation
        float modulatedPWTarget = std::max(0.05f, std::min(0.95f, mPulseWidthTarget + osc1PwMod));
        mPulseWidth += mPulseWidthSmoothCoeff * (modulatedPWTarget - mPulseWidth);

        // Apply per-oscillator LFO pulse width modulation for Osc2
        float modulatedOsc2PW = std::max(0.05f, std::min(0.95f, mOsc2PulseWidth + osc2PwMod));

        // Smooth FM parameters for Osc1
        mFMRatioTarget = mFMRatioCoarse * (1.0f + mFMRatioFine);
        mFMRatio += mFMSmoothCoeff * (mFMRatioTarget - mFMRatio);
        mFMDepth += mFMSmoothCoeff * (mFMDepthTarget - mFMDepth);

        // Apply per-oscillator LFO FM depth modulation (clamped to 0-1 range)
        float modulatedFMDepth = std::max(0.0f, std::min(1.0f, mFMDepth + osc1FmDepthMod));
        float modulatedOsc2FMDepth = std::max(0.0f, std::min(1.0f, mOsc2FMDepth + osc2FmDepthMod));

        // Apply per-oscillator LFO wavetable position modulation (clamped to 0-1 range)
        float modulatedWTPos = std::max(0.0f, std::min(1.0f, mWavetablePosition + osc1WtPosMod));
        float modulatedOsc2WTPos = std::max(0.0f, std::min(1.0f, mOsc2MorphPosition + osc2WtPosMod));

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

          // ─────────────────────────────────────────────────────────────────
          // POLYBLEP ANTI-ALIASING FIX: Save original step, then modulate
          // ─────────────────────────────────────────────────────────────────
          // The Q library's anti-aliased oscillators (saw, square, pulse) use
          // phase_iterator._step for poly_blep's 'dt' parameter. When pitch
          // modulation is active, we must temporarily update _step to match
          // the actual phase increment, then RESTORE IT to prevent compounding.
          // Without restoration, each sample would multiply by pitchModRatio
          // again, causing exponential pitch drift within each audio block.
          // ─────────────────────────────────────────────────────────────────
          uint32_t originalStep = mOsc1UnisonPhases[v]._step.rep;  // Save original
          uint32_t osc1ModulatedStep = static_cast<uint32_t>(
              static_cast<float>(originalStep) * osc1PitchModRatio);
          mOsc1UnisonPhases[v]._step.rep = osc1ModulatedStep;  // Set for poly_blep

          switch (mWaveform)
          {
            case kWaveformSine:
              osc1Sample = q::sin(mOsc1UnisonPhases[v]);
              mOsc1UnisonPhases[v]._phase.rep += osc1ModulatedStep;
              break;
            case kWaveformSaw:
              osc1Sample = q::saw(mOsc1UnisonPhases[v]);
              mOsc1UnisonPhases[v]._phase.rep += osc1ModulatedStep;
              break;
            case kWaveformSquare:
              osc1Sample = q::square(mOsc1UnisonPhases[v]);
              mOsc1UnisonPhases[v]._phase.rep += osc1ModulatedStep;
              break;
            case kWaveformTriangle:
              osc1Sample = q::triangle(mOsc1UnisonPhases[v]);
              mOsc1UnisonPhases[v]._phase.rep += osc1ModulatedStep;
              break;
            case kWaveformPulse:
              mOsc1UnisonPulseOscs[v].width(mPulseWidth);
              osc1Sample = mOsc1UnisonPulseOscs[v](mOsc1UnisonPhases[v]);
              // Protection: PolyBLEP can produce edge cases with extreme width/frequency
              if (isAudioCorrupt(osc1Sample)) osc1Sample = 0.0f;
              mOsc1UnisonPhases[v]._phase.rep += osc1ModulatedStep;
              break;
            case kWaveformFM:
            {
              // ─────────────────────────────────────────────────────────────────
              // FM SYNTHESIS WITH PER-UNISON-VOICE MODULATOR
              // Each unison voice has its own modulator phase, ensuring correct
              // FM behavior when combined with unison detuning.
              // Pitch modulation is applied via scaled phase increment.
              // ─────────────────────────────────────────────────────────────────
              float phaseIncRadians = static_cast<float>(osc1ModulatedStep) /
                                      static_cast<float>(0xFFFFFFFFu) * kTwoPi;

              // Use per-voice modulator phase (not shared across voices)
              // Using wrapPhase() instead of while loop - more robust against NaN/corruption
              mOsc1FMModulatorPhases[v] += phaseIncRadians * mFMRatio;
              mOsc1FMModulatorPhases[v] = wrapPhase(mOsc1FMModulatorPhases[v]);

              // Using fastSin for ~5-10x speedup over std::sin
              float modulatorValue = fastSin(mOsc1FMModulatorPhases[v]);
              constexpr float kMaxModIndex = 4.0f * kPi;  // ~12.57 radians max modulation
              float velScaledDepth = modulatedFMDepth * (0.3f + 0.7f * mVelocity);
              float phaseModulation = velScaledDepth * kMaxModIndex * modulatorValue;

              float carrierPhase = static_cast<float>(mOsc1UnisonPhases[v]._phase.rep) /
                                   static_cast<float>(0xFFFFFFFFu) * kTwoPi;
              osc1Sample = fastSin(carrierPhase + phaseModulation);

              // Protection: if FM produces corrupt value, use 0 (silent) instead of NaN
              if (isAudioCorrupt(osc1Sample)) osc1Sample = 0.0f;

              mOsc1UnisonPhases[v]._phase.rep += osc1ModulatedStep;
              break;
            }
            case kWaveformWavetable:
            {
              // Apply LFO wavetable position modulation
              mWavetableOsc.SetPosition(modulatedWTPos);
              // Apply pitch modulation to wavetable phase increment (per-oscillator)
              float modulatedWTPhaseInc = mOsc1WavetablePhaseIncs[v] * osc1PitchModRatio;
              if (v == 0)
                osc1Sample = mWavetableOsc.ProcessWithPitchMod(osc1PitchModRatio);
              else
                osc1Sample = mWavetableOsc.ProcessAtPhase(mOsc1WavetablePhases[v], modulatedWTPhaseInc, mOsc1WavetableFreqs[v] * osc1PitchModRatio);
              break;
            }
            default:
              osc1Sample = q::sin(mOsc1UnisonPhases[v]);
              mOsc1UnisonPhases[v]._phase.rep += osc1ModulatedStep;
              break;
          }

          // Restore original step to prevent compounding on next sample
          mOsc1UnisonPhases[v]._step.rep = originalStep;

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
          else if (mOsc1UnisonVoices == 2)
          {
            // 2 voices: blend controls stereo spread
            // blend = 0%: both voices centered (mono-compatible)
            // blend = 100%: full stereo spread based on width
            float pan = mOsc1UnisonPans[v];
            float angle = (pan + 1.0f) * kQuarterPi;

            // Full pan gains (at blend = 100%) - using fast trig
            float fullPanLeft = fastCos(angle);
            float fullPanRight = fastSin(angle);

            // Interpolate between centered (kSqrtHalf) and full pan based on blend
            leftGain = kSqrtHalf + mOsc1UnisonBlend * (fullPanLeft - kSqrtHalf);
            rightGain = kSqrtHalf + mOsc1UnisonBlend * (fullPanRight - kSqrtHalf);
          }
          else if (v == 0)
          {
            // 3+ voices: Center voice with equal power to both channels (1/sqrt(2))
            leftGain = kSqrtHalf;
            rightGain = kSqrtHalf;
          }
          else
          {
            // 3+ voices: Spread voices with constant-power panning
            float pan = mOsc1UnisonPans[v];
            float angle = (pan + 1.0f) * kQuarterPi;
            leftGain = fastCos(angle);
            rightGain = fastSin(angle);
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
          if (mOsc1UnisonVoices == 2)
          {
            // 2 voices: equal weight, both are spread voices
            // Power sum = 2 × 0.5² = 0.5, compensation = sqrt(2)
            voiceWeight = kSqrtHalf;  // 1/sqrt(2) × sqrt(2) = 1 per voice after compensation
          }
          else if (mOsc1UnisonVoices > 2)
          {
            float centerWeight = 1.0f - mOsc1UnisonBlend;
            float spreadWeight = mOsc1UnisonBlend / static_cast<float>(mOsc1UnisonVoices - 1);

            // Calculate power sum for compensation
            float powerSum = centerWeight * centerWeight +
                             (mOsc1UnisonVoices - 1) * spreadWeight * spreadWeight;
            // Protect against division by zero or sqrt of tiny/negative values
            float gainCompensation = (powerSum > 1e-8f) ? (1.0f / std::sqrt(powerSum)) : 1.0f;

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

            // ─────────────────────────────────────────────────────────────────
            // POLYBLEP ANTI-ALIASING FIX: Save original step, then modulate
            // ─────────────────────────────────────────────────────────────────
            // Same as Osc1: Save original step, set modulated step for poly_blep,
            // then restore after oscillator processing to prevent compounding.
            // ─────────────────────────────────────────────────────────────────
            uint32_t originalStep = mOsc2UnisonPhases[v]._step.rep;  // Save original
            uint32_t osc2ModulatedStep = static_cast<uint32_t>(
                static_cast<float>(originalStep) * osc2PitchModRatio);
            mOsc2UnisonPhases[v]._step.rep = osc2ModulatedStep;  // Set for poly_blep

            switch (mOsc2Waveform)
            {
              case kWaveformSine:
                osc2Sample = q::sin(mOsc2UnisonPhases[v]);
                mOsc2UnisonPhases[v]._phase.rep += osc2ModulatedStep;
                break;
              case kWaveformSaw:
                osc2Sample = q::saw(mOsc2UnisonPhases[v]);
                mOsc2UnisonPhases[v]._phase.rep += osc2ModulatedStep;
                break;
              case kWaveformSquare:
                osc2Sample = q::square(mOsc2UnisonPhases[v]);
                mOsc2UnisonPhases[v]._phase.rep += osc2ModulatedStep;
                break;
              case kWaveformTriangle:
                osc2Sample = q::triangle(mOsc2UnisonPhases[v]);
                mOsc2UnisonPhases[v]._phase.rep += osc2ModulatedStep;
                break;
              case kWaveformPulse:
                // Apply LFO pulse width modulation to Osc2
                mOsc2UnisonPulseOscs[v].width(modulatedOsc2PW);
                osc2Sample = mOsc2UnisonPulseOscs[v](mOsc2UnisonPhases[v]);
                // Protection: PolyBLEP can produce edge cases with extreme width/frequency
                if (isAudioCorrupt(osc2Sample)) osc2Sample = 0.0f;
                mOsc2UnisonPhases[v]._phase.rep += osc2ModulatedStep;
                break;
              case kWaveformFM:
              {
                // ─────────────────────────────────────────────────────────────────
                // OSC2 FM SYNTHESIS WITH PER-UNISON-VOICE MODULATOR
                // Same as Osc1: each unison voice gets its own modulator phase.
                // Pitch modulation is applied via scaled phase increment.
                // ─────────────────────────────────────────────────────────────────
                float phaseIncRadians = static_cast<float>(osc2ModulatedStep) /
                                        static_cast<float>(0xFFFFFFFFu) * kTwoPi;
                float osc2CombinedRatio = mOsc2FMRatio + mOsc2FMFine;

                // Use per-voice modulator phase (not shared across voices)
                // Using wrapPhase() instead of while loop - more robust against NaN/corruption
                mOsc2FMModulatorPhases[v] += phaseIncRadians * osc2CombinedRatio;
                mOsc2FMModulatorPhases[v] = wrapPhase(mOsc2FMModulatorPhases[v]);

                // Using fastSin for ~5-10x speedup over std::sin
                float modulatorValue = fastSin(mOsc2FMModulatorPhases[v]);
                constexpr float kMaxModIndex = 4.0f * kPi;  // ~12.57 radians max modulation
                float velScaledDepth = modulatedOsc2FMDepth * (0.3f + 0.7f * mVelocity);
                float phaseModulation = velScaledDepth * kMaxModIndex * modulatorValue;

                float carrierPhase = static_cast<float>(mOsc2UnisonPhases[v]._phase.rep) /
                                     static_cast<float>(0xFFFFFFFFu) * kTwoPi;
                osc2Sample = fastSin(carrierPhase + phaseModulation);

                // Protection: if FM produces corrupt value, use 0 (silent) instead of NaN
                if (isAudioCorrupt(osc2Sample)) osc2Sample = 0.0f;

                mOsc2UnisonPhases[v]._phase.rep += osc2ModulatedStep;
                break;
              }
              case kWaveformWavetable:
              {
                // Apply LFO wavetable position modulation
                mOsc2WavetableOsc.SetPosition(modulatedOsc2WTPos);
                // Apply pitch modulation to wavetable phase increment (per-oscillator)
                float modulatedWTPhaseInc = mOsc2WavetablePhaseIncs[v] * osc2PitchModRatio;
                if (v == 0)
                  osc2Sample = mOsc2WavetableOsc.ProcessWithPitchMod(osc2PitchModRatio);
                else
                  osc2Sample = mOsc2WavetableOsc.ProcessAtPhase(mOsc2WavetablePhases[v], modulatedWTPhaseInc, mOsc2WavetableFreqs[v] * osc2PitchModRatio);
                break;
              }
              default:
                osc2Sample = q::sin(mOsc2UnisonPhases[v]);
                mOsc2UnisonPhases[v]._phase.rep += osc2ModulatedStep;
                break;
            }

            // Restore original step to prevent compounding on next sample
            mOsc2UnisonPhases[v]._step.rep = originalStep;

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
            else if (mOsc2UnisonVoices == 2)
            {
              // 2 voices: blend controls stereo spread
              // blend = 0%: both voices centered (mono-compatible)
              // blend = 100%: full stereo spread based on width
              float pan = mOsc2UnisonPans[v];
              float angle = (pan + 1.0f) * kQuarterPi;

              // Full pan gains (at blend = 100%) - using fast trig
              float fullPanLeft = fastCos(angle);
              float fullPanRight = fastSin(angle);

              // Interpolate between centered (kSqrtHalf) and full pan based on blend
              leftGain = kSqrtHalf + mOsc2UnisonBlend * (fullPanLeft - kSqrtHalf);
              rightGain = kSqrtHalf + mOsc2UnisonBlend * (fullPanRight - kSqrtHalf);
            }
            else if (v == 0)
            {
              // Center voice in unison (3+ voices): equal power (1/sqrt(2))
              leftGain = kSqrtHalf;
              rightGain = kSqrtHalf;
            }
            else
            {
              // Spread voices: constant-power panning
              float pan = mOsc2UnisonPans[v];
              float angle = (pan + 1.0f) * kQuarterPi;
              leftGain = fastCos(angle);
              rightGain = fastSin(angle);
            }

            // VOICE WEIGHT WITH POWER COMPENSATION (same as Osc1)
            float voiceWeight = 1.0f;
            if (mOsc2UnisonVoices == 2)
            {
              // 2 voices: Both are equal spread voices (no center)
              // Each voice gets 1/sqrt(2) for constant power
              voiceWeight = kSqrtHalf;
            }
            else if (mOsc2UnisonVoices > 2)
            {
              // 3+ voices: center + spread voices with blend control
              float centerWeight = 1.0f - mOsc2UnisonBlend;
              float spreadWeight = mOsc2UnisonBlend / static_cast<float>(mOsc2UnisonVoices - 1);

              float powerSum = centerWeight * centerWeight +
                               (mOsc2UnisonVoices - 1) * spreadWeight * spreadWeight;
              // Protect against division by zero or sqrt of tiny/negative values
              float gainCompensation = (powerSum > 1e-8f) ? (1.0f / std::sqrt(powerSum)) : 1.0f;

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
        // MIX OSCILLATORS (stereo) WITH PER-OSCILLATOR PAN
        // ─────────────────────────────────────────────────────────────────────────
        // Per-oscillator pan positions the entire oscillator (including all its
        // unison voices) in the stereo field. This is standard in Serum, Vital,
        // Massive, and other professional synths.
        //
        // Serum-style: direct application, no per-sample smoothing. Parameter
        // smoothing is handled at the UI/host level. More CPU efficient and
        // avoids any potential for drift.
        //
        // Linear balance law for stereo sources:
        //   - Center (0): L = 1.0, R = 1.0 (unchanged)
        //   - Left (-1):  L = 1.0, R = 0.0 (full left)
        //   - Right (+1): L = 0.0, R = 1.0 (full right)
        // ─────────────────────────────────────────────────────────────────────────
        // Clamp pan to [-1, 1] for safety (prevents negative gains / phase inversion)
        float pan1 = std::max(-1.0f, std::min(1.0f, mOsc1PanTarget));
        float pan2 = std::max(-1.0f, std::min(1.0f, mOsc2PanTarget));

        float osc1PanL = pan1 <= 0.0f ? 1.0f : (1.0f - pan1);
        float osc1PanR = pan1 >= 0.0f ? 1.0f : (1.0f + pan1);
        float osc2PanL = pan2 <= 0.0f ? 1.0f : (1.0f - pan2);
        float osc2PanR = pan2 >= 0.0f ? 1.0f : (1.0f + pan2);

        // ─────────────────────────────────────────────────────────────────────────
        // SUB OSCILLATOR - Simple waveform for bass foundation (Serum-style)
        // ─────────────────────────────────────────────────────────────────────────
        // A dedicated sub bass oscillator that generates a simple waveform
        // 1-3 octaves below the played note. Essential for:
        //   - Adding low-end weight to leads without muddying the mix
        //   - 808-style sub bass when used as the primary sound source
        //   - Solid foundation under complex supersaw/FM patches
        //
        // WAVEFORMS:
        // Four simple waveforms optimized for sub bass frequencies:
        //   - Sine:     Pure fundamental, cleanest sub (club-standard)
        //   - Triangle: Slight odd harmonics, warmer than sine
        //   - Saw:      All harmonics, aggressive/buzzy sub
        //   - Square:   Strong odd harmonics, hollow/punchy sub
        //
        // NOTE: These are NOT band-limited (no PolyBLEP). This is intentional -
        // the sub is always 1-3 octaves BELOW the played note, so aliasing is
        // not a concern. Even at C8 (4186Hz), the sub at -3 octaves is 523Hz.
        //
        // SOFT ENABLE/DISABLE:
        // Smoothing runs EVERY sample, even when sub is "off". When disabled,
        // effectiveTarget becomes 0 and the level fades smoothly to silence.
        // This prevents clicks when toggling the sub on/off.
        //
        // DIRECT OUT (Serum-style):
        // When enabled, sub output bypasses the filter entirely. This ensures
        // consistent bass regardless of filter sweeps - essential for club/PA
        // systems where filter automation shouldn't affect the sub foundation.
        // ─────────────────────────────────────────────────────────────────────────

        // ─────────────────────────────────────────────────────────────────────────
        // SUB PARAMETER SMOOTHING - Always runs for click-free enable/disable
        // ─────────────────────────────────────────────────────────────────────────
        // Sample-rate independent: ~5ms time constant at any sample rate.
        // Formula: coeff = exp(-1 / (time * sampleRate))
        // We approximate with: coeff = 1 - (1 / (time * sampleRate))
        // For 5ms: coeff ≈ 1 - (1 / (0.005 * 44100)) ≈ 0.9955 at 44.1kHz
        //
        // SOFT ENABLE: When sub is disabled, effectiveTarget = 0, causing
        // level to fade smoothly rather than click off instantly.
        // ─────────────────────────────────────────────────────────────────────────
        float subSmoothCoeff = 1.0f - (200.0f / mSampleRate);  // ~5ms at any sample rate
        float effectiveLevelTarget = mSubOscEnable ? mSubOscLevelTarget : 0.0f;
        mSubOscLevelSmoothed += (1.0f - subSmoothCoeff) * (effectiveLevelTarget - mSubOscLevelSmoothed);
        mSubOscPanSmoothed += (1.0f - subSmoothCoeff) * (mSubOscPanTarget - mSubOscPanSmoothed);

        // ─────────────────────────────────────────────────────────────────────────
        // SUB WAVEFORM GENERATION
        // ─────────────────────────────────────────────────────────────────────────
        // Generate the selected waveform from the phase iterator.
        // All waveforms use q:: functions for consistency with Osc1/Osc2.
        // Phase is always advanced (even at zero level) to maintain continuity.
        // ─────────────────────────────────────────────────────────────────────────
        float subSample = 0.0f;
        switch (mSubOscWaveform)
        {
          case 0:  // Sine - pure fundamental, cleanest
            subSample = q::sin(mSubOscPhase);
            break;
          case 1:  // Triangle - slight odd harmonics, warmer
            subSample = q::triangle(mSubOscPhase);
            break;
          case 2:  // Saw - all harmonics, aggressive
            subSample = q::saw(mSubOscPhase);
            break;
          case 3:  // Square - strong odd harmonics, punchy
            subSample = q::square(mSubOscPhase);
            break;
          default:
            subSample = q::sin(mSubOscPhase);
            break;
        }
        mSubOscPhase++;  // Always advance phase
        subSample *= mSubOscLevelSmoothed;

        // ─────────────────────────────────────────────────────────────────────────
        // SUB STEREO PAN - Linear balance law
        // ─────────────────────────────────────────────────────────────────────────
        // Pan position: -1.0 = full left, 0.0 = center, +1.0 = full right
        // NOTE: For club/PA compatibility, sub bass is typically kept centered.
        // Stereo sub can cause phase cancellation on mono systems.
        // ─────────────────────────────────────────────────────────────────────────
        float subPan = std::max(-1.0f, std::min(1.0f, mSubOscPanSmoothed));
        float subPanL = subPan <= 0.0f ? 1.0f : (1.0f - subPan);
        float subPanR = subPan >= 0.0f ? 1.0f : (1.0f + subPan);
        float subLeft = subSample * subPanL;
        float subRight = subSample * subPanR;

        // ─────────────────────────────────────────────────────────────────────────
        // MIX OSCILLATORS - Sub routing depends on Direct Out setting
        // ─────────────────────────────────────────────────────────────────────────
        // Direct Out OFF: Sub mixed here, goes through filter with Osc1/Osc2
        // Direct Out ON:  Sub NOT mixed here, added after filter (see below)
        // ─────────────────────────────────────────────────────────────────────────
        float mixedLeft = mOsc1Level * osc1Left * osc1PanL + mOsc2Level * osc2Left * osc2PanL;
        float mixedRight = mOsc1Level * osc1Right * osc1PanR + mOsc2Level * osc2Right * osc2PanR;
        if (!mSubOscDirectOut)
        {
          // Sub goes through filter (traditional routing)
          mixedLeft += subLeft;
          mixedRight += subRight;
        }

        // ─────────────────────────────────────────────────────────────────────────
        // FILTER MODULATION (LFO + ENVELOPE)
        // ─────────────────────────────────────────────────────────────────────────
        // Filter cutoff is modulated in octaves for musical results.
        // Sources: LFO (already in filterMod) + Filter Envelope
        //
        // Filter Envelope: 0-1 output scaled by depth (±1.0 = ±4 octaves)
        // Positive depth: envelope opens filter (classic pluck/bass sound)
        // Negative depth: envelope closes filter (inverse/pad sound)
        //
        // Formula: modulatedCutoff = baseCutoff × 2^(filterMod)
        //
        // IMPORTANT: We clamp filterMod (in octaves) BEFORE the exponential
        // calculation to prevent glitches. Hard-clamping the frequency AFTER
        // the exponential creates discontinuities (flat spots) in the LFO sweep
        // that cause audible clicks. By clamping in the octave domain, the
        // sweep smoothly approaches the limits without discontinuity.
        // ─────────────────────────────────────────────────────────────────────────

        // Add filter envelope modulation
        // Envelope outputs 0-1, scale by depth (±1.0) and range (4 octaves)
        float filterEnvValue = mFilterEnv();
        filterMod += filterEnvValue * mFilterEnvDepth * 4.0f;

        // Clamp filterMod to valid octave range (uses cached limits)
        filterMod = std::max(mFilterModMin, std::min(mFilterModMax, filterMod));

        float modulatedCutoff = mFilterCutoffBase * fastExp2(filterMod);
        mFilterL.SetCutoff(modulatedCutoff);
        mFilterR.SetCutoff(modulatedCutoff);

        // ─────────────────────────────────────────────────────────────────────────
        // FILTER (stereo - separate filters for L/R to preserve stereo image)
        // When disabled, pass raw oscillator signal through (hear unfiltered sound)
        // ─────────────────────────────────────────────────────────────────────────
        float filteredLeft, filteredRight;
        if (mParent->mFilterEnable)
        {
          filteredLeft = mFilterL.Process(mixedLeft);
          filteredRight = mFilterR.Process(mixedRight);
        }
        else
        {
          // Filter bypassed - pass through raw signal
          filteredLeft = mixedLeft;
          filteredRight = mixedRight;
        }

        // ─────────────────────────────────────────────────────────────────────────
        // DC BLOCKER - Q Library Implementation
        // ─────────────────────────────────────────────────────────────────────────
        // Uses q::dc_block based on Julius O. Smith's algorithm.
        // High-pass at ~10Hz, sample-rate independent, transparent above 30Hz.
        // Removes DC offset from FM synthesis, wavetable morphing, and filter
        // resonance to keep woofers happy and prevent clipping from DC accumulation.
        // ─────────────────────────────────────────────────────────────────────────
        float dcFreeLeft = mDCBlockerL(filteredLeft);
        float dcFreeRight = mDCBlockerR(filteredRight);

        // ─────────────────────────────────────────────────────────────────────────
        // AMPLITUDE MODULATION (Tremolo)
        // ─────────────────────────────────────────────────────────────────────────
        // Symmetric tremolo: LFO modulates volume around a center point.
        // Maps ampMod from [-1, +1] to amplitude multiplier [0, 1]:
        //   - ampMod = -1 (LFO min) → multiplier = 0.0 (silence)
        //   - ampMod =  0 (LFO mid) → multiplier = 0.5 (half volume)
        //   - ampMod = +1 (LFO max) → multiplier = 1.0 (full volume)
        //
        // Use Low/High to control the tremolo range:
        //   - Low=-100%, High=+100%: Full 0-100% tremolo
        //   - Low=0%, High=+100%: Subtle 50-100% tremolo (no silence)
        //   - Low=-100%, High=0%: Ducking effect 0-50%
        // ─────────────────────────────────────────────────────────────────────────
        float ampMultiplier = std::max(0.0f, std::min(1.0f, (1.0f + ampMod) * 0.5f));

        // STEP 3: Apply envelope, velocity, and amplitude modulation
        float sampleLeft = dcFreeLeft * envAmp * mVelocity * ampMultiplier;
        float sampleRight = dcFreeRight * envAmp * mVelocity * ampMultiplier;

        // ─────────────────────────────────────────────────────────────────────────
        // PER-VOICE SOFT CLIPPING
        // ─────────────────────────────────────────────────────────────────────────
        // Critical for preventing polyphonic overload. Without this, each voice
        // can output 2.0+ (two oscillators at full level), and 32 voices would
        // create 64.0+ amplitude before kPolyScale - impossible to limit cleanly.
        //
        // Using fastTanh for smooth, musical saturation:
        //   - tanh(1.0) ≈ 0.76 (gentle compression)
        //   - tanh(2.0) ≈ 0.96 (heavy compression)
        //   - tanh(3.0) ≈ 0.995 (near-limit)
        //
        // This ensures each voice contributes max ~1.0 to the sum, so:
        //   - 32 voices × ~1.0 × kPolyScale(0.25) = ~8.0 (limiter + soft clip handle it)
        // ─────────────────────────────────────────────────────────────────────────
        sampleLeft = fastTanh(sampleLeft);
        sampleRight = fastTanh(sampleRight);

        // ─────────────────────────────────────────────────────────────────────────
        // SUB DIRECT OUT - Bypass filter for maximum low-end impact (Serum-style)
        // ─────────────────────────────────────────────────────────────────────────
        // When Direct Out is enabled, the sub is added here AFTER the filter,
        // envelope, and soft clipping. This ensures:
        //   - Consistent bass regardless of filter sweeps
        //   - Sub isn't affected by filter resonance
        //   - Clean low-end foundation for club/PA systems
        //
        // The sub still follows note on/off (via voice lifetime) but bypasses
        // all timbral processing. Level is controlled independently.
        //
        // NOTE: We apply envelope to direct out sub so it fades naturally with
        // the note. Without this, the sub would cut off abruptly on note release.
        // ─────────────────────────────────────────────────────────────────────────
        if (mSubOscDirectOut)
        {
          // Apply envelope so sub fades with note (but not filter/effects)
          float directSubLeft = subLeft * envAmp * mVelocity;
          float directSubRight = subRight * envAmp * mVelocity;
          // Soft clip the direct sub too (prevents clipping when combined)
          sampleLeft += fastTanh(directSubLeft);
          sampleRight += fastTanh(directSubRight);
        }

        // ─────────────────────────────────────────────────────────────────────────
        // VOICE STEALING CROSSFADE
        // ─────────────────────────────────────────────────────────────────────────
        // When a voice is being force-killed, apply fade-out gain to prevent clicks.
        // The fade happens over ~1ms (48 samples at 48kHz).
        // ─────────────────────────────────────────────────────────────────────────
        if (mStealFadeCounter > 0)
        {
          sampleLeft *= mStealFadeGain;
          sampleRight *= mStealFadeGain;

          // Update fade for next sample (sample-rate independent)
          mStealFadeGain -= mStealFadeDecrement;
          if (mStealFadeGain < 0.0f) mStealFadeGain = 0.0f;
          mStealFadeCounter--;

          // When fade complete, deactivate the voice
          if (mStealFadeCounter == 0)
          {
            mActive = false;
            mForceRecycle.store(true, std::memory_order_release);
            return;  // Voice is done, stop processing
          }
        }

        // ─────────────────────────────────────────────────────────────────────────
        // VOICE OUTPUT SANITIZATION
        // ─────────────────────────────────────────────────────────────────────────
        // Final protection: sanitize the output sample before accumulation.
        // If any upstream calculation produced NaN/Infinity (filter instability,
        // division by zero, etc.), this prevents it from corrupting the output
        // buffer and all subsequent voices.
        // ─────────────────────────────────────────────────────────────────────────
        sampleLeft = sanitizeAudio(sampleLeft);
        sampleRight = sanitizeAudio(sampleRight);

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
      // NOTE: LFO sample rates are set globally in PluginInstanceDSP::Reset()

      // Pulse width smoothing: ~5ms for responsive but click-free modulation
      mPulseWidthSmoothCoeff = calcSmoothingCoeff(0.005f, static_cast<float>(sampleRate));

      // FM parameter smoothing: ~5ms for smooth ratio/depth changes
      mFMSmoothCoeff = calcSmoothingCoeff(0.005f, static_cast<float>(sampleRate));

      // Update DC blocker cutoff for new sample rate
      // 10Hz is below audible range but above subsonic rumble from speakers
      mDCBlockerL.cutoff(10_Hz, static_cast<float>(sampleRate));
      mDCBlockerR.cutoff(10_Hz, static_cast<float>(sampleRate));

      // Recalculate glide samples for new sample rate
      RecalculateGlideSamples();
    }

    // Parameter setters called from DSP class
    void SetWaveform(int waveform) { mWaveform = waveform; }
    void SetWavetablePosition(float pos) { mWavetablePosition = pos; mWavetableOsc.SetPosition(pos); }

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

    // ─────────────────────────────────────────────────────────────────────────
    // FILTER ENVELOPE SETTERS
    // ─────────────────────────────────────────────────────────────────────────
    void SetFilterEnvAttack(float ms)
    {
      mBaseFilterAttackMs = ms;
      mFilterEnvConfig.attack_rate = q::duration{ms * 0.001f};
    }

    void SetFilterEnvDecay(float ms)
    {
      mBaseFilterDecayMs = ms;
      mFilterEnvConfig.decay_rate = q::duration{ms * 0.001f};
    }

    void SetFilterEnvSustain(float level)
    {
      // Clamp to tiny positive value to avoid -infinity dB from lin_to_db(0)
      level = std::max(0.001f, level);
      mFilterEnvConfig.sustain_level = q::lin_to_db(level);
    }

    void SetFilterEnvRelease(float ms)
    {
      mBaseFilterReleaseMs = ms;
      mFilterEnvConfig.release_rate = q::duration{ms * 0.001f};
    }

    void SetFilterEnvDepth(float depth) { mFilterEnvDepth = depth; }

    // Filter setters (stereo - both L/R filters)
    // NOTE: Cutoff is stored as base value and modulated by LFO in ProcessSamplesAccumulating
    void SetFilterCutoff(float freqHz)
    {
      mFilterCutoffBase = freqHz;
      // Recalculate cached modulation limits (saves log2 calls in inner loop)
      float safeCutoff = std::max(20.0f, freqHz);
      mFilterModMax = std::log2(20000.0f / safeCutoff);
      mFilterModMin = std::log2(20.0f / safeCutoff);
    }
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
    void SetOsc1Pan(float pan) { mOsc1PanTarget = pan; }               // -1.0 to +1.0 (direct, no smoothing)

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
    void SetOsc2Pan(float pan) { mOsc2PanTarget = pan; }                   // -1.0 to +1.0 (direct, no smoothing)

    // ─────────────────────────────────────────────────────────────────────────
    // SUB OSCILLATOR SETTERS (Serum-style)
    // Simple waveform 1-3 octaves below for bass foundation.
    // Level and pan use target values - smoothing applied in ProcessSamplesAccumulating.
    // Direct Out bypasses filter+FX for maximum low-end impact.
    // ─────────────────────────────────────────────────────────────────────────
    void SetSubOscEnable(bool enable) { mSubOscEnable = enable; }
    void SetSubOscWaveform(int waveform) { mSubOscWaveform = waveform; }   // 0=Sine,1=Tri,2=Saw,3=Sq
    void SetSubOscOctave(int octave) { mSubOscOctave = octave; }           // -1, -2, or -3 octaves
    void SetSubOscLevel(float level) { mSubOscLevelTarget = level; }       // 0.0-1.0 (smoothed)
    void SetSubOscPan(float pan) { mSubOscPanTarget = pan; }               // -1.0 to +1.0 (smoothed)
    void SetSubOscDirectOut(bool direct) { mSubOscDirectOut = direct; }   // true = bypass filter+FX

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
    // PORTAMENTO (GLIDE) TIME SETTER
    // ─────────────────────────────────────────────────────────────────────────
    // Sets the glide time in milliseconds. Converts to sample count based on
    // current sample rate. The actual per-sample step is calculated in
    // SetPitchFromMidi when we know the pitch distance.
    //
    // We cache mGlideTimeMs so RecalculateGlideSamples() can recompute
    // mGlideTimeSamples if the sample rate changes.
    // ─────────────────────────────────────────────────────────────────────────
    void SetPortamentoTime(float timeMs)
    {
      mGlideTimeMs = timeMs;  // Cache for sample rate changes

      if (timeMs < 1.0f)
      {
        // Glide disabled - instant pitch change
        mGlideTimeSamples = 0;
        mGlideSamplesRemaining = 0;
        mGlideStepPerSample = 0.0;
      }
      else
      {
        // Calculate sample count for the glide duration
        // samples = timeMs × sampleRate / 1000
        mGlideTimeSamples = static_cast<int>((timeMs * 0.001f) * mSampleRate);
        if (mGlideTimeSamples < 1) mGlideTimeSamples = 1;
      }
    }

    // ─────────────────────────────────────────────────────────────────────────
    // RECALCULATE GLIDE SAMPLES (for sample rate changes)
    // ─────────────────────────────────────────────────────────────────────────
    void RecalculateGlideSamples()
    {
      if (mGlideTimeMs >= 1.0f)
      {
        mGlideTimeSamples = static_cast<int>((mGlideTimeMs * 0.001f) * mSampleRate);
        if (mGlideTimeSamples < 1) mGlideTimeSamples = 1;
      }
    }

    // ─────────────────────────────────────────────────────────────────────────
    // LEGATO PITCH UPDATE
    // Updates the target pitch for legato playing without retriggering envelope.
    // Converts MIDI note number to iPlug2's 1V/octave format.
    // The glide system will smoothly transition to the new pitch.
    //
    // Target pitch is stored in mInputs[kVoiceControlPitch].endValue
    // (single source of truth - no separate mTargetPitch variable).
    // ─────────────────────────────────────────────────────────────────────────
    void SetPitchFromMidi(int midiNote)
    {
      // Convert MIDI note to iPlug2's 1V/octave format:
      // MIDI 69 (A4) = pitch 0 → 440Hz
      // Each semitone = 1/12 octave
      double targetPitch = (midiNote - 69) / 12.0;
      mInputs[kVoiceControlPitch].endValue = targetPitch;  // Single source of truth

      // Case 1: First pitch (uninitialized) - snap immediately, no glide
      if (mCurrentPitch < kPitchInitThreshold)
      {
        mCurrentPitch = targetPitch;
        mGlideSamplesRemaining = 0;
        return;
      }

      // Case 2: Glide disabled - snap immediately
      if (mGlideTimeSamples <= 0)
      {
        mCurrentPitch = targetPitch;
        mGlideSamplesRemaining = 0;
        return;
      }

      // Case 3: Check if pitch distance is worth gliding
      double pitchDistance = targetPitch - mCurrentPitch;
      if (std::abs(pitchDistance) < kMinGlideDistance)
      {
        // Same note or tiny interval - skip glide (optimization)
        mCurrentPitch = targetPitch;
        mGlideSamplesRemaining = 0;
        return;
      }

      // Case 4: Start linear glide
      mGlideStepPerSample = pitchDistance / static_cast<double>(mGlideTimeSamples);
      mGlideSamplesRemaining = mGlideTimeSamples;
    }

    // Get current velocity (for retriggering with same velocity)
    float GetVelocity() const { return mVelocity; }

    // NOTE: LFO setters have been moved to PluginInstanceDSP (global LFOs).
    // Voice accesses LFO values via mParent->mLFO1Buffer[] etc.

  private:
    // Parent DSP class pointer for accessing global LFO buffers
    PluginInstanceDSP* mParent = nullptr;

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
    float mOsc1PanTarget = 0.0f;          // Pan position: -1.0 (left) to +1.0 (right)

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

    // ─────────────────────────────────────────────────────────────────────────
    // FILTER ENVELOPE - Modulates filter cutoff over note lifetime
    // Separate from amp envelope for independent control of filter sweep.
    // Depth controls direction and amount: +100% opens filter, -100% closes it.
    // ─────────────────────────────────────────────────────────────────────────
    q::adsr_envelope_gen mFilterEnv{q::adsr_envelope_gen::config{}, 44100.0f};
    q::adsr_envelope_gen::config mFilterEnvConfig;
    float mBaseFilterAttackMs = 10.0f;    // Base filter env attack time
    float mBaseFilterDecayMs = 100.0f;    // Base filter env decay time
    float mBaseFilterReleaseMs = 200.0f;  // Base filter env release time
    float mFilterEnvDepth = 0.0f;         // ±1.0 = ±4 octaves of filter modulation

    WavetableOscillator mWavetableOsc;  // Wavetable oscillator (mipmapped, morphable)
    ResonantFilter mFilterL;             // Left channel filter (stereo processing)
    ResonantFilter mFilterR;             // Right channel filter (stereo processing)
    float mFilterCutoffBase = 10000.0f;  // Base filter cutoff from knob (before LFO modulation)
    // Cached filter modulation limits - only recalculated when cutoff changes
    // Saves 2× log2() calls per sample per voice (~2.8M calls/sec saved)
    float mFilterModMax = std::log2(20000.0f / 10000.0f);   // +1 octave at 10kHz default
    float mFilterModMin = std::log2(20.0f / 10000.0f);      // -8.96 octaves at 10kHz default

    // NOTE: LFOs are now GLOBAL (in PluginInstanceDSP) instead of per-voice.
    // This ensures all voices modulate in sync (Serum/Vital-style behavior).
    // Voices access LFO values via mParent->mLFO1Buffer[] and mParent->mLFO2Buffer[].

    // ─────────────────────────────────────────────────────────────────────────
    // DC BLOCKER - Q Library Implementation
    // ─────────────────────────────────────────────────────────────────────────
    // Uses Q library's dc_block class based on Julius O. Smith's algorithm.
    // High-pass at ~10Hz removes DC offset from FM/wavetable/filter.
    // Separate instances for L/R to preserve stereo processing state.
    // The q::dc_block handles sample rate in its constructor.
    // ─────────────────────────────────────────────────────────────────────────
    q::dc_block mDCBlockerL{10_Hz, 48000.0f};  // Left channel DC blocker
    q::dc_block mDCBlockerR{10_Hz, 48000.0f};  // Right channel DC blocker
    double mSampleRate = 44100.0;
    float mVelocity = 0.0f;              // Current smoothed velocity (0-1)
    float mTargetVelocity = 0.0f;        // Target velocity to smooth towards
    float mVelocitySmoothCoeff = 0.0f;   // Smoothing coefficient (~5ms)
    bool mActive = false;                // Voice is currently sounding
    int mWaveform = kWaveformSine;       // Current waveform selection

    // ─────────────────────────────────────────────────────────────────────────
    // SMART VOICE STEALING - Prioritize releasing voices over held voices
    // When all voices are busy, we prefer to steal voices that are already
    // in their release phase (note-off received) rather than voices that are
    // still being held. This prevents held notes from being cut off.
    //
    // THREAD SAFETY: Using std::atomic because MIDI messages can potentially
    // arrive from different threads depending on the host. While iPlug2
    // typically processes MIDI on the audio thread, some hosts may call
    // ProcessMidiMsg from a separate MIDI thread. Atomic operations ensure
    // safe concurrent access with minimal performance overhead.
    // ─────────────────────────────────────────────────────────────────────────
    std::atomic<bool> mIsReleasing{false};   // True after Release() called (note-off)
    std::atomic<bool> mForceRecycle{false};  // When true, GetBusy() returns false

    // ─────────────────────────────────────────────────────────────────────────
    // RETRIGGER SMOOTHING
    // When a voice is retriggered while still sounding, we crossfade from
    // the previous amplitude to avoid clicks. mRetriggerOffset holds the
    // level to fade from, mRetriggerDecay controls the ~5ms fade speed.
    // ─────────────────────────────────────────────────────────────────────────
    float mRetriggerOffset = 0.0f;
    float mRetriggerDecay = 1.0f;

    // ─────────────────────────────────────────────────────────────────────────
    // VOICE STEALING CROSSFADE
    // When a voice is force-killed due to voice cap, we fade out over ~1ms
    // to prevent clicks. mStealFadeCounter counts down samples, mStealFadeGain
    // is the current fade multiplier (1.0 → 0.0).
    // mStealFadeDecrement is sample-rate dependent (set in StartStealFade).
    // ─────────────────────────────────────────────────────────────────────────
    int mStealFadeCounter = 0;
    float mStealFadeGain = 1.0f;
    float mStealFadeDecrement = 1.0f / 48.0f;  // Default for 48kHz, recalculated in StartStealFade

    // ─────────────────────────────────────────────────────────────────────────
    // DYNAMIC RELEASE SCALING
    // When voice count is high, release time is sped up to prevent voice explosion.
    // Multiplier > 1.0 means faster release (e.g., 2.0 = release at 2x speed).
    // ─────────────────────────────────────────────────────────────────────────
    float mReleaseSpeedMultiplier = 1.0f;

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
    float mOsc2PanTarget = 0.0f;            // Pan position: -1.0 (left) to +1.0 (right)

    // OSC2 INDEPENDENT PARAMETERS (Serum-style full independence)
    // Each oscillator has its own waveform-specific controls for maximum flexibility.
    // ─────────────────────────────────────────────────────────────────────────
    float mWavetablePosition = 0.0f;        // Osc1 wavetable morph position (0.0-1.0)
    float mOsc2MorphPosition = 0.0f;        // Osc2 wavetable morph position (0.0-1.0)
    float mOsc2PulseWidth = 0.5f;           // Pulse width (0.05-0.95), 0.5 = square
    float mOsc2FMRatio = 2.0f;              // FM frequency ratio (carrier/modulator)
    float mOsc2FMFine = 0.0f;               // FM fine ratio offset (-0.5 to +0.5)
    float mOsc2FMDepth = 0.5f;              // FM modulation depth (0.0-1.0)

    // ─────────────────────────────────────────────────────────────────────────
    // SUB OSCILLATOR STATE - Simple waveform for bass foundation (Serum-style)
    // ─────────────────────────────────────────────────────────────────────────
    // The sub oscillator provides a clean low-end foundation with:
    //   - 4 waveforms: Sine, Triangle, Saw, Square
    //   - Octave offset: -1, -2, or -3 octaves below played note
    //   - Direct Out option: bypass filter+FX for maximum low-end impact
    //   - No unison, no FM - intentionally simple and CPU-efficient
    //
    // SMOOTHING PATTERN:
    // Level and pan use a "target + smoothed" pattern for click-free automation:
    //   - *Target: Set immediately when parameter changes (from UI/automation)
    //   - *Smoothed: Approaches target gradually via one-pole lowpass filter
    //   - Audio code reads *Smoothed, never *Target directly
    //
    // SOFT ENABLE/DISABLE:
    // When sub is disabled, we don't stop smoothing - we set effectiveTarget=0
    // and let level fade to zero. This prevents clicks when toggling on/off.
    // Smoothing runs EVERY sample, even when sub is "off".
    //
    // DIRECT OUT:
    // When enabled, sub output bypasses the filter and effects chain entirely.
    // This is essential for club/PA systems where you want consistent sub bass
    // regardless of filter sweeps or effects processing.
    // ─────────────────────────────────────────────────────────────────────────
    q::phase_iterator mSubOscPhase;         // Phase accumulator (Q library)
    bool mSubOscEnable = false;             // On/off toggle (soft fade via smoothing)
    int mSubOscWaveform = 0;                // 0=Sine, 1=Triangle, 2=Saw, 3=Square
    int mSubOscOctave = -1;                 // Octave offset: -1, -2, or -3
    float mSubOscLevelTarget = 0.5f;        // Level target (0.0-1.0, set by UI)
    float mSubOscLevelSmoothed = 0.0f;      // Level smoothed (starts at 0 for soft start)
    float mSubOscPanTarget = 0.0f;          // Pan target (-1.0 left to +1.0 right)
    float mSubOscPanSmoothed = 0.0f;        // Pan smoothed (used in audio)
    bool mSubOscDirectOut = false;          // Bypass filter+FX when true

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
    // IMPORTANT: We glide in the PITCH domain (1V/octave), NOT frequency domain!
    // This ensures equal glide time across all octaves (musically correct).
    //   - Gliding C2→C3 takes same time as C5→C6 (both 1 octave)
    //   - Frequency domain would make high notes take much longer
    //
    // LINEAR INTERPOLATION GLIDE (predictable, exact timing):
    // - mGlideTimeSamples: Total samples for a complete glide (from parameter)
    // - mGlideSamplesRemaining: Countdown of samples left in current glide
    // - mGlideStepPerSample: Pitch delta per sample (calculated when glide starts)
    // - mCurrentPitch: Current interpolating pitch in 1V/octave format
    //
    // Target pitch is stored in mInputs[kVoiceControlPitch].endValue (single source of truth)
    // ─────────────────────────────────────────────────────────────────────────

    // Glide constants
    static constexpr double kPitchUninitialized = -999.0;     // Sentinel: pitch not yet set
    static constexpr double kPitchInitThreshold = -100.0;     // Below this = uninitialized
    static constexpr double kMinGlideDistance = 0.0008;       // ~1 cent - skip glide if smaller

    // Glide state (linear interpolation)
    int mGlideTimeSamples = 0;              // Total samples for glide (from parameter, 0 = instant)
    int mGlideSamplesRemaining = 0;         // Samples left in current glide (countdown)
    double mGlideStepPerSample = 0.0;       // Pitch increment per sample
    double mCurrentPitch = kPitchUninitialized;  // Current gliding pitch (1V/oct)
    float mGlideTimeMs = 0.0f;              // Cached glide time for sample rate changes
  };

public:
#pragma mark - DSP
  // ─────────────────────────────────────────────────────────────────────────────
  // VOICE COUNT - 32 voices for professional polyphony (matches Vital)
  // ─────────────────────────────────────────────────────────────────────────────
  // iPlug2's voice allocator steals the OLDEST voice when all are busy, regardless
  // of whether it's still held or in release. With only 8 voices, complex chords
  // + rapid playing causes held notes to be stolen.
  //
  // 32 voices provides comfortable headroom for:
  //   - Complex chords (left hand) + aggressive playing (right hand)
  //   - Sustain pedal usage with overlapping notes
  //   - Long release tails without cutting off held notes
  //
  // VOICE COUNT RATIONALE:
  // - Serum: 16 voices default (configurable)
  // - Vital: 32 voices default
  // - Massive: 64 voice cap
  //
  // We use 32 voices (matching Vital) because:
  // - Aggressive playing (smashing chords) can easily need 20+ voices
  // - Release tails overlap with new notes
  // - Smart voice stealing handles edge cases (steals releasing voices first)
  // - Modern CPUs handle 32 voices easily
  // ─────────────────────────────────────────────────────────────────────────────
  PluginInstanceDSP(int nVoices = 32)
  {
    // Get pointer to static wavetable (no copy!)
    mWavetable = &WavetableGenerator::GenerateBasicShapes();

    for (int i = 0; i < nVoices; i++)
    {
      Voice* voice = new Voice();
      voice->SetParent(this);           // Set parent for global LFO access
      voice->SetWavetable(mWavetable);  // Share wavetable data
      mSynth.AddVoice(voice, 0);
      mVoices.push_back(voice);  // Track for cleanup
    }
  }

  // ─────────────────────────────────────────────────────────────────────────────
  // DESTRUCTOR - Clean up dynamically allocated voices
  // ─────────────────────────────────────────────────────────────────────────────
  // IMPORTANT: iPlug2's MidiSynth/VoiceAllocator do NOT delete voices.
  // We must delete the Voice objects we created with new.
  // Without this destructor, each plugin instance leaks 32 × sizeof(Voice) bytes.
  // ─────────────────────────────────────────────────────────────────────────────
  ~PluginInstanceDSP()
  {
    for (Voice* voice : mVoices)
    {
      delete voice;
    }
    mVoices.clear();
  }

  void ProcessBlock(T** inputs, T** outputs, int nOutputs, int nFrames)
  {
    // ─────────────────────────────────────────────────────────────────────────
    // DENORMAL PROTECTION
    // ─────────────────────────────────────────────────────────────────────────
    // Enable FTZ/DAZ for this block to prevent CPU spikes from denormals.
    // The guard automatically restores flags when ProcessBlock returns.
    // ─────────────────────────────────────────────────────────────────────────
    DenormalGuard denormalGuard;

    // Safety check: clamp block size to buffer limit to prevent overflow
    if (nFrames > kMaxBlockSize) nFrames = kMaxBlockSize;

    // Reset mono voice processing flag for this block
    mMonoVoiceProcessedThisBlock = false;

    // Clear outputs first
    for (int c = 0; c < nOutputs; c++)
    {
      memset(outputs[c], 0, nFrames * sizeof(T));
    }

    // ─────────────────────────────────────────────────────────────────────────
    // PROCESS LFO RETRIGGER FLAGS (Thread-Safe)
    // Check atomic flags set by ProcessMidiMsg and reset LFOs if needed.
    // Using exchange() ensures we clear the flag atomically.
    // ─────────────────────────────────────────────────────────────────────────
    if (mLFO1NeedsReset.exchange(false, std::memory_order_acquire))
    {
      mGlobalLFO1.Reset();
    }
    if (mLFO2NeedsReset.exchange(false, std::memory_order_acquire))
    {
      mGlobalLFO2.Reset();
    }

    // ─────────────────────────────────────────────────────────────────────────
    // PRE-COMPUTE GLOBAL LFO BUFFERS
    // Compute LFO values for the entire block BEFORE voice processing.
    // This ensures all voices see the same LFO phase, achieving Serum/Vital-style
    // global LFO behavior where all notes in a chord modulate in sync.
    //
    // BYPASS OPTIMIZATION: Skip LFO processing entirely when destination is Off.
    // This saves CPU when LFOs aren't being used (common for simple patches).
    //
    // IMPORTANT: We clamp the final values to [-1.0, +1.0] to prevent the Low/High
    // settings from exceeding the intended modulation depth. Without clamping:
    //   - Low=-200%, High=+200% would give ±200% modulation
    //   - Filter would get ±8 octaves instead of ±4
    //   - Pitch would get ±48 semitones instead of ±24
    // ─────────────────────────────────────────────────────────────────────────

    // LFO1: Process or bypass based on Enable flag and destination
    // Bypass when: disabled OR destination is Off
    if (!mLFO1Enable || mLFO1Destination == LFODestination::Off)
    {
      // Bypass: fill buffer with 0 (no modulation)
      std::fill_n(mLFO1Buffer, nFrames, 0.0f);
    }
    else
    {
      for (int s = 0; s < nFrames; s++)
      {
        float lfo1Raw = mGlobalLFO1.Process();
        float lfo1Normalized = (lfo1Raw + 1.0f) * 0.5f;  // [-1,+1] → [0,1]
        float lfo1Mapped = mLFO1Low + lfo1Normalized * (mLFO1High - mLFO1Low);
        mLFO1Buffer[s] = std::max(-1.0f, std::min(1.0f, lfo1Mapped));
      }
    }

    // LFO2: Process or bypass based on Enable flag and destination
    // Bypass when: disabled OR destination is Off
    if (!mLFO2Enable || mLFO2Destination == LFODestination::Off)
    {
      // Bypass: fill buffer with 0 (no modulation)
      std::fill_n(mLFO2Buffer, nFrames, 0.0f);
    }
    else
    {
      for (int s = 0; s < nFrames; s++)
      {
        float lfo2Raw = mGlobalLFO2.Process();
        float lfo2Normalized = (lfo2Raw + 1.0f) * 0.5f;  // [-1,+1] → [0,1]
        float lfo2Mapped = mLFO2Low + lfo2Normalized * (mLFO2High - mLFO2Low);
        mLFO2Buffer[s] = std::max(-1.0f, std::min(1.0f, lfo2Mapped));
      }
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // VOICE MANAGEMENT - Prevent voice explosion from unison + long release
    // ═══════════════════════════════════════════════════════════════════════════
    //
    // ╔═══════════════════════════════════════════════════════════════════════════╗
    // ║  ⚠️  CRITICAL: REAL-TIME AUDIO THREAD - NO MEMORY ALLOCATION ALLOWED  ⚠️  ║
    // ╠═══════════════════════════════════════════════════════════════════════════╣
    // ║  This code runs in ProcessBlock() which has a strict deadline (~1-5ms).   ║
    // ║  Memory allocation (new, malloc, std::vector, std::string, etc.) can      ║
    // ║  take unpredictable time and WILL cause audio glitches.                   ║
    // ║                                                                           ║
    // ║  FORBIDDEN in audio thread:                                               ║
    // ║    ✗ std::vector, std::map, std::string (they allocate on heap)           ║
    // ║    ✗ new, delete, malloc, free                                            ║
    // ║    ✗ std::mutex, locks (can block indefinitely)                           ║
    // ║    ✗ File I/O, console output (printf, cout)                              ║
    // ║                                                                           ║
    // ║  SAFE alternatives:                                                       ║
    // ║    ✓ Fixed-size arrays on stack (like below)                              ║
    // ║    ✓ Pre-allocated buffers (allocated in constructor)                     ║
    // ║    ✓ std::array<T, N> (stack allocated, fixed size)                       ║
    // ║    ✓ Atomic operations for thread communication                           ║
    // ╚═══════════════════════════════════════════════════════════════════════════╝
    //
    // ═══════════════════════════════════════════════════════════════════════════
    {
      // Fixed-size stack arrays - NEVER use std::vector here!
      constexpr int kMaxVoices = 32;
      Voice* releasingVoices[kMaxVoices];
      float releasingLevels[kMaxVoices];
      int releasingCount = 0;
      int activeCount = 0;

      for (Voice* voice : mVoices)
      {
        if (voice->GetBusy() || voice->IsBeingStolen())
        {
          activeCount++;
          if (voice->IsReleasingCandidate() && !voice->IsBeingStolen() && releasingCount < kMaxVoices)
          {
            releasingVoices[releasingCount] = voice;
            releasingLevels[releasingCount] = voice->GetEnvelopeLevel();
            releasingCount++;
          }
        }
      }

      // Store for UI feedback
      mActiveVoiceCount = activeCount;

      // ─────────────────────────────────────────────────────────────────────────
      // DYNAMIC RELEASE SCALING (MORE AGGRESSIVE)
      // Speed up release phase when voices are active to prevent buildup.
      //   - 0-8 voices: normal speed (1.0x)
      //   - 8-12 voices: 2x speed
      //   - 12-16 voices: 4x speed
      //   - 16-20 voices: 8x speed
      //   - 20+ voices: 16x speed (very aggressive)
      // ─────────────────────────────────────────────────────────────────────────
      float releaseMultiplier = 1.0f;
      if (activeCount > 20) releaseMultiplier = 16.0f;
      else if (activeCount > 16) releaseMultiplier = 8.0f;
      else if (activeCount > 12) releaseMultiplier = 4.0f;
      else if (activeCount > 8) releaseMultiplier = 2.0f;

      for (Voice* voice : mVoices)
      {
        voice->SetReleaseSpeedMultiplier(releaseMultiplier);
      }

      // ─────────────────────────────────────────────────────────────────────────
      // HARD VOICE CAP - Kill quietest releasing voices if over limit
      // Uses simple selection sort (fast for small N, no allocation).
      // Much lower cap (16) to prevent glitching.
      // ─────────────────────────────────────────────────────────────────────────
      constexpr int kHardVoiceCap = 16;  // Lower cap for safety
      if (activeCount > kHardVoiceCap && releasingCount > 0)
      {
        int voicesToKill = activeCount - kHardVoiceCap;
        voicesToKill = std::min(voicesToKill, releasingCount);

        // Simple selection: find and kill the N quietest voices
        for (int k = 0; k < voicesToKill; k++)
        {
          // Find the quietest remaining voice
          int quietestIdx = -1;
          float quietestLevel = 2.0f;  // > max possible envelope

          for (int i = 0; i < releasingCount; i++)
          {
            if (releasingVoices[i] != nullptr && releasingLevels[i] < quietestLevel)
            {
              quietestLevel = releasingLevels[i];
              quietestIdx = i;
            }
          }

          if (quietestIdx >= 0)
          {
            releasingVoices[quietestIdx]->StartStealFade();
            releasingVoices[quietestIdx] = nullptr;  // Mark as processed
          }
        }
      }
    }

    // MidiSynth processes MIDI queue and calls voice ProcessSamplesAccumulating
    mSynth.ProcessBlock(inputs, outputs, 0, nOutputs, nFrames);

    // ─────────────────────────────────────────────────────────────────────────
    // MONO/LEGATO VOICE PROCESSING
    // ─────────────────────────────────────────────────────────────────────────
    // In mono/legato mode, we trigger voices directly (bypassing MIDI queue).
    // MidiSynth::ProcessBlock has a fast-path that skips processing when
    // mVoicesAreActive is false AND the queue is empty. Since we bypass the
    // queue, mVoicesAreActive may be false even though our mono voice is busy.
    // We must manually process the mono voice here.
    //
    // To prevent double processing (if MidiSynth DID process it), we use
    // mMonoVoiceProcessedThisBlock flag.
    // ─────────────────────────────────────────────────────────────────────────
    if (mVoiceMode != 0 && mMonoVoice != nullptr && mMonoVoice->GetBusy())
    {
      if (!mMonoVoiceProcessedThisBlock)
      {
        mMonoVoice->ProcessSamplesAccumulating(inputs, outputs, 0, nOutputs, 0, nFrames);
        mMonoVoiceProcessedThisBlock = true;
      }
    }

    // ─────────────────────────────────────────────────────────────────────────
    // MASTER GAIN WITH SMOOTHING (Serum-style approach)
    // ─────────────────────────────────────────────────────────────────────────
    // We use a one-pole lowpass filter to smooth gain changes, preventing
    // clicks when the user moves the gain knob.
    //
    // GAIN STAGING:
    //   1. Each voice has per-voice soft clipping (fastTanh) → max ~1.0 per voice
    //   2. Fixed headroom factor keeps signal manageable for limiter
    //   3. Post-accumulation soft clipping catches transient peaks
    //   4. Master limiter with makeup gain handles the rest
    //
    // WHY NOT 1/√N DYNAMIC COMPENSATION:
    //   - Musicians expect chords to be LOUDER than single notes
    //   - New notes should always sound at full volume, not get buried
    //   - Serum/Vital don't do this - they rely on limiting instead
    //   - Dynamic gain causes "pumping" artifacts when voice count changes
    // ─────────────────────────────────────────────────────────────────────────
    constexpr float kPolyScale = 0.25f;  // Single voice at -12dB, leaves headroom for polyphony

    for (int s = 0; s < nFrames; s++)
    {
      mGainSmoothed += mGainSmoothCoeff * (mGain - mGainSmoothed);

      float gainScale = kPolyScale * mGainSmoothed;

      for (int c = 0; c < nOutputs; c++)
      {
        outputs[c][s] *= gainScale;

        // ─────────────────────────────────────────────────────────────────────────
        // POST-ACCUMULATION SOFT CLIPPING
        // ─────────────────────────────────────────────────────────────────────────
        // Even with per-voice soft clipping and 1/√N gain compensation,
        // transients can still exceed the limiter's attack time (1ms).
        //
        // This soft clip catches accumulated peaks BEFORE the limiter,
        // ensuring the limiter sees a gentler signal it can handle smoothly.
        // ─────────────────────────────────────────────────────────────────────────
        float sample = static_cast<float>(outputs[c][s]);
        outputs[c][s] = static_cast<T>(fastTanh(sample));
      }
    }

    // ─────────────────────────────────────────────────────────────────────────
    // STEREO DELAY PROCESSING
    // ─────────────────────────────────────────────────────────────────────────
    // Process delay AFTER gain scaling to prevent feedback loop blowup.
    // The signal entering the delay is already at a controlled level, so
    // the feedback loop remains stable even at high feedback settings.
    //
    // BYPASS: When mDelayEnable is false, the entire loop is skipped.
    // Signal passes through unchanged (already in outputs buffer).
    // This provides true zero CPU overhead when delay is disabled.
    // ─────────────────────────────────────────────────────────────────────────
    if (nOutputs >= 2 && mDelayEnable)
    {
      for (int s = 0; s < nFrames; s++)
      {
        float left = static_cast<float>(outputs[0][s]);
        float right = static_cast<float>(outputs[1][s]);

        mDelay.Process(left, right);

        // Post-delay soft clipping: delay can add dry+wet exceeding 1.0
        // Must clip BEFORE limiter to prevent transients slipping through
        left = fastTanh(left);
        right = fastTanh(right);

        outputs[0][s] = static_cast<T>(left);
        outputs[1][s] = static_cast<T>(right);
      }
    }
    // When delay is disabled, signal passes through unchanged (already in outputs)

    // ─────────────────────────────────────────────────────────────────────────
    // REVERB EFFECT - Dattorro plate reverb
    // ─────────────────────────────────────────────────────────────────────────
    // Dattorro plate reverb provides professional-quality reverb with:
    //   - Figure-8 tank topology for dense, natural-sounding decay
    //   - Modulated allpass filters for chorus-like shimmer
    //   - Early reflections network for realistic room character
    //   - Configurable modes (Plate, Chamber, Hall, Cathedral)
    //
    // PROCESSING: Applied after delay effect, before final limiter.
    // Uses the same dry/wet mixing approach as the delay for consistency.
    //
    // BYPASS: When mReverbEnable is false, the entire loop is skipped.
    // Signal passes through unchanged (already in outputs buffer).
    // This provides true zero CPU overhead when reverb is disabled.
    // ─────────────────────────────────────────────────────────────────────────
    if (nOutputs >= 2 && mReverbEnable)
    {
      for (int s = 0; s < nFrames; s++)
      {
        float left = static_cast<float>(outputs[0][s]);
        float right = static_cast<float>(outputs[1][s]);

        mReverb.Process(left, right);

        // Post-reverb soft clipping: reverb can add dry+wet exceeding 1.0
        // Must clip BEFORE limiter to prevent transients slipping through
        left = fastTanh(left);
        right = fastTanh(right);

        outputs[0][s] = static_cast<T>(left);
        outputs[1][s] = static_cast<T>(right);
      }
    }
    // When reverb is disabled, signal passes through unchanged (already in outputs)

    // ─────────────────────────────────────────────────────────────────────────
    // OUTPUT LIMITER - Professional-grade brickwall limiting (Q library)
    // ─────────────────────────────────────────────────────────────────────────
    // Uses Q's soft_knee_compressor with envelope followers for transparent
    // limiting with proper attack/release behavior (no pumping artifacts).
    //
    // STEREO LINKING: Both channels share the same gain reduction computed
    // from the maximum envelope of both channels. This preserves stereo image.
    //
    // Two layers of protection:
    //   1. Soft-knee limiter with fast attack (0.1ms) - Catches transients
    //   2. Final fastTanh soft clipping - Musical saturation, no harsh clipping
    //   3. NaN/Inf check - Replace corrupt samples with silence
    // ─────────────────────────────────────────────────────────────────────────
    if (nOutputs >= 2)
    {
      for (int s = 0; s < nFrames; s++)
      {
        float left = static_cast<float>(outputs[0][s]);
        float right = static_cast<float>(outputs[1][s]);

        // Layer 1: Q library soft-knee limiter with attack/release
        // Track envelope of both channels with attack/release smoothing
        float envL = mLimiterEnvL(std::abs(left));
        float envR = mLimiterEnvR(std::abs(right));

        // Stereo link: use max envelope for shared gain reduction
        float maxEnv = std::max(envL, envR);

        // Only compute gain reduction if signal is above noise floor
        float gain = 1.0f;
        if (maxEnv > 0.001f)  // -60dB noise floor
        {
          // Convert to dB, apply compressor, convert back to linear
          q::decibel envDb = q::lin_to_db(maxEnv);
          q::decibel gainDb = mLimiter(envDb);
          gain = q::lin_float(gainDb);
        }

        // Apply stereo-linked gain reduction
        left *= gain;
        right *= gain;

        // Layer 2: Final soft clipping - musical saturation instead of harsh digital clipping
        // fastTanh smoothly limits to ~±1.0 without the harsh artifacts of hard clipping
        // No makeup gain needed - kPolyScale and limiting handle overall level
        left = fastTanh(left);
        right = fastTanh(right);

        // Layer 3: NaN/Inf protection
        if (isAudioCorrupt(left)) left = 0.0f;
        if (isAudioCorrupt(right)) right = 0.0f;

        outputs[0][s] = static_cast<T>(left);
        outputs[1][s] = static_cast<T>(right);
      }
    }
    else
    {
      // Mono fallback (rare)
      for (int s = 0; s < nFrames; s++)
      {
        float sample = static_cast<float>(outputs[0][s]);

        float env = mLimiterEnvL(std::abs(sample));
        float gain = 1.0f;
        if (env > 0.001f)
        {
          q::decibel envDb = q::lin_to_db(env);
          q::decibel gainDb = mLimiter(envDb);
          gain = q::lin_float(gainDb);
        }

        sample *= gain;

        // Final soft clipping (same as stereo path)
        sample = fastTanh(sample);
        if (isAudioCorrupt(sample)) sample = 0.0f;

        outputs[0][s] = static_cast<T>(sample);
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

    // Initialize global LFOs with the correct sample rate
    mGlobalLFO1.SetSampleRate(static_cast<float>(sampleRate));
    mGlobalLFO2.SetSampleRate(static_cast<float>(sampleRate));
    mGlobalLFO1.Reset();
    mGlobalLFO2.Reset();

    // Initialize delay effect
    mDelay.SetSampleRate(static_cast<float>(sampleRate));
    mDelay.Reset();

    // Initialize reverb effect
    mReverb.SetSampleRate(static_cast<float>(sampleRate));
    mReverb.Reset();

    // Initialize output limiter envelope followers with correct sample rate
    // Attack: 0.1ms (fast to catch transients), Release: 50ms for smooth recovery
    mLimiterEnvL.config(0.1_ms, 50_ms, static_cast<float>(sampleRate));
    mLimiterEnvR.config(0.1_ms, 50_ms, static_cast<float>(sampleRate));
    mLimiterEnvL = 0.0f;  // Reset envelope state
    mLimiterEnvR = 0.0f;
  }

  // ─────────────────────────────────────────────────────────────────────────────
  // VOICE COUNT ACCESSOR - For UI feedback
  // Returns the current number of active voices (0-32)
  // ─────────────────────────────────────────────────────────────────────────────
  int GetActiveVoiceCount() const { return mActiveVoiceCount; }

  void ProcessMidiMsg(const IMidiMsg& msg)
  {
    // ─────────────────────────────────────────────────────────────────────────
    // MONO/LEGATO MODE HANDLING
    // ─────────────────────────────────────────────────────────────────────────
    // In Mono/Legato modes, we handle voice allocation manually instead of
    // using iPlug2's polyphonic allocator:
    //
    // MONO MODE (mVoiceMode == 1):
    //   - Single voice always used
    //   - New notes retrigger envelope (percussive)
    //   - Note-off only releases if it's the current note
    //   - Glide slides pitch to new note
    //
    // LEGATO MODE (mVoiceMode == 2):
    //   - Single voice always used
    //   - Overlapping notes DON'T retrigger (smooth transitions)
    //   - Note-off only releases if no other notes held (requires note stack)
    //   - Glide slides pitch to new note
    //
    // POLY MODE (mVoiceMode == 0):
    //   - Standard polyphonic behavior via iPlug2's allocator
    // ─────────────────────────────────────────────────────────────────────────

    const int status = msg.StatusMsg();
    const int noteNum = msg.NoteNumber();
    const int velocity = msg.Velocity();

    // Handle Mono/Legato modes with direct voice control (bypass iPlug2's queue)
    // We use mVoices[0] as the dedicated mono voice for predictable behavior
    if (mVoiceMode != 0)  // Mono or Legato
    {
      if (status == IMidiMsg::kNoteOn && velocity > 0)
      {
        bool isLegato = (mVoiceMode == 2);
        bool isFirstNote = (mNoteStackSize == 0);

        // Add note to stack (if not already there and stack not full)
        bool noteAlreadyInStack = false;
        for (int i = 0; i < mNoteStackSize; i++)
        {
          if (mNoteStack[i] == noteNum)
          {
            noteAlreadyInStack = true;
            break;
          }
        }
        if (!noteAlreadyInStack && mNoteStackSize < kMaxNoteStackSize)
        {
          mNoteStack[mNoteStackSize++] = noteNum;
        }

        // Always use voice 0 for mono/legato
        mMonoVoice = mVoices[0];
        double velocityNorm = velocity / 127.0;

        // Set glide time (glide only if not first note and glide enabled)
        if (mGlideEnable && !isFirstNote)
        {
          mMonoVoice->SetPortamentoTime(mGlideTimeMs);
        }
        else
        {
          mMonoVoice->SetPortamentoTime(0.0f);  // Instant pitch change
        }

        // Set the target pitch
        mMonoVoice->SetPitchFromMidi(noteNum);

        if (isFirstNote)
        {
          // First note - trigger the voice (full attack)
          mMonoVoice->Trigger(velocityNorm, false);
        }
        else
        {
          // Subsequent note
          if (isLegato)
          {
            // LEGATO: Don't retrigger envelope, just pitch changes (already set above)
            // The glide will handle the smooth transition
          }
          else
          {
            // MONO: Retrigger envelope for punchy attack on each note
            mMonoVoice->Trigger(velocityNorm, true);  // isRetrigger=true for smooth transition
          }
        }

        // LFO retrigger
        if (mLFO1Retrigger)
          mLFO1NeedsReset.store(true, std::memory_order_release);
        if (mLFO2Retrigger)
          mLFO2NeedsReset.store(true, std::memory_order_release);

        return;  // Don't process through normal poly path
      }
      else if (status == IMidiMsg::kNoteOff || (status == IMidiMsg::kNoteOn && velocity == 0))
      {
        // Remove note from stack
        int noteIndex = -1;
        for (int i = 0; i < mNoteStackSize; i++)
        {
          if (mNoteStack[i] == noteNum)
          {
            noteIndex = i;
            break;
          }
        }

        if (noteIndex >= 0)
        {
          // Check if this was the top note BEFORE removing
          bool wasTopNote = (noteIndex == mNoteStackSize - 1);

          // Shift remaining notes down to remove this note
          for (int i = noteIndex; i < mNoteStackSize - 1; i++)
          {
            mNoteStack[i] = mNoteStack[i + 1];
          }
          mNoteStackSize--;

          if (mNoteStackSize == 0)
          {
            // No more notes held - release the voice
            if (mMonoVoice)
            {
              mMonoVoice->Release();
            }
          }
          else if (wasTopNote)
          {
            // Released top note but still have notes held - go back to previous
            int previousNote = mNoteStack[mNoteStackSize - 1];

            // Set glide for the return (if enabled)
            if (mGlideEnable && mMonoVoice)
            {
              mMonoVoice->SetPortamentoTime(mGlideTimeMs);
            }

            // Change pitch to previous note
            if (mMonoVoice)
            {
              mMonoVoice->SetPitchFromMidi(previousNote);
            }

            // In MONO mode, retrigger envelope when returning to previous note
            // In LEGATO mode, don't retrigger (smooth return)
            if (mVoiceMode == 1 && mMonoVoice)  // Mono
            {
              mMonoVoice->Trigger(mMonoVoice->GetVelocity(), true);
            }
          }
          // If released a note that wasn't on top, just remove from stack (done above)
        }

        return;
      }
      // Other MIDI messages (CC, pitch bend, etc.) pass through to poly handler
    }

    // ─────────────────────────────────────────────────────────────────────────
    // POLY MODE: SMART VOICE STEALING (Production-Quality Implementation)
    //
    // Before processing a note-on, check if we need to steal a voice.
    // Prefer stealing voices that are in release phase (note-off received)
    // over voices that are still being held. This prevents held notes from
    // being cut off when playing aggressively.
    //
    // ALGORITHM (same approach used by Serum, Vital, etc.):
    // 1. On note-on, count free voices
    // 2. If no free voices, find the BEST releasing voice to recycle:
    //    - Must be in release phase (note-off already received)
    //    - Prefer the voice with LOWEST envelope level (closest to silent)
    //    - This minimizes audible artifacts when stealing
    // 3. Mark that voice for recycling (GetBusy() returns false)
    // 4. iPlug2's allocator will then pick that voice as "free"
    //
    // THREAD SAFETY:
    // This function may be called from the MIDI thread while the audio thread
    // is processing. All voice state access uses std::atomic with proper
    // memory ordering to ensure thread-safe operation.
    // ─────────────────────────────────────────────────────────────────────────
    if (status == IMidiMsg::kNoteOn && velocity > 0)
    {
      int freeVoices = 0;
      Voice* bestStealCandidate = nullptr;
      float lowestEnvelopeLevel = 1.0f;  // Start with max, find lowest

      for (Voice* voice : mVoices)
      {
        if (!voice->GetBusy())
        {
          freeVoices++;
        }
        else if (voice->IsReleasingCandidate())
        {
          // Compare envelope levels - pick the voice closest to silent
          // Lower envelope = further into release = less audible when cut
          float envLevel = voice->GetEnvelopeLevel();
          if (envLevel < lowestEnvelopeLevel)
          {
            lowestEnvelopeLevel = envLevel;
            bestStealCandidate = voice;
          }
        }
      }

      // If no free voices but we have a releasing candidate, mark it for recycling
      if (freeVoices == 0 && bestStealCandidate)
      {
        bestStealCandidate->MarkForRecycle();
      }

      // ─────────────────────────────────────────────────────────────────────────
      // GLOBAL LFO RETRIGGER (Thread-Safe)
      // If retrigger mode is enabled, set a flag to reset LFO phase.
      // The actual reset happens in ProcessBlock to avoid race conditions,
      // since ProcessMidiMsg may be called from a different thread.
      // ─────────────────────────────────────────────────────────────────────────
      if (mLFO1Retrigger)
      {
        mLFO1NeedsReset.store(true, std::memory_order_release);
      }
      if (mLFO2Retrigger)
      {
        mLFO2NeedsReset.store(true, std::memory_order_release);
      }
    }

    mSynth.AddMidiMsgToQueue(msg);
  }

  // ─────────────────────────────────────────────────────────────────────────────
  // GLOBAL LFO SETTERS
  // LFOs are now global (Serum/Vital-style), shared across all voices.
  // ─────────────────────────────────────────────────────────────────────────────
  void SetTempo(float bpm)
  {
    mTempo = bpm;
    // Recalculate LFO rates if they're tempo-synced
    if (mLFO1SyncRate != LFOSyncRate::Off)
      mGlobalLFO1.SetRate(SyncRateToHz(mLFO1SyncRate, mTempo));
    if (mLFO2SyncRate != LFOSyncRate::Off)
      mGlobalLFO2.SetRate(SyncRateToHz(mLFO2SyncRate, mTempo));
    // Recalculate delay time if tempo-synced
    if (mDelaySyncRate != DelaySyncRate::Off)
      mDelay.SetDelayTime(DelaySyncRateToMs(mDelaySyncRate, mTempo));
  }

  // ─────────────────────────────────────────────────────────────────────────────
  // TRANSPORT STATE CHANGE HANDLING
  // ─────────────────────────────────────────────────────────────────────────────
  // When playback starts (especially mid-song with "chase" events), the delay
  // buffer and limiter envelopes may contain stale data from previous playback.
  // This causes noise/artifacts. We detect transport start and clear buffers.
  // ─────────────────────────────────────────────────────────────────────────────
  void SetTransportRunning(bool isRunning)
  {
    // Detect transport start (was stopped, now running)
    if (isRunning && !mTransportWasRunning)
    {
      // Clear delay buffer to prevent stale data from feeding back as noise
      mDelay.Reset();

      // Clear reverb buffer to prevent stale reverb tails from transport restart
      mReverb.Reset();

      // Reset limiter envelopes to prevent gain pumping on transport start
      mLimiterEnvL = 0.0f;
      mLimiterEnvR = 0.0f;
    }
    mTransportWasRunning = isRunning;
  }

  void SetLFO1Rate(float hz)
  {
    mLFO1FreeRate = hz;
    if (mLFO1SyncRate == LFOSyncRate::Off)
      mGlobalLFO1.SetRate(hz);
  }

  void SetLFO1Sync(int sync)
  {
    mLFO1SyncRate = static_cast<LFOSyncRate>(sync);
    if (mLFO1SyncRate == LFOSyncRate::Off)
      mGlobalLFO1.SetRate(mLFO1FreeRate);
    else
      mGlobalLFO1.SetRate(SyncRateToHz(mLFO1SyncRate, mTempo));
  }

  void SetLFO1Enable(bool enable) { mLFO1Enable = enable; }
  void SetLFO1Low(float low) { mLFO1Low = low / 100.0f; }      // Convert % to decimal
  void SetLFO1High(float high) { mLFO1High = high / 100.0f; }  // Convert % to decimal
  void SetLFO1Waveform(int waveform) { mGlobalLFO1.SetWaveform(static_cast<LFOWaveform>(waveform)); }
  void SetLFO1Retrigger(bool retrigger) { mLFO1Retrigger = retrigger; }
  void SetLFO1Destination(int dest) { mLFO1Destination = static_cast<LFODestination>(dest); }

  void SetLFO2Rate(float hz)
  {
    mLFO2FreeRate = hz;
    if (mLFO2SyncRate == LFOSyncRate::Off)
      mGlobalLFO2.SetRate(hz);
  }

  void SetLFO2Sync(int sync)
  {
    mLFO2SyncRate = static_cast<LFOSyncRate>(sync);
    if (mLFO2SyncRate == LFOSyncRate::Off)
      mGlobalLFO2.SetRate(mLFO2FreeRate);
    else
      mGlobalLFO2.SetRate(SyncRateToHz(mLFO2SyncRate, mTempo));
  }

  void SetLFO2Enable(bool enable) { mLFO2Enable = enable; }
  void SetLFO2Low(float low) { mLFO2Low = low / 100.0f; }      // Convert % to decimal
  void SetLFO2High(float high) { mLFO2High = high / 100.0f; }  // Convert % to decimal
  void SetLFO2Waveform(int waveform) { mGlobalLFO2.SetWaveform(static_cast<LFOWaveform>(waveform)); }
  void SetLFO2Retrigger(bool retrigger) { mLFO2Retrigger = retrigger; }
  void SetLFO2Destination(int dest) { mLFO2Destination = static_cast<LFODestination>(dest); }

  // Global bypass setters
  void SetFilterEnable(bool enable) { mFilterEnable = enable; }
  void SetDelayEnable(bool enable) { mDelayEnable = enable; }

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
      case kParamFilterEnable:
        SetFilterEnable(value > 0.5);
        break;

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

      // ─────────────────────────────────────────────────────────────────────────
      // FILTER ENVELOPE PARAMETERS
      // ─────────────────────────────────────────────────────────────────────────
      case kParamFilterEnvAttack:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          dynamic_cast<Voice&>(voice).SetFilterEnvAttack(static_cast<float>(value));
        });
        break;

      case kParamFilterEnvDecay:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          dynamic_cast<Voice&>(voice).SetFilterEnvDecay(static_cast<float>(value));
        });
        break;

      case kParamFilterEnvSustain:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          // Convert 0-100% to 0.0-1.0
          dynamic_cast<Voice&>(voice).SetFilterEnvSustain(static_cast<float>(value) / 100.0f);
        });
        break;

      case kParamFilterEnvRelease:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          dynamic_cast<Voice&>(voice).SetFilterEnvRelease(static_cast<float>(value));
        });
        break;

      case kParamFilterEnvDepth:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          // Convert -100 to +100 to -1.0 to +1.0
          dynamic_cast<Voice&>(voice).SetFilterEnvDepth(static_cast<float>(value) / 100.0f);
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

      case kParamOsc1Pan:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          // Convert -100% to +100% to -1.0 to +1.0
          dynamic_cast<Voice&>(voice).SetOsc1Pan(static_cast<float>(value / 100.0));
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

      case kParamOsc2Pan:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          // Convert -100% to +100% to -1.0 to +1.0
          dynamic_cast<Voice&>(voice).SetOsc2Pan(static_cast<float>(value / 100.0));
        });
        break;

      // ─────────────────────────────────────────────────────────────────────────
      // SUB OSCILLATOR PARAMETERS (Serum-style)
      // Simple waveform 1-3 octaves below for bass foundation.
      // Direct Out bypasses filter for maximum low-end impact.
      // ─────────────────────────────────────────────────────────────────────────
      case kParamSubOscEnable:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          dynamic_cast<Voice&>(voice).SetSubOscEnable(value > 0.5);
        });
        break;

      case kParamSubOscWaveform:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          // Enum index: 0=Sine, 1=Triangle, 2=Saw, 3=Square
          dynamic_cast<Voice&>(voice).SetSubOscWaveform(static_cast<int>(value));
        });
        break;

      case kParamSubOscOctave:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          // Enum index to octave: 0=-1, 1=-2, 2=-3
          static const int kOctaves[] = {-1, -2, -3};
          int idx = static_cast<int>(value);
          int octave = (idx >= 0 && idx < 3) ? kOctaves[idx] : -1;
          dynamic_cast<Voice&>(voice).SetSubOscOctave(octave);
        });
        break;

      case kParamSubOscLevel:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          // Convert 0-100% to 0.0-1.0
          dynamic_cast<Voice&>(voice).SetSubOscLevel(static_cast<float>(value / 100.0));
        });
        break;

      case kParamSubOscPan:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          // Convert -100% to +100% to -1.0 to +1.0
          dynamic_cast<Voice&>(voice).SetSubOscPan(static_cast<float>(value / 100.0));
        });
        break;

      case kParamSubOscDirectOut:
        mSynth.ForEachVoice([value](SynthVoice& voice) {
          // Direct Out: bypass filter+FX for maximum low-end impact
          dynamic_cast<Voice&>(voice).SetSubOscDirectOut(value > 0.5);
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

      // ─────────────────────────────────────────────────────────────────────────
      // LFO1 PARAMETERS (GLOBAL)
      // LFOs are now global - all voices share the same LFO phase (Serum/Vital-style)
      // ─────────────────────────────────────────────────────────────────────────
      case kParamLFO1Enable:
        SetLFO1Enable(value > 0.5);
        break;

      case kParamLFO1Rate:
        SetLFO1Rate(static_cast<float>(value));
        break;

      case kParamLFO1Low:
        SetLFO1Low(static_cast<float>(value));
        break;

      case kParamLFO1High:
        SetLFO1High(static_cast<float>(value));
        break;

      case kParamLFO1Waveform:
        SetLFO1Waveform(static_cast<int>(value));
        break;

      case kParamLFO1Retrigger:
        SetLFO1Retrigger(static_cast<int>(value) == 1);
        break;

      case kParamLFO1Destination:
        SetLFO1Destination(static_cast<int>(value));
        break;

      case kParamLFO1Sync:
        SetLFO1Sync(static_cast<int>(value));
        break;

      // ─────────────────────────────────────────────────────────────────────────
      // LFO2 PARAMETERS (GLOBAL)
      // ─────────────────────────────────────────────────────────────────────────
      case kParamLFO2Enable:
        SetLFO2Enable(value > 0.5);
        break;

      case kParamLFO2Rate:
        SetLFO2Rate(static_cast<float>(value));
        break;

      case kParamLFO2Low:
        SetLFO2Low(static_cast<float>(value));
        break;

      case kParamLFO2High:
        SetLFO2High(static_cast<float>(value));
        break;

      case kParamLFO2Waveform:
        SetLFO2Waveform(static_cast<int>(value));
        break;

      case kParamLFO2Retrigger:
        SetLFO2Retrigger(static_cast<int>(value) == 1);
        break;

      case kParamLFO2Destination:
        SetLFO2Destination(static_cast<int>(value));
        break;

      case kParamLFO2Sync:
        SetLFO2Sync(static_cast<int>(value));
        break;

      // ─────────────────────────────────────────────────────────────────────────
      // STEREO DELAY PARAMETERS
      // ─────────────────────────────────────────────────────────────────────────
      case kParamDelayEnable:
        SetDelayEnable(value > 0.5);
        break;

      case kParamDelayTime:
        mDelayTimeMs = static_cast<float>(value);
        // Only apply manual delay time if sync is off
        if (mDelaySyncRate == DelaySyncRate::Off)
        {
          mDelay.SetDelayTime(mDelayTimeMs);
        }
        break;

      case kParamDelaySync:
        mDelaySyncRate = static_cast<DelaySyncRate>(static_cast<int>(value));
        // Recalculate delay time based on sync setting
        if (mDelaySyncRate == DelaySyncRate::Off)
        {
          mDelay.SetDelayTime(mDelayTimeMs);
        }
        else
        {
          mDelay.SetDelayTime(DelaySyncRateToMs(mDelaySyncRate, mTempo));
        }
        break;

      case kParamDelayFeedback:
        // Convert 0-90% to 0.0-0.9
        mDelay.SetFeedback(static_cast<float>(value / 100.0));
        break;

      case kParamDelayDry:
        // Convert 0-100% to 0.0-1.0
        mDelay.SetDryLevel(static_cast<float>(value / 100.0));
        break;

      case kParamDelayWet:
        // Convert 0-100% to 0.0-1.0
        mDelay.SetWetLevel(static_cast<float>(value / 100.0));
        break;

      case kParamDelayMode:
        mDelay.SetMode(static_cast<DelayMode>(static_cast<int>(value)));
        break;

      // ─────────────────────────────────────────────────────────────────────────
      // REVERB PARAMETERS
      // ─────────────────────────────────────────────────────────────────────────
      case kParamReverbEnable:
        mReverbEnable = value > 0.5;
        break;

      case kParamReverbDecay:
        // Convert 0-100% to 0.0-1.0
        mReverb.SetDecay(static_cast<float>(value / 100.0));
        break;

      case kParamReverbSize:
        // Convert 0-100% to 0.0-1.0
        mReverb.SetSize(static_cast<float>(value / 100.0));
        break;

      case kParamReverbDamping:
        // Damping controls high-frequency rolloff via the high-cut filter
        // 0% = no damping (bright) = 20000 Hz, 100% = full damping (dark) = 1000 Hz
        {
          float damping = static_cast<float>(value / 100.0);
          float highCutHz = 20000.0f - damping * 19000.0f;  // 20000 -> 1000 Hz
          mReverb.SetHighCut(highCutHz);
        }
        break;

      case kParamReverbWidth:
        // Convert 0-100% to 0.0-1.0
        mReverb.SetWidth(static_cast<float>(value / 100.0));
        break;

      case kParamReverbDry:
        // Convert 0-100% to 0.0-1.0
        mReverb.SetDryLevel(static_cast<float>(value / 100.0));
        break;

      case kParamReverbWet:
        // Convert 0-100% to 0.0-1.0
        mReverb.SetWetLevel(static_cast<float>(value / 100.0));
        break;

      case kParamReverbPreDelay:
        // Pre-delay in milliseconds (0-100 ms)
        mReverb.SetPreDelay(static_cast<float>(value));
        break;

      case kParamReverbMode:
        mReverb.SetMode(static_cast<ReverbMode>(static_cast<int>(value)));
        break;

      case kParamReverbColor:
        mReverb.SetColor(static_cast<ReverbColor>(static_cast<int>(value)));
        break;

      case kParamReverbModRate:
        // Modulation rate in Hz (0.1-2.0)
        mReverb.SetModRate(static_cast<float>(value));
        break;

      case kParamReverbModDepth:
        // Modulation depth 0-100% -> 0.0-1.0
        mReverb.SetModDepth(static_cast<float>(value / 100.0));
        break;

      case kParamReverbLowCut:
        // Low-cut frequency in Hz (20-500)
        mReverb.SetLowCut(static_cast<float>(value));
        break;

      case kParamReverbDensity:
        // Density 0-100% -> 0.0-1.0
        mReverb.SetDensity(static_cast<float>(value / 100.0));
        break;

      case kParamReverbEarlyLate:
        // Early/Late balance 0-100% -> 0.0-1.0
        mReverb.SetEarlyLate(static_cast<float>(value / 100.0));
        break;

      case kParamReverbFreeze:
        // Freeze mode on/off
        mReverb.SetFreeze(value > 0.5);
        break;

      // ─────────────────────────────────────────────────────────────────────────
      // VOICE MODE & GLIDE PARAMETERS
      // ─────────────────────────────────────────────────────────────────────────
      case kParamVoiceMode:
      {
        // ───────────────────────────────────────────────────────────────────────
        // VOICE MODE SWITCH - CRITICAL: Must cleanly transition between modes
        // ───────────────────────────────────────────────────────────────────────
        // PROBLEM: Poly and Mono/Legato modes use different voice ownership:
        //   - Poly: iPlug2's VoiceAllocator owns and manages all voices
        //   - Mono/Legato: We manually control mVoices[0] via mMonoVoice pointer
        //
        // If we switch modes while voices are playing, we get conflicts:
        //   - Poly→Mono: VoiceAllocator still thinks it owns mVoices[0]
        //   - Mono→Poly: mVoices[0] might be busy but allocator state is stale
        //
        // SOLUTION: Stop all voices and reset allocator state BEFORE switching.
        // This prevents double-processing, dangling pointers, and crashes.
        // ───────────────────────────────────────────────────────────────────────

        int newMode = static_cast<int>(value);

        // Skip if mode isn't actually changing
        if (newMode == mVoiceMode)
          break;

        // STEP 1: Release all voices gracefully (allows envelope release phase)
        // This handles both poly voices AND our mono voice in one pass.
        mSynth.ForEachVoice([](SynthVoice& voice) {
          voice.Release();
        });

        // STEP 2: Clear mono-specific state
        // mMonoVoice is just a pointer to mVoices[0], not a separate allocation.
        // Setting to nullptr prevents stale pointer access during transition.
        mMonoVoice = nullptr;
        mNoteStackSize = 0;

        // STEP 3: Reset iPlug2's VoiceAllocator internal state
        // This clears the allocator's tracking of which voices are active/free.
        // Without this, the allocator has stale state after we manually
        // released voices, causing incorrect voice allocation on next note.
        mSynth.Reset();

        // STEP 4: Now safe to switch mode
        mVoiceMode = newMode;

        break;
      }

      case kParamGlideEnable:
        mGlideEnable = value > 0.5;
        // Update all voices with new glide state
        mSynth.ForEachVoice([this](SynthVoice& voice) {
          Voice& v = dynamic_cast<Voice&>(voice);
          if (mGlideEnable && (mVoiceMode == 1 || mVoiceMode == 2))
            v.SetPortamentoTime(mGlideTimeMs);
          else
            v.SetPortamentoTime(0.0f);  // Instant pitch change
        });
        break;

      case kParamGlideTime:
        mGlideTimeMs = static_cast<float>(value);
        // Update all voices with new glide time (if glide is enabled)
        if (mGlideEnable && (mVoiceMode == 1 || mVoiceMode == 2))
        {
          mSynth.ForEachVoice([this](SynthVoice& voice) {
            dynamic_cast<Voice&>(voice).SetPortamentoTime(mGlideTimeMs);
          });
        }
        break;

      default:
        break;
    }
  }

private:
  MidiSynth mSynth{VoiceAllocator::kPolyModePoly};
  std::vector<Voice*> mVoices;  // Track voices for destructor cleanup
  const WavetableOscillator::WavetableData* mWavetable = nullptr;  // Pointer to static table
  float mGain = 0.8f;
  float mGainSmoothed = 0.8f;
  float mGainSmoothCoeff = 0.001f;  // Sample-rate aware, set in Reset()

  // ─────────────────────────────────────────────────────────────────────────────
  // VOICE MANAGEMENT - Track active voice count for dynamic release and hard cap
  // ─────────────────────────────────────────────────────────────────────────────
  int mActiveVoiceCount = 0;  // Current number of active voices

  // ─────────────────────────────────────────────────────────────────────────────
  // VOICE MODE & GLIDE (Portamento)
  // ─────────────────────────────────────────────────────────────────────────────
  // Voice modes control how polyphony behaves:
  //   0 = Poly:   Multiple voices, each note gets its own voice (default)
  //   1 = Mono:   Single voice, new notes instantly take over, retriggering env
  //   2 = Legato: Single voice, overlapping notes glide smoothly without retrigger
  //
  // Glide smoothly slides pitch between notes (essential for bass/lead sounds).
  // Only active in Mono/Legato modes - Poly has no concept of "previous note".
  // ─────────────────────────────────────────────────────────────────────────────
  int mVoiceMode = 0;         // 0=Poly, 1=Mono, 2=Legato
  bool mGlideEnable = false;  // Glide on/off
  float mGlideTimeMs = 100.0f; // Glide time in milliseconds

  // Note stack for mono/legato - tracks all currently held notes
  // When top note is released, we glide back to previous held note
  static constexpr int kMaxNoteStackSize = 16;
  int mNoteStack[kMaxNoteStackSize] = {0};  // MIDI note numbers
  int mNoteStackSize = 0;                    // Current number of held notes
  Voice* mMonoVoice = nullptr;               // The single voice used in mono/legato mode
  bool mMonoVoiceProcessedThisBlock = false; // Prevents double processing

  // ─────────────────────────────────────────────────────────────────────────────
  // GLOBAL FILTER ENABLE - Bypass all voice filters when off (hear raw oscillators)
  // ─────────────────────────────────────────────────────────────────────────────
  bool mFilterEnable = true;  // Default ON for subtractive synthesis

  // ─────────────────────────────────────────────────────────────────────────────
  // GLOBAL LFOs - Shared across all voices (Serum/Vital style)
  // Unlike per-voice LFOs, global LFOs ensure all voices modulate in sync.
  // Pre-computed buffers enable sample-accurate modulation.
  // ─────────────────────────────────────────────────────────────────────────────
  static constexpr int kMaxBlockSize = 4096;

  // LFO1 - Global instance and config
  LFO mGlobalLFO1;
  float mLFO1Buffer[kMaxBlockSize] = {0};   // Pre-computed values for current block
  float mLFO1Low = -1.0f;                   // Low point (-1.0 to +1.0, maps to -100% to +100%)
  float mLFO1High = 1.0f;                   // High point (-1.0 to +1.0, maps to -100% to +100%)
  LFODestination mLFO1Destination = LFODestination::Filter;
  bool mLFO1Enable = true;                  // On/Off toggle (default ON for backwards compatibility)
  bool mLFO1Retrigger = false;
  LFOSyncRate mLFO1SyncRate = LFOSyncRate::Off;
  float mLFO1FreeRate = 1.0f;               // Free-running rate (Hz)

  // LFO2 - Global instance and config
  LFO mGlobalLFO2;
  float mLFO2Buffer[kMaxBlockSize] = {0};   // Pre-computed values for current block
  float mLFO2Low = 0.0f;                    // Low point (default 0 = off)
  float mLFO2High = 0.0f;                   // High point (default 0 = off)
  LFODestination mLFO2Destination = LFODestination::Off;
  bool mLFO2Enable = false;                 // On/Off toggle (default OFF - LFO2 is secondary)
  bool mLFO2Retrigger = false;
  LFOSyncRate mLFO2SyncRate = LFOSyncRate::Off;
  float mLFO2FreeRate = 1.0f;               // Free-running rate (Hz)

  // Thread-safe LFO retrigger flags
  // Set by ProcessMidiMsg (may be on MIDI thread), read/cleared by ProcessBlock (audio thread)
  std::atomic<bool> mLFO1NeedsReset{false};
  std::atomic<bool> mLFO2NeedsReset{false};

  float mTempo = 120.0f;                    // Host tempo in BPM
  bool mTransportWasRunning = false;        // Track transport state for buffer clearing

  // ─────────────────────────────────────────────────────────────────────────────
  // STEREO DELAY - Post-processing effect with dry/wet filtering
  // Processed after master gain but before final output
  // ─────────────────────────────────────────────────────────────────────────────
  StereoDelay mDelay;
  float mDelayTimeMs = 250.0f;              // Manual delay time when sync is off
  DelaySyncRate mDelaySyncRate = DelaySyncRate::Off;

  // ─────────────────────────────────────────────────────────────────────────────
  // GLOBAL DELAY ENABLE - Bypass entire delay processing when off (zero CPU)
  // Unlike filter bypass which still processes signal path, delay bypass skips
  // the entire processing loop for true zero-overhead when disabled.
  // ─────────────────────────────────────────────────────────────────────────────
  bool mDelayEnable = false;                // Default OFF (effect, not core sound)

  // ─────────────────────────────────────────────────────────────────────────────
  // DATTORRO PLATE REVERB - Professional-quality reverb effect
  // Processed after delay but before final output limiter
  // ─────────────────────────────────────────────────────────────────────────────
  DattorroReverb mReverb;

  // ─────────────────────────────────────────────────────────────────────────────
  // GLOBAL REVERB ENABLE - Bypass entire reverb processing when off (zero CPU)
  // Unlike filter bypass which still processes signal path, reverb bypass skips
  // the entire processing loop for true zero-overhead when disabled.
  // ─────────────────────────────────────────────────────────────────────────────
  bool mReverbEnable = false;               // Default OFF (effect, not core sound)

  // ─────────────────────────────────────────────────────────────────────────────
  // OUTPUT LIMITER - Professional-grade brickwall limiting using Q library
  // ─────────────────────────────────────────────────────────────────────────────
  // Uses Q's soft_knee_compressor with very high ratio for transparent limiting.
  // Envelope followers track peak levels with attack/release for musical response.
  //
  // STEREO LINKING: Both channels share the same gain reduction to preserve
  // stereo image. Gain is computed from the maximum envelope of both channels.
  //
  // GAIN STAGING: With kPolyScale=0.25 (single voice at -12dB):
  //   - Threshold: -7dB (conservative, catches peaks early)
  //   - Knee width: 6dB (smooth transition into limiting)
  //   - Ratio: 20:1 (0.05) - near-brickwall limiting
  //   - Attack: 0.1ms (very fast to catch transients from polyphony)
  //   - Release: 50ms (faster recovery for dense playing)
  //   - Final stage: fastTanh soft clipping (no harsh digital clipping)
  // ─────────────────────────────────────────────────────────────────────────────
  q::ar_envelope_follower mLimiterEnvL{0.1_ms, 50_ms, 48000.0f};
  q::ar_envelope_follower mLimiterEnvR{0.1_ms, 50_ms, 48000.0f};
  q::soft_knee_compressor mLimiter{-7_dB, 6_dB, 0.05f};  // threshold, width, ratio (20:1)
};
