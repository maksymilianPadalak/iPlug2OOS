/**
 * Runtime Parameters Provider
 *
 * Provides runtimeParameters to shared components (like Dropdown)
 * so they can look up enum options by paramId.
 */

import React, { createContext, useContext } from 'react';

/**
 * Minimal type for parameters that shared components need.
 * Plugin-local RuntimeParameter types are supersets of this.
 */
export type RuntimeParameter = {
  id: number;
  name: string;
  min?: number;
  max?: number;
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

const RuntimeParametersContext = createContext<RuntimeParameter[]>([]);

export type RuntimeParametersProviderProps = {
  parameters: RuntimeParameter[];
  children: React.ReactNode;
};

export function RuntimeParametersProvider({ parameters, children }: RuntimeParametersProviderProps) {
  return (
    <RuntimeParametersContext.Provider value={parameters}>
      {children}
    </RuntimeParametersContext.Provider>
  );
}

export function useRuntimeParameters(): RuntimeParameter[] {
  return useContext(RuntimeParametersContext);
}
