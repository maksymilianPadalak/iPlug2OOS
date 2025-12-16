/**
 * Arbitrary Message Hook
 *
 * Generic hook for any arbitrary message from DSP.
 * Use this for LFO waveforms, spectrum data, envelope followers, etc.
 *
 * @sync Any changes to this hook's API must be reflected in hooksManifest.ts
 *
 * @example
 * const message = useArbitraryMessage(EMsgTags.kMsgTagLFOWaveform);
 * if (message) {
 *   const floatData = new Float32Array(message.data);
 * }
 */

import { useSyncExternalStore, useCallback } from 'react';
import { arbitraryMessageStore } from '@/glue/state/arbitraryMessageStore';

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
