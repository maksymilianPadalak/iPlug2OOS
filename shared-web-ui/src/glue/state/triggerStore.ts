/**
 * Trigger Store
 *
 * Tracks which trigger IDs are currently active for UI visualization.
 * Used by VoiceCard to show pulse animation when triggered.
 */

type Listener = () => void;

let activeIds = new Map<number, number>(); // id -> timestamp
let version = 0;
const listeners = new Set<Listener>();

function notifyListeners() {
  version++;
  listeners.forEach((listener) => listener());
}

export const triggerStore = {
  /**
   * Trigger an ID (called when sending note on or triggering a voice)
   */
  trigger(id: number): void {
    activeIds = new Map(activeIds);
    activeIds.set(id, Date.now());
    notifyListeners();

    // Auto-clear after 100ms
    setTimeout(() => {
      triggerStore.release(id);
    }, 100);
  },

  /**
   * Release a trigger
   */
  release(id: number): void {
    if (activeIds.has(id)) {
      activeIds = new Map(activeIds);
      activeIds.delete(id);
      notifyListeners();
    }
  },

  /**
   * Check if an ID is currently active
   */
  isActive(id: number): boolean {
    return activeIds.has(id);
  },

  /**
   * Get current version (for change detection)
   */
  getVersion(): number {
    return version;
  },

  /**
   * Subscribe to changes
   */
  subscribe(listener: Listener): () => void {
    listeners.add(listener);
    return () => listeners.delete(listener);
  },
};

/**
 * Trigger a voice - call this when sending MIDI note or triggering a sound
 */
export function trigger(id: number): void {
  triggerStore.trigger(id);
}
