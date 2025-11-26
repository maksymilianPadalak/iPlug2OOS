/**
 * Parameter value conversion and display utilities
 *
 * Handles all iPlug2 shape types:
 * - ShapeLinear: Linear mapping (default)
 * - ShapePowCurve: Power curve with configurable exponent
 * - ShapeExp: Exponential/logarithmic mapping (for frequency, etc.)
 *
 * @see IPlugParameter.h / IPlugParameter.cpp for reference implementations
 */

import { runtimeParameters, type RuntimeParameter } from '../config/runtimeParameters';

// Build lookup map from runtimeParameters
const parameterMap = new Map<number, RuntimeParameter>(
  runtimeParameters.map((p) => [p.id, p]),
);

/**
 * Get parameter metadata by index
 */
function getParameterMetadata(paramIdx: number): RuntimeParameter | null {
  return parameterMap.get(paramIdx) ?? null;
}

/**
 * Convert normalized parameter value (0-1) to display value
 */
export function normalizedToDisplay(paramIdx: number, normalizedValue: number): string {
  const metadata = getParameterMetadata(paramIdx);

  if (!metadata) {
    return normalizedValue.toFixed(2);
  }

  const actualValue = normalizedToActual(paramIdx, normalizedValue);
  const formatted = formatValue(actualValue, metadata);
  const unit = metadata.unit ? ` ${metadata.unit}` : '';

  return formatted + unit;
}

/**
 * Format value based on parameter type and step
 */
function formatValue(value: number, metadata: RuntimeParameter): string {
  // Integer types
  if (metadata.type === 'int' || metadata.type === 'bool') {
    return Math.round(value).toString();
  }

  // Enum types - use enum value names if available
  if (metadata.type === 'enum' && metadata.enumValues) {
    const index = Math.round(value);
    return metadata.enumValues[index] ?? value.toString();
  }

  // Float types - determine decimal places from step
  const step = metadata.step || 0.01;
  if (step >= 1) {
    return Math.round(value).toString();
  } else if (step >= 0.1) {
    return value.toFixed(1);
  } else if (step >= 0.01) {
    return value.toFixed(2);
  } else {
    return value.toFixed(3);
  }
}

/**
 * Convert normalized parameter value (0-1) to actual value
 *
 * Matches iPlug2 IParam::FromNormalized() behavior for all shape types.
 */
export function normalizedToActual(paramIdx: number, normalizedValue: number): number {
  const metadata = getParameterMetadata(paramIdx);
  const value = Math.max(0, Math.min(1, normalizedValue));

  if (!metadata) {
    return value;
  }

  const { min, max, shape, shapeParameter } = metadata;

  switch (shape) {
    case 'ShapePowCurve': {
      // IPlugParameter.cpp line 51-54:
      // return min + pow(value, mShape) * (max - min)
      const shaped = Math.pow(value, shapeParameter);
      return min + shaped * (max - min);
    }

    case 'ShapeExp': {
      // IPlugParameter.cpp line 61-74:
      // mAdd = log(min), mMul = log(max/min)
      // return exp(mAdd + value * mMul) = min * pow(max/min, value)
      const safeMin = Math.max(min, 0.00000001);
      return safeMin * Math.pow(max / safeMin, value);
    }

    case 'ShapeLinear':
    default: {
      // IPlugParameter.cpp line 26-29:
      // return min + value * (max - min)
      return min + value * (max - min);
    }
  }
}

/**
 * Convert actual parameter value to normalized value (0-1)
 *
 * Matches iPlug2 IParam::ToNormalized() behavior for all shape types.
 * This is the inverse of normalizedToActual.
 */
export function actualToNormalized(paramIdx: number, actualValue: number): number {
  const metadata = getParameterMetadata(paramIdx);

  if (!metadata) {
    return actualValue;
  }

  const { min, max, shape, shapeParameter } = metadata;
  const span = max - min;

  if (span <= 0) {
    return 0;
  }

  let normalized: number;

  switch (shape) {
    case 'ShapePowCurve': {
      // IPlugParameter.cpp line 56-59:
      // return pow((value - min) / (max - min), 1.0 / mShape)
      const linear = (actualValue - min) / span;
      normalized = Math.pow(linear, 1 / shapeParameter);
      break;
    }

    case 'ShapeExp': {
      // IPlugParameter.cpp line 77-80:
      // return (log(value) - mAdd) / mMul = log(value/min) / log(max/min)
      const safeMin = Math.max(min, 0.00000001);
      const safeValue = Math.max(actualValue, safeMin);
      normalized = Math.log(safeValue / safeMin) / Math.log(max / safeMin);
      break;
    }

    case 'ShapeLinear':
    default: {
      // IPlugParameter.cpp line 31-34:
      // return (value - min) / (max - min)
      normalized = (actualValue - min) / span;
      break;
    }
  }

  return Math.max(0, Math.min(1, normalized));
}

/**
 * Get default normalized values matching C++ DSP defaults
 *
 * Uses actualToNormalized to properly handle all shape types.
 */
export function getDefaultNormalizedValues(): Map<number, number> {
  const defaults = new Map<number, number>();

  for (const param of runtimeParameters) {
    defaults.set(param.id, actualToNormalized(param.id, param.default));
  }

  return defaults;
}

/**
 * Get parameter input element ID
 */
export function getParamInputId(paramIdx: number): string {
  const metadata = getParameterMetadata(paramIdx);

  if (!metadata) {
    return `param${paramIdx}`;
  }

  // Convert key like "kParamGain" to "paramGain"
  const key = metadata.key;
  if (key.startsWith('kParam')) {
    return 'param' + key.slice(6);
  }

  return key.toLowerCase();
}

/**
 * Get parameter value display element ID
 */
export function getParamValueId(paramIdx: number): string {
  return getParamInputId(paramIdx) + 'Value';
}
