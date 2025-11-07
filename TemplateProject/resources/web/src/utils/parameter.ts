/**
 * Parameter value conversion and display utilities
 * AUTO-GENERATED - DO NOT EDIT MANUALLY
 * Generated from C++ source files using AI extraction
 */

import { EParams } from '../config/constants';

/**
 * Parameter display configuration
 */
interface ParamDisplayConfig {
  unit: string;
  format: (value: number) => string;
}

/**
 * Convert normalized parameter value (0-1) to display value
 */
export function normalizedToDisplay(paramIdx: EParams, normalizedValue: number): string {
  const config = getParamDisplayConfig(paramIdx);
  const displayValue = normalizedToActual(paramIdx, normalizedValue);
  return config.format(displayValue) + ' ' + config.unit;
}

/**
 * Convert normalized parameter value (0-1) to actual value
 */
export function normalizedToActual(paramIdx: EParams, normalizedValue: number): number {
  const value = Math.max(0, Math.min(1, normalizedValue));
  
  switch (paramIdx) {
    case EParams.kParamGain:
    case EParams.kParamSustain:
    case EParams.kParamLFODepth:
    case EParams.kParamOsc1Mix:
    case EParams.kParamOsc2Mix:
    case EParams.kParamOsc3Mix:
    case EParams.kParamFilterKeytrack:
    case EParams.kParamFilterSustain:
    case EParams.kParamLFO2Depth:
    case EParams.kParamDelayFeedback:
    case EParams.kParamDelayDry:
    case EParams.kParamDelayWet:
    case EParams.kParamReverbDamp:
    case EParams.kParamReverbDry:
    case EParams.kParamReverbWet:
      return value * 100; // 0 to 100
    case EParams.kParamNoteGlideTime:
      return value * 30; // 0 to 30
    case EParams.kParamAttack:
    case EParams.kParamDecay:
    case EParams.kParamFilterAttack:
    case EParams.kParamFilterDecay:
      return 1 + Math.pow(value, 3) * 999; // Power curve: 1-1000ms, shape=3
    case EParams.kParamRelease:
    case EParams.kParamFilterRelease:
      return 2 + Math.pow(value, 3) * 998; // Power curve: 2-1000ms, shape=3
    case EParams.kParamLFOShape:
    case EParams.kParamLFORateTempo:
    case EParams.kParamLFO2RateTempo:
    case EParams.kParamLFO2Shape:
      return Math.round(value * 1); // 0-1 enum (2 options)
    case EParams.kParamLFORateHz:
    case EParams.kParamLFO2RateHz:
      return (function() {
      const minFreq = 0.01;
      const maxFreq = 40;
      const mAdd = Math.log(minFreq);
      const mMul = Math.log(maxFreq / minFreq);
      return Math.exp(mAdd + value * mMul);
    })(); // Exponential: 0.01-40Hz
    case EParams.kParamLFORateMode:
    case EParams.kParamLFO2RateMode:
    case EParams.kParamOscSync:
      return value > 0.5 ? 1.0 : 0.0; // boolean
    // Oscillator parameters
    case EParams.kParamOsc1Detune:
    case EParams.kParamOsc2Detune:
    case EParams.kParamOsc3Detune:
      return (value - 0.5) * 100; // -50 to 50
    case EParams.kParamOsc1Octave:
    case EParams.kParamOsc2Octave:
    case EParams.kParamOsc3Octave:
      return Math.round(value * 4 + -2); // -2 to 2
    case EParams.kParamOsc1Wave:
    case EParams.kParamOsc2Wave:
    case EParams.kParamOsc3Wave:
      return Math.round(value * 3); // 0-3 enum (4 options)
    // Filter parameters
    case EParams.kParamFilterCutoff:
      return (function() {
      const minFreq = 20;
      const maxFreq = 20000;
      const mAdd = Math.log(minFreq);
      const mMul = Math.log(maxFreq / minFreq);
      return Math.exp(mAdd + value * mMul);
    })(); // Exponential: 20-20000Hz
    case EParams.kParamFilterResonance:
      return 0.1 + value * 9.9; // 0.1 to 10
    case EParams.kParamFilterEnvAmount:
      return (value - 0.5) * 10000; // -5000 to 5000
    // LFO2
    // Delay
    case EParams.kParamDelayTime:
      return 1 + value * 1999; // 1 to 2000
    // Sync
    case EParams.kParamOscSyncRatio:
      return 0.125 + value * 7.875; // 0.125 to 8
    // Reverb parameters
    case EParams.kParamReverbRoomSize:
      return 0.3 + value * 0.69; // 0.3 to 0.99
    case EParams.kParamReverbWidth:
      return value * 1; // 0 to 1

    default:
      return value;
  }
}

/**
 * Get parameter display configuration
 */
function getParamDisplayConfig(paramIdx: EParams): ParamDisplayConfig {
  switch (paramIdx) {
    case EParams.kParamGain:
    case EParams.kParamSustain:
    case EParams.kParamFilterSustain:
    case EParams.kParamDelayFeedback:
    case EParams.kParamDelayDry:
    case EParams.kParamDelayWet:
      return {
        unit: '%',
        format: (v) => v.toFixed(1)
      };
    case EParams.kParamNoteGlideTime:
    case EParams.kParamAttack:
    case EParams.kParamDecay:
    case EParams.kParamRelease:
    case EParams.kParamFilterAttack:
    case EParams.kParamFilterDecay:
    case EParams.kParamFilterRelease:
    case EParams.kParamDelayTime:
      return {
        unit: 'ms',
        format: (v) => v.toFixed(1)
      };
    case EParams.kParamLFOShape:
    case EParams.kParamLFORateHz:
    case EParams.kParamLFORateTempo:
    case EParams.kParamLFORateMode:
    case EParams.kParamLFODepth:
    case EParams.kParamOsc1Mix:
    case EParams.kParamOsc2Mix:
    case EParams.kParamOsc3Mix:
    case EParams.kParamOsc1Wave:
    case EParams.kParamOsc2Wave:
    case EParams.kParamOsc3Wave:
    case EParams.kParamFilterCutoff:
    case EParams.kParamFilterResonance:
    case EParams.kParamLFO2RateHz:
    case EParams.kParamLFO2RateTempo:
    case EParams.kParamLFO2RateMode:
    case EParams.kParamLFO2Shape:
    case EParams.kParamLFO2Depth:
    case EParams.kParamOscSync:
    case EParams.kParamOscSyncRatio:
    case EParams.kParamReverbRoomSize:
    case EParams.kParamReverbDamp:
    case EParams.kParamReverbWidth:
    case EParams.kParamReverbDry:
    case EParams.kParamReverbWet:
      return {
        unit: '',
        format: (v) => v.toString()
      };
    case EParams.kParamOsc1Detune:
    case EParams.kParamOsc2Detune:
    case EParams.kParamOsc3Detune:
      return {
        unit: 'ct',
        format: (v) => v >= 0 ? `+${v.toFixed(0)}` : `${v.toFixed(0)}`
      };
    case EParams.kParamOsc1Octave:
    case EParams.kParamOsc2Octave:
    case EParams.kParamOsc3Octave:
      return {
        unit: '',
        format: (v) => v >= 0 ? `+${v.toFixed(0)}` : `${v.toFixed(0)}`
      };
    case EParams.kParamFilterEnvAmount:
      return {
        unit: 'Hz',
        format: (v) => v.toFixed(1)
      };
    case EParams.kParamFilterKeytrack:
      return {
        unit: 'Hz/key',
        format: (v) => v.toFixed(1)
      };

    default:
      return {
        unit: '',
        format: (v) => v.toString()
      };
  }
}

/**
 * Convert actual parameter value to normalized value (0-1)
 * This is the inverse of normalizedToActual
 */
export function actualToNormalized(paramIdx: EParams, actualValue: number): number {
  switch (paramIdx) {
    case EParams.kParamGain:
    case EParams.kParamSustain:
    case EParams.kParamLFODepth:
    case EParams.kParamOsc1Mix:
    case EParams.kParamOsc2Mix:
    case EParams.kParamOsc3Mix:
    case EParams.kParamFilterKeytrack:
    case EParams.kParamFilterSustain:
    case EParams.kParamLFO2Depth:
    case EParams.kParamDelayFeedback:
    case EParams.kParamDelayDry:
    case EParams.kParamDelayWet:
    case EParams.kParamReverbDamp:
    case EParams.kParamReverbDry:
    case EParams.kParamReverbWet:
      return actualValue / 100.0; // 0 to 100 -> 0 to 1
    case EParams.kParamNoteGlideTime:
      return actualValue / 30.0; // 0 to 30 -> 0 to 1
    case EParams.kParamAttack:
    case EParams.kParamDecay:
    case EParams.kParamFilterAttack:
    case EParams.kParamFilterDecay:
      return Math.pow((actualValue - 1) / 999, 1.0 / 3); // Inverse power curve: 1-1000ms, shape=3
    case EParams.kParamRelease:
    case EParams.kParamFilterRelease:
      return Math.pow((actualValue - 2) / 998, 1.0 / 3); // Inverse power curve: 2-1000ms, shape=3
    case EParams.kParamLFOShape:
    case EParams.kParamLFORateTempo:
    case EParams.kParamLFO2RateTempo:
    case EParams.kParamLFO2Shape:
      return actualValue / 1.0; // 0-1 enum -> 0-1
    case EParams.kParamLFORateHz:
    case EParams.kParamLFO2RateHz:
      return (function() {
      const minFreq = 0.01;
      const maxFreq = 40;
      const mAdd = Math.log(minFreq);
      const mMul = Math.log(maxFreq / minFreq);
      return (Math.log(actualValue) - mAdd) / mMul;
    })(); // Inverse exponential: 0.01-40Hz
    case EParams.kParamLFORateMode:
    case EParams.kParamLFO2RateMode:
    case EParams.kParamOscSync:
      return actualValue > 0.5 ? 1.0 : 0.0; // boolean
    // Oscillator parameters
    case EParams.kParamOsc1Detune:
    case EParams.kParamOsc2Detune:
    case EParams.kParamOsc3Detune:
      return (actualValue / 100.0) + 0.5; // -50 to 50 -> 0 to 1
    case EParams.kParamOsc1Octave:
    case EParams.kParamOsc2Octave:
    case EParams.kParamOsc3Octave:
      return (actualValue - -2) / 4.0; // -2 to 2 -> 0 to 1
    case EParams.kParamOsc1Wave:
    case EParams.kParamOsc2Wave:
    case EParams.kParamOsc3Wave:
      return actualValue / 3.0; // 0-3 enum -> 0-1
    // Filter parameters
    case EParams.kParamFilterCutoff:
      return (function() {
      const minFreq = 20;
      const maxFreq = 20000;
      const mAdd = Math.log(minFreq);
      const mMul = Math.log(maxFreq / minFreq);
      return (Math.log(actualValue) - mAdd) / mMul;
    })(); // Inverse exponential: 20-20000Hz
    case EParams.kParamFilterResonance:
      return (actualValue - 0.1) / 9.9; // 0.1 to 10 -> 0 to 1
    case EParams.kParamFilterEnvAmount:
      return (actualValue / 10000.0) + 0.5; // -5000 to 5000 -> 0 to 1
    // LFO2
    // Delay
    case EParams.kParamDelayTime:
      return (actualValue - 1) / 1999.0; // 1 to 2000 -> 0 to 1
    // Sync
    case EParams.kParamOscSyncRatio:
      return (actualValue - 0.125) / 7.875; // 0.125 to 8 -> 0 to 1
    // Reverb parameters
    case EParams.kParamReverbRoomSize:
      return (actualValue - 0.3) / 0.69; // 0.3 to 0.99 -> 0 to 1
    case EParams.kParamReverbWidth:
      return actualValue / 1.0; // 0 to 1 -> 0 to 1

    default:
      return actualValue;
  }
}

/**
 * Get default normalized values matching C++ DSP defaults
 * These values exactly match the defaults in TemplateProject.cpp
 */
export function getDefaultNormalizedValues(): Map<EParams, number> {
  const defaults = new Map<EParams, number>();
  
  // Gain: 100 (0-100 %)
  defaults.set(EParams.kParamGain, 100 / 100);
  // Note Glide Time: 0 (0-30 ms)
  defaults.set(EParams.kParamNoteGlideTime, 0 / 30);
  // Attack: 10 (1-1000 ms)
  defaults.set(EParams.kParamAttack, Math.pow((10 - 1) / 999, 1.0 / 3));
  // Decay: 10 (1-1000 ms)
  defaults.set(EParams.kParamDecay, Math.pow((10 - 1) / 999, 1.0 / 3));
  // Sustain: 50 (0-100 %)
  defaults.set(EParams.kParamSustain, 50 / 100);
  // Release: 10 (2-1000 ms)
  defaults.set(EParams.kParamRelease, Math.pow((10 - 2) / 998, 1.0 / 3));
  // LFO Shape: 0 (0-0)
  defaults.set(EParams.kParamLFOShape, actualToNormalized(EParams.kParamLFOShape, 0));
  // LFO Rate: 1 (0.01-40)
  defaults.set(EParams.kParamLFORateHz, (Math.log(1) - Math.log(0.01)) / Math.log(40 / 0.01));
  // LFO Rate: 0 (0-0)
  defaults.set(EParams.kParamLFORateTempo, actualToNormalized(EParams.kParamLFORateTempo, 0));
  // LFO Sync: 1 (0-1)
  defaults.set(EParams.kParamLFORateMode, 1 / 1);
  // LFO Depth: 0 (0-100)
  defaults.set(EParams.kParamLFODepth, 0 / 100);
  // Osc1 Mix: 100 (0-100)
  defaults.set(EParams.kParamOsc1Mix, 100 / 100);
  // Osc2 Mix: 0 (0-100)
  defaults.set(EParams.kParamOsc2Mix, 0 / 100);
  // Osc3 Mix: 0 (0-100)
  defaults.set(EParams.kParamOsc3Mix, 0 / 100);
  // Osc1 Detune: 0 (-50-50 cents)
  defaults.set(EParams.kParamOsc1Detune, (0 / 100) + 0.5);
  // Osc2 Detune: 0 (-50-50 cents)
  defaults.set(EParams.kParamOsc2Detune, (0 / 100) + 0.5);
  // Osc3 Detune: 0 (-50-50 cents)
  defaults.set(EParams.kParamOsc3Detune, (0 / 100) + 0.5);
  // Osc1 Octave: 0 (-2-2)
  defaults.set(EParams.kParamOsc1Octave, actualToNormalized(EParams.kParamOsc1Octave, 0));
  // Osc2 Octave: 0 (-2-2)
  defaults.set(EParams.kParamOsc2Octave, actualToNormalized(EParams.kParamOsc2Octave, 0));
  // Osc3 Octave: 0 (-2-2)
  defaults.set(EParams.kParamOsc3Octave, actualToNormalized(EParams.kParamOsc3Octave, 0));
  // Osc1 Wave: 0 (0-0)
  defaults.set(EParams.kParamOsc1Wave, actualToNormalized(EParams.kParamOsc1Wave, 0));
  // Osc2 Wave: 0 (0-0)
  defaults.set(EParams.kParamOsc2Wave, actualToNormalized(EParams.kParamOsc2Wave, 0));
  // Osc3 Wave: 0 (0-0)
  defaults.set(EParams.kParamOsc3Wave, actualToNormalized(EParams.kParamOsc3Wave, 0));
  // Filter Cutoff: 1000 (20-20000)
  defaults.set(EParams.kParamFilterCutoff, (Math.log(1000) - Math.log(20)) / Math.log(20000 / 20));
  // Filter Resonance: 1 (0.1-10)
  defaults.set(EParams.kParamFilterResonance, (1 - 0.1) / 9.9);
  // Filter Env Amount: 0 (-5000-5000 Hz)
  defaults.set(EParams.kParamFilterEnvAmount, (0 / 10000) + 0.5);
  // Filter Keytrack: 0 (0-100 Hz/key)
  defaults.set(EParams.kParamFilterKeytrack, 0 / 100);
  // Filter Attack: 10 (1-1000 ms)
  defaults.set(EParams.kParamFilterAttack, Math.pow((10 - 1) / 999, 1.0 / 3));
  // Filter Decay: 10 (1-1000 ms)
  defaults.set(EParams.kParamFilterDecay, Math.pow((10 - 1) / 999, 1.0 / 3));
  // Filter Sustain: 50 (0-100 %)
  defaults.set(EParams.kParamFilterSustain, 50 / 100);
  // Filter Release: 10 (2-1000 ms)
  defaults.set(EParams.kParamFilterRelease, Math.pow((10 - 2) / 998, 1.0 / 3));
  // LFO2 Rate: 1 (0.01-40)
  defaults.set(EParams.kParamLFO2RateHz, (Math.log(1) - Math.log(0.01)) / Math.log(40 / 0.01));
  // LFO2 Rate: 0 (0-0)
  defaults.set(EParams.kParamLFO2RateTempo, actualToNormalized(EParams.kParamLFO2RateTempo, 0));
  // LFO2 Sync: 1 (0-1)
  defaults.set(EParams.kParamLFO2RateMode, 1 / 1);
  // LFO2 Shape: 0 (0-0)
  defaults.set(EParams.kParamLFO2Shape, actualToNormalized(EParams.kParamLFO2Shape, 0));
  // LFO2 Depth: 0 (0-100)
  defaults.set(EParams.kParamLFO2Depth, 0 / 100);
  // Delay Time: 250 (1-2000 ms)
  defaults.set(EParams.kParamDelayTime, (250 - 1) / 1999);
  // Delay Feedback: 0 (0-100 %)
  defaults.set(EParams.kParamDelayFeedback, 0 / 100);
  // Delay Dry: 100 (0-100 %)
  defaults.set(EParams.kParamDelayDry, 100 / 100);
  // Delay Wet: 0 (0-100 %)
  defaults.set(EParams.kParamDelayWet, 0 / 100);
  // Osc Sync: 0 (0-1)
  defaults.set(EParams.kParamOscSync, 0 / 1);
  // Sync Ratio: 1 (0.125-8)
  defaults.set(EParams.kParamOscSyncRatio, (1 - 0.125) / 7.875);
  // Reverb Room Size: 0.5 (0.3-0.99)
  defaults.set(EParams.kParamReverbRoomSize, (0.5 - 0.3) / 0.69);
  // Reverb Damp: 50 (0-100)
  defaults.set(EParams.kParamReverbDamp, 50 / 100);
  // Reverb Width: 0.5 (0-1)
  defaults.set(EParams.kParamReverbWidth, 0.5 / 1);
  // Reverb Dry: 100 (0-100)
  defaults.set(EParams.kParamReverbDry, 100 / 100);
  // Reverb Wet: 0 (0-100)
  defaults.set(EParams.kParamReverbWet, 0 / 100);

  
  return defaults;
}

export function getParamInputId(paramIdx: EParams): string {
  const paramNames: Record<EParams, string> = {
    [EParams.kParamGain]: 'paramGain',
    [EParams.kParamNoteGlideTime]: 'paramNoteGlideTime',
    [EParams.kParamAttack]: 'paramAttack',
    [EParams.kParamDecay]: 'paramDecay',
    [EParams.kParamSustain]: 'paramSustain',
    [EParams.kParamRelease]: 'paramRelease',
    [EParams.kParamLFOShape]: 'paramLFOShape',
    [EParams.kParamLFORateHz]: 'paramLFORateHz',
    [EParams.kParamLFORateTempo]: 'paramLFORateTempo',
    [EParams.kParamLFORateMode]: 'paramLFORateMode',
    [EParams.kParamLFODepth]: 'paramLFODepth',
    [EParams.kParamOsc1Mix]: 'paramOsc1Mix',
    [EParams.kParamOsc2Mix]: 'paramOsc2Mix',
    [EParams.kParamOsc3Mix]: 'paramOsc3Mix',
    [EParams.kParamOsc1Detune]: 'paramOsc1Detune',
    [EParams.kParamOsc2Detune]: 'paramOsc2Detune',
    [EParams.kParamOsc3Detune]: 'paramOsc3Detune',
    [EParams.kParamOsc1Octave]: 'paramOsc1Octave',
    [EParams.kParamOsc2Octave]: 'paramOsc2Octave',
    [EParams.kParamOsc3Octave]: 'paramOsc3Octave',
    [EParams.kParamOsc1Wave]: 'paramOsc1Wave',
    [EParams.kParamOsc2Wave]: 'paramOsc2Wave',
    [EParams.kParamOsc3Wave]: 'paramOsc3Wave',
    [EParams.kParamFilterCutoff]: 'paramFilterCutoff',
    [EParams.kParamFilterResonance]: 'paramFilterResonance',
    [EParams.kParamFilterEnvAmount]: 'paramFilterEnvAmount',
    [EParams.kParamFilterKeytrack]: 'paramFilterKeytrack',
    [EParams.kParamFilterAttack]: 'paramFilterAttack',
    [EParams.kParamFilterDecay]: 'paramFilterDecay',
    [EParams.kParamFilterSustain]: 'paramFilterSustain',
    [EParams.kParamFilterRelease]: 'paramFilterRelease',
    [EParams.kParamLFO2RateHz]: 'paramLFO2RateHz',
    [EParams.kParamLFO2RateTempo]: 'paramLFO2RateTempo',
    [EParams.kParamLFO2RateMode]: 'paramLFO2RateMode',
    [EParams.kParamLFO2Shape]: 'paramLFO2Shape',
    [EParams.kParamLFO2Depth]: 'paramLFO2Depth',
    [EParams.kParamDelayTime]: 'paramDelayTime',
    [EParams.kParamDelayFeedback]: 'paramDelayFeedback',
    [EParams.kParamDelayDry]: 'paramDelayDry',
    [EParams.kParamDelayWet]: 'paramDelayWet',
    [EParams.kParamOscSync]: 'paramOscSync',
    [EParams.kParamOscSyncRatio]: 'paramOscSyncRatio',
    [EParams.kParamReverbRoomSize]: 'paramReverbRoomSize',
    [EParams.kParamReverbDamp]: 'paramReverbDamp',
    [EParams.kParamReverbWidth]: 'paramReverbWidth',
    [EParams.kParamReverbDry]: 'paramReverbDry',
    [EParams.kParamReverbWet]: 'paramReverbWet',

    [EParams.kNumParams]: '',
  };
  
  return paramNames[paramIdx];
}

/**
 * Get parameter value display element ID
 */
export function getParamValueId(paramIdx: EParams): string {
  return getParamInputId(paramIdx) + 'Value';
}
