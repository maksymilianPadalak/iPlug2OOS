/**
 * Runtime Parameters Provider
 *
 * Provides runtimeParameters to shared components (like Dropdown)
 * so they can look up enum options by paramId.
 * Also initializes parameterStore with default values on mount.
 */

import React, { createContext, useContext, useEffect } from 'react';
import { parameterStore } from './state/parameterStore';
import {
  fromNormalized,
  toNormalized,
  toNormalizedStep,
  snapToStep,
} from './utils/normalization';

// Re-export normalization utilities for backwards compatibility
export { fromNormalized, toNormalized, toNormalizedStep, snapToStep };

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
  step?: number;
  unit?: string;
  shape?: string;
  shapeParameter?: number;
  enumValues?: string[] | null;
};

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
