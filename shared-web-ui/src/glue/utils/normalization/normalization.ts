/**
 * Normalization Utilities
 *
 * Pure functions for converting between normalized (0-1) and actual parameter values.
 * Matches iPlug2's IParam normalization behavior exactly.
 */

/**
 * Convert normalized value (0-1) to actual value using parameter shape.
 * Matches iPlug2's IParam::FromNormalized behavior exactly.
 *
 * From IPlugParameter.cpp:
 * - ShapeLinear: min + value * (max - min)
 * - ShapePowCurve: min + pow(value, mShape) * (max - min)
 * - ShapeExp: exp(mAdd + value * mMul) where mAdd=log(safeMin), mMul=log(max/safeMin)
 */
export function fromNormalized(
  normalized: number,
  min: number,
  max: number,
  shape: string = 'ShapeLinear',
  shapeParameter: number = 0
): number {
  const range = max - min;

  switch (shape) {
    case 'ShapeLinear':
      return min + normalized * range;

    case 'ShapePowCurve':
      // shapeParameter is the exponent (mShape in iPlug2)
      return min + Math.pow(normalized, shapeParameter) * range;

    case 'ShapeExp': {
      // Matches iPlug2 exactly: clamp min to 0.00000001 if <= 0
      const safeMin = min <= 0 ? 0.00000001 : min;
      const mAdd = Math.log(safeMin);
      const mMul = Math.log(max / safeMin);
      return Math.exp(mAdd + normalized * mMul);
    }

    default:
      return min + normalized * range;
  }
}

/**
 * Convert actual value to normalized (0-1) using parameter shape.
 * Inverse of fromNormalized.
 */
export function toNormalized(
  value: number,
  min: number,
  max: number,
  shape: string = 'ShapeLinear',
  shapeParameter: number = 0
): number {
  const range = max - min;
  if (range === 0) return 0;

  switch (shape) {
    case 'ShapeLinear':
      return (value - min) / range;

    case 'ShapePowCurve':
      return Math.pow((value - min) / range, 1 / shapeParameter);

    case 'ShapeExp': {
      const safeMin = min <= 0 ? 0.00000001 : min;
      const mAdd = Math.log(safeMin);
      const mMul = Math.log(max / safeMin);
      return (Math.log(value) - mAdd) / mMul;
    }

    default:
      return (value - min) / range;
  }
}

/**
 * Convert step size to normalized step size.
 * @param step - Step size in actual value units
 * @param min - Parameter minimum value
 * @param max - Parameter maximum value
 * @returns Normalized step size (0-1 range)
 */
export function toNormalizedStep(step: number, min: number, max: number): number {
  const range = max - min;
  if (range === 0) return 0;
  return step / range;
}

/**
 * Snap a normalized value to the nearest step.
 * @param value - Normalized value (0-1)
 * @param normalizedStep - Normalized step size
 * @returns Value snapped to nearest step, clamped to 0-1
 */
export function snapToStep(value: number, normalizedStep: number): number {
  if (normalizedStep <= 0) return value;
  const snapped = Math.round(value / normalizedStep) * normalizedStep;
  return Math.max(0, Math.min(1, snapped));
}
