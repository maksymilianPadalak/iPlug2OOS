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
      return {
        unit: '%',
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

  
  return defaults;
}

export function getParamInputId(paramIdx: EParams): string {
  const paramNames: Record<EParams, string> = {
    [EParams.kParamGain]: 'paramGain',

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
