/**
 * Runtime Parameters Provider
 *
 * Provides runtimeParameters to shared components (like Dropdown)
 * so they can look up enum options by paramId.
 * Also initializes parameterStore with default values on mount.
 */

import React, { createContext, useContext, useEffect } from 'react';
import { parameterStore } from './state/parameterStore';

/**
 * Minimal type for parameters that shared components need.
 * Plugin-local RuntimeParameter types are supersets of this.
 */
export type RuntimeParameter = {
  id: number;
  name: string;
  min?: number;
  max?: number;
  default?: number;
  unit?: string;
  shape?: string;
  shapeParameter?: number;
  enumValues?: string[] | null;
};

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

const RuntimeParametersContext = createContext<RuntimeParameter[]>([]);

export type RuntimeParametersProviderProps = {
  parameters: RuntimeParameter[];
  children: React.ReactNode;
};

export function RuntimeParametersProvider({ parameters, children }: RuntimeParametersProviderProps) {
  // Initialize parameterStore with normalized default values on mount
  useEffect(() => {
    const defaults = new Map<number, number>();
    parameters.forEach((param) => {
      if (param.default !== undefined && param.min !== undefined && param.max !== undefined) {
        const normalized = toNormalized(
          param.default,
          param.min,
          param.max,
          param.shape,
          param.shapeParameter
        );
        defaults.set(param.id, normalized);
      }
    });
    if (defaults.size > 0) {
      parameterStore.initializeValues(defaults);
    }
  }, [parameters]);

  return (
    <RuntimeParametersContext.Provider value={parameters}>
      {children}
    </RuntimeParametersContext.Provider>
  );
}

export function useRuntimeParameters(): RuntimeParameter[] {
  return useContext(RuntimeParametersContext);
}
