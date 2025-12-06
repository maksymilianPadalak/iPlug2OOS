/**
 * Waveform Buffer Store
 *
 * Stores oscillator waveform samples for visualization.
 * Uses per-control-tag subscriptions to avoid unnecessary re-renders.
 */

type Listener = () => void;

type WaveformData = {
  samples: Float32Array;
  timestamp: number;
};

function createWaveformStore() {
  const buffers = new Map<number, WaveformData>();
  const listeners = new Map<number, Set<Listener>>();

  const emptyData: WaveformData = { samples: new Float32Array(0), timestamp: 0 };

  return {
    get: (ctrlTag: number): WaveformData => buffers.get(ctrlTag) ?? emptyData,

    update: (ctrlTag: number, samples: Float32Array): void => {
      buffers.set(ctrlTag, { samples, timestamp: Date.now() });
      listeners.get(ctrlTag)?.forEach((l) => l());
    },

    subscribe: (ctrlTag: number, listener: Listener): (() => void) => {
      if (!listeners.has(ctrlTag)) listeners.set(ctrlTag, new Set());
      listeners.get(ctrlTag)!.add(listener);
      return () => listeners.get(ctrlTag)?.delete(listener);
    },
  };
}

export const waveformStore = createWaveformStore();
