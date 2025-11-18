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
      return value * 200; // 0 to 200
    // Delay
    case EParams.kParamDelayTime:
      return 1 + value * 1999; // 1 to 2000
    case EParams.kParamDelayFeedback:
      return value * 95; // 0 to 95
    case EParams.kParamDelayDry:
    case EParams.kParamDelayWet:
      return value * 100; // 0 to 100

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
    case EParams.kParamDelayFeedback:
    case EParams.kParamDelayDry:
    case EParams.kParamDelayWet:
      return {
        unit: '%',
        format: (v) => v.toFixed(1)
      };
    case EParams.kParamDelayTime:
      return {
        unit: 'ms',
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
      return actualValue / 200.0; // 0 to 200 -> 0 to 1
    // Delay
    case EParams.kParamDelayTime:
      return (actualValue - 1) / 1999.0; // 1 to 2000 -> 0 to 1
    case EParams.kParamDelayFeedback:
      return actualValue / 95.0; // 0 to 95 -> 0 to 1
    case EParams.kParamDelayDry:
    case EParams.kParamDelayWet:
      return actualValue / 100.0; // 0 to 100 -> 0 to 1

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
  
  // Gain: 100 (0-200 %)
  defaults.set(EParams.kParamGain, 100 / 200);
  // Delay Time: 250 (1-2000 ms)
  defaults.set(EParams.kParamDelayTime, (250 - 1) / 1999);
  // Delay Feedback: 30 (0-95 %)
  defaults.set(EParams.kParamDelayFeedback, 30 / 95);
  // Delay Dry: 50 (0-100 %)
  defaults.set(EParams.kParamDelayDry, 50 / 100);
  // Delay Wet: 50 (0-100 %)
  defaults.set(EParams.kParamDelayWet, 50 / 100);

  
  return defaults;
}

export function getParamInputId(paramIdx: EParams): string {
  const paramNames: Record<EParams, string> = {
    [EParams.kParamGain]: 'paramGain',
    [EParams.kParamDelayTime]: 'paramDelayTime',
    [EParams.kParamDelayFeedback]: 'paramDelayFeedback',
    [EParams.kParamDelayDry]: 'paramDelayDry',
    [EParams.kParamDelayWet]: 'paramDelayWet',

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
