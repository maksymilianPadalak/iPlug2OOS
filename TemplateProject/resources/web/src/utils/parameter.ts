/**
 * Parameter value conversion and display utilities
 * AUTO-GENERATED - DO NOT EDIT MANUALLY
 * Generated from C++ source files using deterministic parsing
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
    case EParams.kParamOsc1Mix:
    case EParams.kParamOsc2Mix:
    case EParams.kParamOsc3Mix:
    case EParams.kParamOsc4Mix:
    case EParams.kParamReverbDamp:
    case EParams.kParamReverbDry:
    case EParams.kParamReverbWet:
      return value * 100; // 0 to 100
    case EParams.kParamAttack:
      return 1 + Math.pow(value, 3) * 1999; // Power curve: 1-2000ms, shape=3
    case EParams.kParamDecay:
      return 1 + Math.pow(value, 3) * 3999; // Power curve: 1-4000ms, shape=3
    case EParams.kParamRelease:
      return 2 + Math.pow(value, 3) * 7998; // Power curve: 2-8000ms, shape=3
    // Oscillator parameters
    case EParams.kParamOsc1Detune:
    case EParams.kParamOsc2Detune:
    case EParams.kParamOsc4Detune:
      return (value - 0.5) * 100; // -50 to 50
    case EParams.kParamOsc1Octave:
    case EParams.kParamOsc2Octave:
    case EParams.kParamOsc4Octave:
      return Math.round(value * 4 + -2); // -2 to 2
    case EParams.kParamOsc1Wave:
    case EParams.kParamOsc2Wave:
    case EParams.kParamOsc3Wave:
    case EParams.kParamOsc4Wave:
      return Math.round(value * 3); // 0-3 enum (4 options)
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
    case EParams.kParamOsc1Mix:
    case EParams.kParamOsc2Mix:
    case EParams.kParamOsc3Mix:
    case EParams.kParamOsc4Mix:
    case EParams.kParamReverbDamp:
    case EParams.kParamReverbDry:
    case EParams.kParamReverbWet:
      return {
        unit: '%',
        format: (v) => v.toFixed(1)
      };
    case EParams.kParamAttack:
    case EParams.kParamDecay:
    case EParams.kParamRelease:
      return {
        unit: 'ms',
        format: (v) => v.toFixed(1)
      };
    case EParams.kParamOsc1Detune:
    case EParams.kParamOsc2Detune:
    case EParams.kParamOsc4Detune:
      return {
        unit: 'ct',
        format: (v) => v >= 0 ? `+${v.toFixed(0)}` : `${v.toFixed(0)}`
      };
    case EParams.kParamOsc1Octave:
    case EParams.kParamOsc2Octave:
    case EParams.kParamOsc4Octave:
      return {
        unit: '',
        format: (v) => v >= 0 ? `+${v.toFixed(0)}` : `${v.toFixed(0)}`
      };
    case EParams.kParamOsc1Wave:
    case EParams.kParamOsc2Wave:
    case EParams.kParamOsc3Wave:
    case EParams.kParamOsc4Wave:
    case EParams.kParamReverbRoomSize:
    case EParams.kParamReverbWidth:
      return {
        unit: '',
        format: (v) => v.toString()
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
    case EParams.kParamOsc1Mix:
    case EParams.kParamOsc2Mix:
    case EParams.kParamOsc3Mix:
    case EParams.kParamOsc4Mix:
    case EParams.kParamReverbDamp:
    case EParams.kParamReverbDry:
    case EParams.kParamReverbWet:
      return actualValue / 100.0; // 0 to 100 -> 0 to 1
    case EParams.kParamAttack:
      return Math.pow((actualValue - 1) / 1999, 1.0 / 3); // Inverse power curve: 1-2000ms, shape=3
    case EParams.kParamDecay:
      return Math.pow((actualValue - 1) / 3999, 1.0 / 3); // Inverse power curve: 1-4000ms, shape=3
    case EParams.kParamRelease:
      return Math.pow((actualValue - 2) / 7998, 1.0 / 3); // Inverse power curve: 2-8000ms, shape=3
    // Oscillator parameters
    case EParams.kParamOsc1Detune:
    case EParams.kParamOsc2Detune:
    case EParams.kParamOsc4Detune:
      return (actualValue / 100.0) + 0.5; // -50 to 50 -> 0 to 1
    case EParams.kParamOsc1Octave:
    case EParams.kParamOsc2Octave:
    case EParams.kParamOsc4Octave:
      return (actualValue - -2) / 4.0; // -2 to 2 -> 0 to 1
    case EParams.kParamOsc1Wave:
    case EParams.kParamOsc2Wave:
    case EParams.kParamOsc3Wave:
    case EParams.kParamOsc4Wave:
      return actualValue / 3.0; // 0-3 enum -> 0-1
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
  
  // Gain: 80 (0-100 %)
  defaults.set(EParams.kParamGain, 80 / 100);
  // Attack: 20 (1-2000 ms)
  defaults.set(EParams.kParamAttack, Math.pow((20 - 1) / 1999, 1.0 / 3));
  // Decay: 400 (1-4000 ms)
  defaults.set(EParams.kParamDecay, Math.pow((400 - 1) / 3999, 1.0 / 3));
  // Sustain: 70 (0-100 %)
  defaults.set(EParams.kParamSustain, 70 / 100);
  // Release: 600 (2-8000 ms)
  defaults.set(EParams.kParamRelease, Math.pow((600 - 2) / 7998, 1.0 / 3));
  // Osc1 Mix: 60 (0-100 %)
  defaults.set(EParams.kParamOsc1Mix, 60 / 100);
  // Osc2 Mix: 25 (0-100 %)
  defaults.set(EParams.kParamOsc2Mix, 25 / 100);
  // Osc3 Mix: 10 (0-100 %)
  defaults.set(EParams.kParamOsc3Mix, 10 / 100);
  // Osc4 Mix: 5 (0-100 %)
  defaults.set(EParams.kParamOsc4Mix, 5 / 100);
  // Osc1 Detune: 0 (-50-50 cents)
  defaults.set(EParams.kParamOsc1Detune, (0 / 100) + 0.5);
  // Osc2 Detune: 7 (-50-50 cents)
  defaults.set(EParams.kParamOsc2Detune, (7 / 100) + 0.5);
  // Osc4 Detune: 19 (-50-50 cents)
  defaults.set(EParams.kParamOsc4Detune, (19 / 100) + 0.5);
  // Osc1 Octave: 0 (-2-2)
  defaults.set(EParams.kParamOsc1Octave, actualToNormalized(EParams.kParamOsc1Octave, 0));
  // Osc2 Octave: 0 (-2-2)
  defaults.set(EParams.kParamOsc2Octave, actualToNormalized(EParams.kParamOsc2Octave, 0));
  // Osc4 Octave: 1 (-2-2)
  defaults.set(EParams.kParamOsc4Octave, actualToNormalized(EParams.kParamOsc4Octave, 1));
  // Osc1 Wave: 1 (0-3)
  defaults.set(EParams.kParamOsc1Wave, actualToNormalized(EParams.kParamOsc1Wave, 1));
  // Osc2 Wave: 1 (0-3)
  defaults.set(EParams.kParamOsc2Wave, actualToNormalized(EParams.kParamOsc2Wave, 1));
  // Osc3 Wave: 1 (0-3)
  defaults.set(EParams.kParamOsc3Wave, actualToNormalized(EParams.kParamOsc3Wave, 1));
  // Osc4 Wave: 3 (0-3)
  defaults.set(EParams.kParamOsc4Wave, actualToNormalized(EParams.kParamOsc4Wave, 3));
  // Reverb Room Size: 0.85 (0.3-0.99)
  defaults.set(EParams.kParamReverbRoomSize, (0.85 - 0.3) / 0.69);
  // Reverb Damp: 35 (0-100 %)
  defaults.set(EParams.kParamReverbDamp, 35 / 100);
  // Reverb Width: 0.9 (0-1)
  defaults.set(EParams.kParamReverbWidth, 0.9 / 1);
  // Reverb Dry: 70 (0-100 %)
  defaults.set(EParams.kParamReverbDry, 70 / 100);
  // Reverb Wet: 30 (0-100 %)
  defaults.set(EParams.kParamReverbWet, 30 / 100);

  
  return defaults;
}

export function getParamInputId(paramIdx: EParams): string {
  const paramNames: Record<EParams, string> = {
    [EParams.kParamGain]: 'paramGain',
    [EParams.kParamAttack]: 'paramAttack',
    [EParams.kParamDecay]: 'paramDecay',
    [EParams.kParamSustain]: 'paramSustain',
    [EParams.kParamRelease]: 'paramRelease',
    [EParams.kParamOsc1Mix]: 'paramOsc1Mix',
    [EParams.kParamOsc2Mix]: 'paramOsc2Mix',
    [EParams.kParamOsc3Mix]: 'paramOsc3Mix',
    [EParams.kParamOsc4Mix]: 'paramOsc4Mix',
    [EParams.kParamOsc1Detune]: 'paramOsc1Detune',
    [EParams.kParamOsc2Detune]: 'paramOsc2Detune',
    [EParams.kParamOsc3Detune]: 'paramOsc3Detune',
    [EParams.kParamOsc4Detune]: 'paramOsc4Detune',
    [EParams.kParamOsc1Octave]: 'paramOsc1Octave',
    [EParams.kParamOsc2Octave]: 'paramOsc2Octave',
    [EParams.kParamOsc3Octave]: 'paramOsc3Octave',
    [EParams.kParamOsc4Octave]: 'paramOsc4Octave',
    [EParams.kParamOsc1Wave]: 'paramOsc1Wave',
    [EParams.kParamOsc2Wave]: 'paramOsc2Wave',
    [EParams.kParamOsc3Wave]: 'paramOsc3Wave',
    [EParams.kParamOsc4Wave]: 'paramOsc4Wave',
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
