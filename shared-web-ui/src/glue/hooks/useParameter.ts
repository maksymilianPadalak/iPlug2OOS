/**
 * Parameter Hook
 *
 * All-in-one hook for parameter controls. Provides:
 * - Value subscription (O(1) re-renders)
 * - DAW automation lifecycle (begin/end change)
 * - Value sending to DSP
 * - Step snapping (values snap to nearest step)
 */

import { useSyncExternalStore, useCallback, useRef, useMemo } from 'react';
import { parameterStore } from '../state/parameterStore';
import {
  beginParameterChange,
  endParameterChange,
  sendParameterValue,
} from '../iplugBridge/iplugBridge';
import { isUpdatingFromProcessor } from '../processorCallbacks/processorCallbacks';
import { useRuntimeParameters } from '../RuntimeParametersProvider';
import { toNormalizedStep, snapToStep } from '../utils/normalization';

/**
 * Hook for parameter controls.
 *
 * @param paramIdx - Parameter index (from EParams enum)
 * @returns { value, setValue, beginChange, endChange }
 */
export function useParameter(paramIdx: number) {
  const isChanging = useRef(false);
  const runtimeParameters = useRuntimeParameters();

  // Get parameter metadata for step handling
  const paramMeta = useMemo(
    () => runtimeParameters.find(p => p.id === paramIdx),
    [runtimeParameters, paramIdx]
  );

  // Calculate normalized step size
  const normalizedStep = useMemo(() => {
    if (!paramMeta?.step || paramMeta.min === undefined || paramMeta.max === undefined) {
      return 0;
    }
    return toNormalizedStep(paramMeta.step, paramMeta.min, paramMeta.max);
  }, [paramMeta]);

  // DAW automation lifecycle
  const beginChange = useCallback(() => {
    if (!isChanging.current) {
      isChanging.current = true;
      beginParameterChange(paramIdx);
    }
  }, [paramIdx]);

  const endChange = useCallback(() => {
    if (isChanging.current) {
      isChanging.current = false;
      endParameterChange(paramIdx);
    }
  }, [paramIdx]);

  // Value subscription (O(1) re-renders)
  const subscribe = useCallback(
    (onStoreChange: () => void) => parameterStore.subscribe(paramIdx, onStoreChange),
    [paramIdx]
  );

  const getSnapshot = useCallback(
    () => parameterStore.get(paramIdx),
    [paramIdx]
  );

  const value = useSyncExternalStore(subscribe, getSnapshot);

  // Set value with step snapping and send to DSP
  const setValue = useCallback(
    (newValue: number) => {
      const snappedValue = normalizedStep > 0 ? snapToStep(newValue, normalizedStep) : newValue;
      parameterStore.set(paramIdx, snappedValue);
      if (!isUpdatingFromProcessor()) {
        sendParameterValue(paramIdx, snappedValue);
      }
    },
    [paramIdx, normalizedStep]
  );

  return { value, setValue, beginChange, endChange };
}
