/**
 * MIDI State Store
 *
 * Stores MIDI note state from DSP.
 * Only notifies listeners when note state actually changes (not for CC, etc.)
 */

type Listener = () => void;

type MidiMessage = {
  statusByte: number;
  dataByte1: number;
  dataByte2: number;
  timestamp: number;
};

function createMidiStore() {
  // Use a plain object for activeNotes - replaced on change for stable references
  let activeNotes = new Map<number, number>(); // noteNumber -> velocity
  let lastMessage: MidiMessage | null = null;
  const listeners = new Set<Listener>();

  return {
    getActiveNotes: (): Map<number, number> => activeNotes,
    getLastMessage: (): MidiMessage | null => lastMessage,
    isNoteActive: (noteNumber: number): boolean => activeNotes.has(noteNumber),
    getNoteVelocity: (noteNumber: number): number => activeNotes.get(noteNumber) ?? 0,

    handleMessage: (statusByte: number, dataByte1: number, dataByte2: number): void => {
      lastMessage = {
        statusByte,
        dataByte1,
        dataByte2,
        timestamp: Date.now(),
      };

      // Parse MIDI message type (high nibble of status byte)
      const messageType = statusByte & 0xf0;
      const noteNumber = dataByte1;
      const velocity = dataByte2;

      let noteStateChanged = false;

      if (messageType === 0x90 && velocity > 0) {
        // Note On - only notify if note wasn't already active with same velocity
        if (activeNotes.get(noteNumber) !== velocity) {
          // Create new Map to ensure stable reference changes
          activeNotes = new Map(activeNotes);
          activeNotes.set(noteNumber, velocity);
          noteStateChanged = true;
        }
      } else if (messageType === 0x80 || (messageType === 0x90 && velocity === 0)) {
        // Note Off - only notify if note was actually active
        if (activeNotes.has(noteNumber)) {
          activeNotes = new Map(activeNotes);
          activeNotes.delete(noteNumber);
          noteStateChanged = true;
        }
      }
      // CC, pitch bend, etc. - don't notify (keyboard doesn't care)

      if (noteStateChanged) {
        listeners.forEach((l) => l());
      }
    },

    subscribe: (listener: Listener): (() => void) => {
      listeners.add(listener);
      return () => listeners.delete(listener);
    },
  };
}

export const midiStore = createMidiStore();
