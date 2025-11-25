/**
 * Parameter value conversion and display utilities
 * Reads from runtimeParameters for dynamic lookup
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
 */
export function normalizedToActual(paramIdx: number, normalizedValue: number): number {
  const metadata = getParameterMetadata(paramIdx);
  const value = Math.max(0, Math.min(1, normalizedValue));

  if (!metadata) {
    return value;
  }

  const { min, max, shape, shapeParameter } = metadata;
  const span = max - min;

  // Apply shape curve if needed
  let shaped = value;
  if (shape === 'ShapePowCurve' && shapeParameter > 0) {
    shaped = Math.pow(value, shapeParameter);
  } else if (shape === 'ShapeExp') {
    // Exponential shape (e.g., for frequency)
    shaped = (Math.exp(value * Math.log(1 + shapeParameter)) - 1) / shapeParameter;
  }

  return min + shaped * span;
}

/**
 * Convert actual parameter value to normalized value (0-1)
 * This is the inverse of normalizedToActual
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

  let normalized = (actualValue - min) / span;

  // Inverse shape curve if needed
  if (shape === 'ShapePowCurve' && shapeParameter > 0) {
    normalized = Math.pow(normalized, 1 / shapeParameter);
  } else if (shape === 'ShapeExp' && shapeParameter > 0) {
    normalized = Math.log(1 + normalized * shapeParameter) / Math.log(1 + shapeParameter);
  }

  return Math.max(0, Math.min(1, normalized));
}

/**
 * Get default normalized values matching C++ DSP defaults
 * Reads from runtimeParameters
 */
export function getDefaultNormalizedValues(): Map<number, number> {
  const defaults = new Map<number, number>();

  for (const param of runtimeParameters) {
    const { id, min, max, default: defaultValue } = param;
    const span = max - min;

    if (span > 0) {
      const normalized = (defaultValue - min) / span;
      defaults.set(id, normalized);
    } else {
      defaults.set(id, 0);
    }
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
