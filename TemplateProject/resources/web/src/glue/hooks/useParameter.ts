/**
 * Parameter Hook
 *
 * All-in-one hook for parameter controls. Provides:
 * - Value subscription (O(1) re-renders)
 * - DAW automation lifecycle (begin/end change)
 * - Value sending to DSP
 *
 * @sync Any changes to this hook's API must be reflected in hooksManifest.ts
 */

import { useSyncExternalStore, useCallback, useRef } from 'react';
import { parameterStore } from '@/glue/state/parameterStore';
import {
  beginParameterChange,
  endParameterChange,
  sendParameterValue,
} from '@/glue/iplugBridge/iplugBridge';
import { isUpdatingFromProcessor } from '@/glue/processorCallbacks/processorCallbacks';

/**
 * Hook for parameter controls.
 *
 * @param paramIdx - Parameter index (from EParams enum)
 * @returns { value, setValue, beginChange, endChange }
 *
 * @example Knob/Slider (continuous gesture):
 * ```tsx
 * const { value, setValue, beginChange, endChange } = useParameter(paramIdx);
 *
 * // On drag start
 * beginChange();
 *
 * // During drag
 * setValue(newValue);
 *
 * // On drag end
 * endChange();
 * ```
 *
 * @example Toggle/Dropdown (instant):
 * ```tsx
 * const { value, setValue, beginChange, endChange } = useParameter(paramIdx);
 *
 * const handleChange = (newValue: number) => {
 *   beginChange();
 *   setValue(newValue);
 *   endChange();
 * };
 * ```
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
