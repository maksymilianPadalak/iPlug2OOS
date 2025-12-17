/**
 * Hook for waveform visualization
 *
 * Subscribes to waveform buffer data for a specific control tag.
 */

import { useSyncExternalStore, useCallback } from 'react';
import { waveformStore } from '../state/waveformStore';

/**
 * Subscribe to waveform data for a specific control tag.
 *
 * @param ctrlTag - The control tag (e.g., kCtrlTagOsc1Waveform)
 * @returns { samples: Float32Array, timestamp: number }
 */
export function useWaveform(ctrlTag: number): { samples: Float32Array; timestamp: number } {
  const subscribe = useCallback(
    (onStoreChange: () => void) => waveformStore.subscribe(ctrlTag, onStoreChange),
    [ctrlTag]
  );

  const getSnapshot = useCallback(() => waveformStore.get(ctrlTag), [ctrlTag]);

  return useSyncExternalStore(subscribe, getSnapshot);
}
