/**
 * Parameter value conversion and display utilities
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
      return value * 100; // 0-100%
      
    case EParams.kParamNoteGlideTime:
      return value * 30; // 0-30ms
      
    case EParams.kParamAttack:
    case EParams.kParamDecay:
    case EParams.kParamFilterAttack:
    case EParams.kParamFilterDecay:
      // Power curve: 1-1000ms, shape=3
      return 1 + Math.pow(value, 3) * 999;
      
    case EParams.kParamSustain:
    case EParams.kParamFilterSustain:
      return value * 100; // 0-100%
      
    case EParams.kParamRelease:
    case EParams.kParamFilterRelease:
      return 2 + value * 998; // 2-1000ms
      
    case EParams.kParamLFORateHz:
    case EParams.kParamLFO2RateHz:
      return 0.01 + value * 39.99; // 0.01-40Hz
      
    case EParams.kParamLFODepth:
    case EParams.kParamLFO2Depth:
      return value * 100; // 0-100%
    
    // Oscillator parameters
    case EParams.kParamOsc1Mix:
    case EParams.kParamOsc2Mix:
    case EParams.kParamOsc3Mix:
    case EParams.kParamDelayFeedback:
    case EParams.kParamDelayDry:
    case EParams.kParamDelayWet:
    case EParams.kParamReverbDamp:
    case EParams.kParamReverbDry:
    case EParams.kParamReverbWet:
      return value * 100; // 0-100%
    
    // Reverb parameters
    case EParams.kParamReverbRoomSize:
      return 0.3 + value * 0.69; // 0.3 to 0.99
    case EParams.kParamReverbWidth:
      return value; // 0-1 (UI shows 0-1, DSP converts to -1 to 1)
    
    case EParams.kParamOsc1Detune:
    case EParams.kParamOsc2Detune:
    case EParams.kParamOsc3Detune:
      return (value - 0.5) * 100; // -50 to +50 cents
    
    case EParams.kParamOsc1Octave:
    case EParams.kParamOsc2Octave:
    case EParams.kParamOsc3Octave:
      return Math.round((value - 0.5) * 4); // -2 to +2 octaves
    
    case EParams.kParamOsc1Wave:
    case EParams.kParamOsc2Wave:
    case EParams.kParamOsc3Wave:
    case EParams.kParamLFOShape:
    case EParams.kParamLFO2Shape:
      return Math.round(value * 4); // 0-4 enum (5 options)
    
    case EParams.kParamLFORateTempo:
    case EParams.kParamLFO2RateTempo:
      return Math.round(value * 14); // 0-14 enum (15 options)
    
    // Filter parameters
    case EParams.kParamFilterCutoff:
      // Exponential: 20-20000Hz
      const minFreq = 20;
      const maxFreq = 20000;
      const mAdd = Math.log(minFreq);
      const mMul = Math.log(maxFreq / minFreq);
      return Math.exp(mAdd + value * mMul);
    
    case EParams.kParamFilterResonance:
      return 0.1 + value * 9.9; // 0.1-10
    
    case EParams.kParamFilterEnvAmount:
      return (value - 0.5) * 10000; // -5000 to +5000 Hz
    
    case EParams.kParamFilterKeytrack:
      return value * 100; // 0-100 Hz/key
    
    // Delay
    case EParams.kParamDelayTime:
      return 1 + value * 1999; // 1-2000ms
    
    // Sync
    case EParams.kParamOscSyncRatio:
      // 0.125, 0.25, 0.5, 1, 2, 4, 8 (logarithmic)
      const ratios = [0.125, 0.25, 0.5, 1, 2, 4, 8];
      const ratioIdx = Math.round(value * (ratios.length - 1));
      return ratios[ratioIdx];
      
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
    case EParams.kParamLFODepth:
    case EParams.kParamLFO2Depth:
    case EParams.kParamOsc1Mix:
    case EParams.kParamOsc2Mix:
    case EParams.kParamOsc3Mix:
    case EParams.kParamDelayFeedback:
    case EParams.kParamDelayDry:
    case EParams.kParamDelayWet:
    case EParams.kParamFilterSustain:
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
      
    case EParams.kParamLFORateHz:
    case EParams.kParamLFO2RateHz:
      return {
        unit: 'Hz',
        format: (v) => v.toFixed(2)
      };
    
    case EParams.kParamOsc1Detune:
    case EParams.kParamOsc2Detune:
    case EParams.kParamOsc3Detune:
      return {
        unit: 'ct',
        format: (v) => v.toFixed(1)
      };
    
    case EParams.kParamOsc1Octave:
    case EParams.kParamOsc2Octave:
    case EParams.kParamOsc3Octave:
      return {
        unit: '',
        format: (v) => v >= 0 ? `+${v}` : `${v}`
      };
    
    case EParams.kParamFilterCutoff:
      return {
        unit: 'Hz',
        format: (v) => v.toFixed(0)
      };
    
    case EParams.kParamFilterResonance:
      return {
        unit: '',
        format: (v) => v.toFixed(2)
      };
    
    case EParams.kParamFilterEnvAmount:
      return {
        unit: 'Hz',
        format: (v) => v >= 0 ? `+${v.toFixed(0)}` : `${v.toFixed(0)}`
      };
    
    case EParams.kParamFilterKeytrack:
      return {
        unit: 'Hz/key',
        format: (v) => v.toFixed(1)
      };
    
    case EParams.kParamOscSyncRatio:
      return {
        unit: ':1',
        format: (v) => v.toFixed(3)
      };
    
    case EParams.kParamReverbRoomSize:
      return {
        unit: '',
        format: (v) => v.toFixed(2)
      };
    
    case EParams.kParamReverbDamp:
    case EParams.kParamReverbDry:
    case EParams.kParamReverbWet:
      return {
        unit: '%',
        format: (v) => v.toFixed(1)
      };
    
    case EParams.kParamReverbWidth:
      return {
        unit: '',
        format: (v) => v.toFixed(2)
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
      return actualValue / 100.0;
      
    case EParams.kParamNoteGlideTime:
      return actualValue / 30.0;
      
    case EParams.kParamAttack:
    case EParams.kParamDecay:
    case EParams.kParamFilterAttack:
    case EParams.kParamFilterDecay:
      // Inverse power curve: 1-1000ms, shape=3
      return Math.pow((actualValue - 1) / 999, 1.0 / 3.0);
      
    case EParams.kParamSustain:
    case EParams.kParamFilterSustain:
      return actualValue / 100.0;
      
    case EParams.kParamRelease:
    case EParams.kParamFilterRelease:
      return (actualValue - 2) / 998.0;
      
    case EParams.kParamLFORateHz:
    case EParams.kParamLFO2RateHz:
      return (actualValue - 0.01) / 39.99;
      
    case EParams.kParamLFODepth:
    case EParams.kParamLFO2Depth:
      return actualValue / 100.0;
    
    // Oscillator parameters
    case EParams.kParamOsc1Mix:
    case EParams.kParamOsc2Mix:
    case EParams.kParamOsc3Mix:
    case EParams.kParamDelayFeedback:
    case EParams.kParamDelayDry:
    case EParams.kParamDelayWet:
    case EParams.kParamReverbDamp:
    case EParams.kParamReverbDry:
    case EParams.kParamReverbWet:
      return actualValue / 100.0;
    
    // Reverb parameters
    case EParams.kParamReverbRoomSize:
      return (actualValue - 0.3) / 0.69; // 0.3 to 0.99 -> 0 to 1
    case EParams.kParamReverbWidth:
      return actualValue; // 0-1
    
    case EParams.kParamOsc1Detune:
    case EParams.kParamOsc2Detune:
    case EParams.kParamOsc3Detune:
      return (actualValue / 100.0) + 0.5; // -50 to +50 cents -> 0 to 1
    
    case EParams.kParamOsc1Octave:
    case EParams.kParamOsc2Octave:
    case EParams.kParamOsc3Octave:
      return (actualValue / 4.0) + 0.5; // -2 to +2 octaves -> 0 to 1
    
    case EParams.kParamOsc1Wave:
    case EParams.kParamOsc2Wave:
    case EParams.kParamOsc3Wave:
    case EParams.kParamLFOShape:
    case EParams.kParamLFO2Shape:
      return actualValue / 4.0; // 0-4 enum -> 0-1 (5 options)
    
    case EParams.kParamLFORateTempo:
    case EParams.kParamLFO2RateTempo:
      return actualValue / 14.0; // 0-14 enum -> 0-1 (15 options)
    
    // Filter parameters
    case EParams.kParamFilterCutoff:
      // Inverse exponential: 20-20000Hz
      const minFreq = 20;
      const maxFreq = 20000;
      const mAdd = Math.log(minFreq);
      const mMul = Math.log(maxFreq / minFreq);
      return (Math.log(actualValue) - mAdd) / mMul;
    
    case EParams.kParamFilterResonance:
      return (actualValue - 0.1) / 9.9;
    
    case EParams.kParamFilterEnvAmount:
      return (actualValue / 10000.0) + 0.5; // -5000 to +5000 Hz
    
    case EParams.kParamFilterKeytrack:
      return actualValue / 100.0;
    
    // Delay
    case EParams.kParamDelayTime:
      return (actualValue - 1) / 1999.0;
    
    // Sync
    case EParams.kParamOscSyncRatio:
      // 0.125, 0.25, 0.5, 1, 2, 4, 8 -> find index
      const ratios = [0.125, 0.25, 0.5, 1, 2, 4, 8];
      const ratioIdx = ratios.findIndex(r => Math.abs(r - actualValue) < 0.001);
      return ratioIdx >= 0 ? ratioIdx / (ratios.length - 1) : 0.5;
      
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
  
  // Gain: 100.0 (0-100)
  defaults.set(EParams.kParamGain, 1.0);
  
  // Note Glide Time: 0.0 (0-30ms)
  defaults.set(EParams.kParamNoteGlideTime, 0.0);
  
  // Attack: 10.0ms (1-1000ms, power curve shape=3)
  defaults.set(EParams.kParamAttack, actualToNormalized(EParams.kParamAttack, 10.0));
  
  // Decay: 10.0ms (1-1000ms, power curve shape=3)
  defaults.set(EParams.kParamDecay, actualToNormalized(EParams.kParamDecay, 10.0));
  
  // Sustain: 50.0 (0-100%)
  defaults.set(EParams.kParamSustain, 0.5);
  
  // Release: 10.0ms (2-1000ms)
  defaults.set(EParams.kParamRelease, actualToNormalized(EParams.kParamRelease, 10.0));
  
  // LFO Shape: kTriangle (enum 0)
  defaults.set(EParams.kParamLFOShape, 0.0);
  
  // LFO Rate Hz: 1.0Hz (0.01-40Hz)
  defaults.set(EParams.kParamLFORateHz, actualToNormalized(EParams.kParamLFORateHz, 1.0));
  
  // LFO Rate Tempo: k1 (enum index 11 out of 15)
  defaults.set(EParams.kParamLFORateTempo, 11 / 14);
  
  // LFO Sync: true
  defaults.set(EParams.kParamLFORateMode, 1.0);
  
  // LFO Depth: 0.0 (0-100%)
  defaults.set(EParams.kParamLFODepth, 0.0);
  
  // Oscillators
  defaults.set(EParams.kParamOsc1Mix, 1.0); // 100%
  defaults.set(EParams.kParamOsc2Mix, 0.0); // 0%
  defaults.set(EParams.kParamOsc3Mix, 0.0); // 0%
  defaults.set(EParams.kParamOsc1Detune, actualToNormalized(EParams.kParamOsc1Detune, 0.0)); // 0 cents
  defaults.set(EParams.kParamOsc2Detune, actualToNormalized(EParams.kParamOsc2Detune, 0.0)); // 0 cents
  defaults.set(EParams.kParamOsc3Detune, actualToNormalized(EParams.kParamOsc3Detune, 0.0)); // 0 cents
  defaults.set(EParams.kParamOsc1Octave, actualToNormalized(EParams.kParamOsc1Octave, 0)); // 0 octaves
  defaults.set(EParams.kParamOsc2Octave, actualToNormalized(EParams.kParamOsc2Octave, 0)); // 0 octaves
  defaults.set(EParams.kParamOsc3Octave, actualToNormalized(EParams.kParamOsc3Octave, 0)); // 0 octaves
  defaults.set(EParams.kParamOsc1Wave, 0.0); // Sine (enum 0)
  defaults.set(EParams.kParamOsc2Wave, 0.0); // Sine (enum 0)
  defaults.set(EParams.kParamOsc3Wave, 0.0); // Sine (enum 0)
  
  // Filter
  defaults.set(EParams.kParamFilterCutoff, actualToNormalized(EParams.kParamFilterCutoff, 1000.0)); // 1000Hz
  defaults.set(EParams.kParamFilterResonance, actualToNormalized(EParams.kParamFilterResonance, 1.0)); // 1.0
  defaults.set(EParams.kParamFilterEnvAmount, actualToNormalized(EParams.kParamFilterEnvAmount, 0.0)); // 0Hz
  defaults.set(EParams.kParamFilterKeytrack, actualToNormalized(EParams.kParamFilterKeytrack, 0.0)); // 0 Hz/key
  defaults.set(EParams.kParamFilterAttack, actualToNormalized(EParams.kParamFilterAttack, 10.0)); // 10ms
  defaults.set(EParams.kParamFilterDecay, actualToNormalized(EParams.kParamFilterDecay, 10.0)); // 10ms
  defaults.set(EParams.kParamFilterSustain, 0.5); // 50%
  defaults.set(EParams.kParamFilterRelease, actualToNormalized(EParams.kParamFilterRelease, 10.0)); // 10ms
  
  // LFO2
  defaults.set(EParams.kParamLFO2Shape, 0.0); // kTriangle (enum 0)
  defaults.set(EParams.kParamLFO2RateHz, actualToNormalized(EParams.kParamLFO2RateHz, 1.0)); // 1.0Hz
  defaults.set(EParams.kParamLFO2RateTempo, 11 / 14); // k1 (enum index 11)
  defaults.set(EParams.kParamLFO2RateMode, 1.0); // Sync enabled
  defaults.set(EParams.kParamLFO2Depth, 0.0); // 0%
  
  // Delay
  defaults.set(EParams.kParamDelayTime, actualToNormalized(EParams.kParamDelayTime, 250.0)); // 250ms
  defaults.set(EParams.kParamDelayFeedback, 0.0); // 0%
  defaults.set(EParams.kParamDelayDry, 1.0); // 100% dry
  defaults.set(EParams.kParamDelayWet, 0.0); // 0% wet
  
  // Sync
  defaults.set(EParams.kParamOscSync, 0.0); // false
  defaults.set(EParams.kParamOscSyncRatio, actualToNormalized(EParams.kParamOscSyncRatio, 1.0)); // 1.0
  
  // Reverb
  defaults.set(EParams.kParamReverbRoomSize, actualToNormalized(EParams.kParamReverbRoomSize, 0.5)); // 0.5
  defaults.set(EParams.kParamReverbDamp, 0.5); // 50%
  defaults.set(EParams.kParamReverbWidth, actualToNormalized(EParams.kParamReverbWidth, 0.5)); // 0.5
  defaults.set(EParams.kParamReverbDry, 1.0); // 100% dry
  defaults.set(EParams.kParamReverbWet, 0.0); // 0% wet
  
  return defaults;
}

export function getParamInputId(paramIdx: EParams): string {
  const paramNames: Record<EParams, string> = {
    [EParams.kParamGain]: 'paramGain',
    [EParams.kParamNoteGlideTime]: 'paramGlide',
    [EParams.kParamAttack]: 'paramAttack',
    [EParams.kParamDecay]: 'paramDecay',
    [EParams.kParamSustain]: 'paramSustain',
    [EParams.kParamRelease]: 'paramRelease',
    [EParams.kParamLFOShape]: 'paramLFOShape',
    [EParams.kParamLFORateHz]: 'paramLFORateHz',
    [EParams.kParamLFORateTempo]: 'paramLFORateTempo',
    [EParams.kParamLFORateMode]: 'paramLFOSync',
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
    [EParams.kParamLFO2RateMode]: 'paramLFO2Sync',
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
