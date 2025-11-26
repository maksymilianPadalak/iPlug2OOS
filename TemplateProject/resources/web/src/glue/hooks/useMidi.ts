/**
 * MIDI Hook
 *
 * Provides access to MIDI state from DSP (active notes, last message).
 * Uses useSyncExternalStore for O(1) re-renders.
 */

import { useSyncExternalStore, useCallback } from 'react';
import { midiStore } from '../state/midiStore';

export function useMidi() {
  const subscribe = useCallback(
    (onStoreChange: () => void) => midiStore.subscribe(onStoreChange),
    []
  );

  const getActiveNotes = useCallback(() => midiStore.getActiveNotes(), []);
  const getLastMessage = useCallback(() => midiStore.getLastMessage(), []);

  const activeNotes = useSyncExternalStore(subscribe, getActiveNotes);
  const lastMessage = useSyncExternalStore(subscribe, getLastMessage);

  const isNoteActive = useCallback(
    (noteNumber: number) => midiStore.isNoteActive(noteNumber),
    []
  );

  const getNoteVelocity = useCallback(
    (noteNumber: number) => midiStore.getNoteVelocity(noteNumber),
    []
  );

  return {
    activeNotes,
    lastMessage,
    isNoteActive,
    getNoteVelocity,
  };
}
