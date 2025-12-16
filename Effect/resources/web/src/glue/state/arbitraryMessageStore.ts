/**
 * Arbitrary Message Store
 *
 * Generic store for binary data from DSP, keyed by msgTag.
 * Used for spectrum, oscilloscope, envelope visualizations, etc.
 *
 * Usage:
 *   const message = useArbitraryMessage(EMsgTags.kMsgTagSpectrum);
 *   if (message) {
 *     const floatData = new Float32Array(message.data);
 *   }
 */

type Listener = () => void;

type ArbitraryMessage = {
  data: ArrayBuffer;
  timestamp: number;
};

function createArbitraryMessageStore() {
  const messages = new Map<number, ArbitraryMessage>();
  const listeners = new Map<number, Set<Listener>>();

  return {
    get: (msgTag: number): ArbitraryMessage | null => messages.get(msgTag) ?? null,

    set: (msgTag: number, data: ArrayBuffer): void => {
      messages.set(msgTag, { data, timestamp: Date.now() });
      listeners.get(msgTag)?.forEach(l => l());
    },

    subscribe: (msgTag: number, listener: Listener): (() => void) => {
      if (!listeners.has(msgTag)) listeners.set(msgTag, new Set());
      listeners.get(msgTag)!.add(listener);
      return () => listeners.get(msgTag)?.delete(listener);
    },
  };
}

export const arbitraryMessageStore = createArbitraryMessageStore();
