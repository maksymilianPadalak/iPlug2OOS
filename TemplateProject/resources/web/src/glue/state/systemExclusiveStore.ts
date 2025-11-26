/**
 * System Exclusive State Store
 *
 * Stores System Exclusive messages from DSP.
 * Uses subscription pattern for O(1) re-renders.
 */

type Listener = () => void;

type SystemExclusiveMessage = {
  data: ArrayBuffer;
  timestamp: number;
};

function createSystemExclusiveStore() {
  let lastMessage: SystemExclusiveMessage | null = null;
  const listeners = new Set<Listener>();

  return {
    getLastMessage: (): SystemExclusiveMessage | null => lastMessage,

    handleMessage: (data: ArrayBuffer): void => {
      lastMessage = {
        data,
        timestamp: Date.now(),
      };
      listeners.forEach(l => l());
    },

    subscribe: (listener: Listener): (() => void) => {
      listeners.add(listener);
      return () => listeners.delete(listener);
    },
  };
}

export const systemExclusiveStore = createSystemExclusiveStore();
