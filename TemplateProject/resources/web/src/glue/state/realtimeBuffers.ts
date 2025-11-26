/**
 * Real-time Data Stores
 *
 * Provides stores for high-frequency audio data (meters).
 * Uses per-channel subscriptions to avoid unnecessary re-renders.
 */

type Listener = () => void;

type MeterData = { peak: number; rms: number };

function createMeterStore() {
  // Cached objects - only replaced when values actually change
  let left: MeterData = { peak: 0, rms: 0 };
  let right: MeterData = { peak: 0, rms: 0 };

  // Per-channel listeners for O(1) re-renders
  const leftListeners = new Set<Listener>();
  const rightListeners = new Set<Listener>();

  return {
    getLeft: (): MeterData => left,
    getRight: (): MeterData => right,

    update: (channel: number, peak: number, rms: number): void => {
      if (channel === 0) {
        // Only create new object and notify if values changed
        if (left.peak !== peak || left.rms !== rms) {
          left = { peak, rms };
          leftListeners.forEach((l) => l());
        }
      } else {
        if (right.peak !== peak || right.rms !== rms) {
          right = { peak, rms };
          rightListeners.forEach((l) => l());
        }
      }
    },

    subscribe: (channel: number, listener: Listener): (() => void) => {
      const listeners = channel === 0 ? leftListeners : rightListeners;
      listeners.add(listener);
      return () => listeners.delete(listener);
    },
  };
}

export const meterStore = createMeterStore();
