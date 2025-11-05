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
      // Power curve: 1-1000ms
      return 1 + Math.pow(value, 3) * 999;
      
    case EParams.kParamSustain:
      return value * 100; // 0-100%
      
    case EParams.kParamRelease:
      return 2 + value * 998; // 2-1000ms
      
    case EParams.kParamLFORateHz:
      return 0.01 + value * 39.99; // 0.01-40Hz
      
    case EParams.kParamLFODepth:
      return value * 100; // 0-100%
      
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
      return {
        unit: '%',
        format: (v) => v.toFixed(1)
      };
      
    case EParams.kParamNoteGlideTime:
    case EParams.kParamAttack:
    case EParams.kParamDecay:
    case EParams.kParamRelease:
      return {
        unit: 'ms',
        format: (v) => v.toFixed(1)
      };
      
    case EParams.kParamLFORateHz:
      return {
        unit: 'Hz',
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
 * Get parameter input element ID
 */
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

