/**
 * Fine-grained Parameter Store
 *
 * Provides O(1) re-renders for parameter changes by using per-parameter subscriptions.
 *
 * Performance:
 * - Change param 0 → only components using param 0 re-render
 * - setMany batches notifications to avoid mid-update re-renders
 */

import { getDefaultNormalizedValues } from '../../utils/parameter';

type Listener = () => void;

function createParameterStore(size = 256) {
  let values = new Float32Array(size);
  const listeners = new Map<number, Set<Listener>>();

  const growIfNeeded = (paramIdx: number) => {
    if (paramIdx >= values.length) {
      const newSize = Math.max(values.length * 2, paramIdx + 1);
      const newValues = new Float32Array(newSize);
      newValues.set(values);
      values = newValues;
    }
  };

  return {
    get: (paramIdx: number): number => {
      return paramIdx < values.length ? values[paramIdx] : 0;
    },

    set: (paramIdx: number, value: number): void => {
      growIfNeeded(paramIdx);
      if (values[paramIdx] === value) return;
      values[paramIdx] = value;
      listeners.get(paramIdx)?.forEach((l) => l());
    },

    setMany: (params: Map<number, number>): void => {
      // Collect which params actually changed
      const changed: number[] = [];

      params.forEach((value, paramIdx) => {
        growIfNeeded(paramIdx);
        if (values[paramIdx] !== value) {
          values[paramIdx] = value;
          changed.push(paramIdx);
        }
      });

      // Batch notify after all values are set
      changed.forEach((paramIdx) => {
        listeners.get(paramIdx)?.forEach((l) => l());
      });
    },

    subscribe: (paramIdx: number, listener: Listener): (() => void) => {
      if (!listeners.has(paramIdx)) listeners.set(paramIdx, new Set());
      listeners.get(paramIdx)!.add(listener);
      return () => listeners.get(paramIdx)?.delete(listener);
    },

    // For initialization - direct write without notifications
    initializeValues: (defaults: Map<number, number>): void => {
      defaults.forEach((value, paramIdx) => {
        growIfNeeded(paramIdx);
        values[paramIdx] = value;
      });
    },
  };
}

export const parameterStore = createParameterStore(256);

// Initialize with default values on module load
const defaults = getDefaultNormalizedValues();
parameterStore.initializeValues(defaults);
