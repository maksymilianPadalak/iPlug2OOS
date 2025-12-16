/**
 * Hook for meter visualization
 *
 * Per-channel subscriptions - left meter changes don't re-render right meter.
 * Stable object references - no re-render if values unchanged.
 *
 * @sync Any changes to this hook's API must be reflected in hooksManifest.ts
 */

import { useSyncExternalStore, useCallback } from 'react';
import { meterStore } from '@/glue/state/realtimeBuffers';

/**
 * Subscribe to meter data for a specific channel.
 *
 * @param channel - 0 for left, 1 for right
 * @returns { peak: number, rms: number } - stable reference, only changes when values change
 */
export function useMeter(channel: 0 | 1): { peak: number; rms: number } {
  const subscribe = useCallback(
    (onStoreChange: () => void) => meterStore.subscribe(channel, onStoreChange),
    [channel]
  );

  const getSnapshot = useCallback(
    () => (channel === 0 ? meterStore.getLeft() : meterStore.getRight()),
    [channel]
  );

  return useSyncExternalStore(subscribe, getSnapshot);
}
