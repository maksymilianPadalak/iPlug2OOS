/**
 * Mock Bridge Provider for Storybook
 *
 * Initializes parameter store with defaults for component testing.
 */

import React, { useEffect } from 'react';
import { parameterStore } from '../glue/state/parameterStore';
import { runtimeParameters } from '../config/runtimeParameters';

type MockBridgeProviderProps = {
  children: React.ReactNode;
};

export function MockBridgeProvider({ children }: MockBridgeProviderProps) {
  // Initialize parameters with defaults from runtimeParameters
  // TODO: Handle normalization properly for non-linear shapes (ShapePowCurve, ShapeExp)
  // Currently only works for ShapeLinear. See IPlugParameter.h for shape implementations.
  useEffect(() => {
    runtimeParameters.forEach((param) => {
      const normalized = (param.default - param.min) / (param.max - param.min);
      parameterStore.set(param.id, normalized);
    });
  }, []);

  return <>{children}</>;
}
