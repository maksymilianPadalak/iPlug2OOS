/**
 * Arbitrary Message Hook
 *
 * Generic hook for any arbitrary message from DSP.
 * Use this for LFO waveforms, spectrum data, envelope followers, etc.
 */

import { useSyncExternalStore, useCallback } from 'react';
import { arbitraryMessageStore } from '../state/arbitraryMessageStore';

type ArbitraryMessage = {
  data: ArrayBuffer;
  timestamp: number;
};

export function useArbitraryMessage(msgTag: number): ArbitraryMessage | null {
  const subscribe = useCallback(
    (onStoreChange: () => void) => arbitraryMessageStore.subscribe(msgTag, onStoreChange),
    [msgTag]
  );

  const getSnapshot = useCallback(
    () => arbitraryMessageStore.get(msgTag),
    [msgTag]
  );

  return useSyncExternalStore(subscribe, getSnapshot);
}
