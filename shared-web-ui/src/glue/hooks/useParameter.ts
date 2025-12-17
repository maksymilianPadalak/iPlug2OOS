/**
 * Parameter Hook
 *
 * All-in-one hook for parameter controls. Provides:
 * - Value subscription (O(1) re-renders)
 * - DAW automation lifecycle (begin/end change)
 * - Value sending to DSP
 */

import { useSyncExternalStore, useCallback, useRef } from 'react';
import { parameterStore } from '../state/parameterStore';
import {
  beginParameterChange,
  endParameterChange,
  sendParameterValue,
} from '../iplugBridge/iplugBridge';
import { isUpdatingFromProcessor } from '../processorCallbacks/processorCallbacks';

/**
 * Hook for parameter controls.
 *
 * @param paramIdx - Parameter index (from EParams enum)
 * @returns { value, setValue, beginChange, endChange }
 */
export function useParameter(paramIdx: number) {
  const isChanging = useRef(false);

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

  // Set value and send to DSP
  const setValue = useCallback(
    (newValue: number) => {
      parameterStore.set(paramIdx, newValue);
      if (!isUpdatingFromProcessor()) {
        sendParameterValue(paramIdx, newValue);
      }
    },
    [paramIdx]
  );

  return { value, setValue, beginChange, endChange };
}
