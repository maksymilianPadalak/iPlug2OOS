/**
 * MIDI keyboard handling
 */

import { sendNoteOn, sendNoteOff } from '../communication/iplug-bridge';
import { NoteNames, MidiStatus } from '../config/constants';

/**
 * QWERTY to note mapping (C major scale starting from C)
 */
const QWERTY_TO_NOTE: Record<string, number> = {
  'KeyA': 0,   // C
  'KeyW': 1,   // C#
  'KeyS': 2,   // D
  'KeyE': 3,   // D#
  'KeyD': 4,   // E
  'KeyF': 5,   // F
  'KeyT': 6,   // F#
  'KeyG': 7,   // G
  'KeyY': 8,   // G#
  'KeyH': 9,   // A
  'KeyU': 10,  // A#
  'KeyJ': 11,  // B
  'KeyK': 12,  // C (octave)
  'KeyO': 13,  // C#
  'KeyL': 14   // D
};

let keyboardOctave = 4; // C4 = 60
const pressedKeys = new Set<number>();
const keyboardKeys: Record<number, HTMLElement> = {};

/**
 * Initialize piano keyboard
 */
export function initKeyboard(): void {
  const keyboard = document.getElementById('pianoKeyboard');
  if (!keyboard) return;

  const notes = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B', 'C'];
  const isBlack = [false, true, false, true, false, false, true, false, true, false, true, false, false];

  let whiteKeyIndex = 0;
  notes.forEach((note, index) => {
    const key = document.createElement('div');
    const noteNum = index;
    key.dataset.note = noteNum.toString();
    key.dataset.noteName = note;

    if (isBlack[index]) {
      const leftPos = whiteKeyIndex * 70 - 14;
      key.style.cssText = `
        width: 28px;
        height: 70px;
        background: #000000;
        border: 1px solid #ffffff;
        position: absolute;
        left: ${leftPos}px;
        z-index: 2;
        cursor: pointer;
        border-radius: 0 0 4px 4px;
      `;
    } else {
      key.style.cssText = `
        width: 70px;
        height: 120px;
        background: #ffffff;
        border: 1px solid #000000;
        position: relative;
        display: inline-block;
        cursor: pointer;
        border-radius: 0 0 4px 4px;
      `;
      whiteKeyIndex++;
    }

    key.addEventListener('mousedown', (e) => { e.preventDefault(); playNote(noteNum); });
    key.addEventListener('mouseup', () => releaseNote(noteNum));
    key.addEventListener('mouseleave', () => releaseNote(noteNum));
    key.addEventListener('touchstart', (e) => { e.preventDefault(); playNote(noteNum); });
    key.addEventListener('touchend', (e) => { e.preventDefault(); releaseNote(noteNum); });
    key.addEventListener('touchcancel', (e) => { e.preventDefault(); releaseNote(noteNum); });

    keyboard.appendChild(key);
    keyboardKeys[noteNum] = key;
  });

  updateOctaveDisplay();
}

/**
 * Play a note
 */
function playNote(noteOffset: number): void {
  const noteNum = keyboardOctave * 12 + noteOffset;
  if (pressedKeys.has(noteNum)) return;

  pressedKeys.add(noteNum);
  sendNoteOn(noteNum, 127);

  // Visual feedback
  const key = keyboardKeys[noteOffset];
  if (key) {
    const isBlack = key.dataset.noteName?.includes('#');
    key.style.background = isBlack ? '#444444' : '#cccccc';
  }
}

/**
 * Release a note
 */
function releaseNote(noteOffset: number): void {
  const noteNum = keyboardOctave * 12 + noteOffset;
  if (!pressedKeys.has(noteNum)) return;

  pressedKeys.delete(noteNum);
  sendNoteOff(noteNum, 0);

  // Visual feedback
  const key = keyboardKeys[noteOffset];
  if (key) {
    const isBlack = key.dataset.noteName?.includes('#');
    key.style.background = isBlack ? '#000000' : '#ffffff';
  }
}

/**
 * Update octave display
 */
function updateOctaveDisplay(): void {
  const octaveEl = document.getElementById('octaveDisplay');
  if (octaveEl) {
    octaveEl.textContent = 'C' + keyboardOctave;
  }
}

/**
 * Setup keyboard event handlers
 */
export function setupKeyboardHandlers(): void {
  document.addEventListener('keydown', (e) => {
    if (document.activeElement && document.activeElement.tagName === 'INPUT') return;

    // Octave switching
    if (e.code === 'KeyZ') {
      keyboardOctave = Math.max(0, keyboardOctave - 1);
      updateOctaveDisplay();
      // Release all current notes
      pressedKeys.forEach(noteNum => {
        const noteOffset = noteNum % 12;
        releaseNote(noteOffset);
      });
      return;
    }
    if (e.code === 'KeyX') {
      keyboardOctave = Math.min(10, keyboardOctave + 1);
      updateOctaveDisplay();
      // Release all current notes
      pressedKeys.forEach(noteNum => {
        const noteOffset = noteNum % 12;
        releaseNote(noteOffset);
      });
      return;
    }

    // Note playing
    const noteOffset = QWERTY_TO_NOTE[e.code];
    if (noteOffset !== undefined) {
      e.preventDefault();
      playNote(noteOffset);
    }
  });

  document.addEventListener('keyup', (e) => {
    if (document.activeElement && document.activeElement.tagName === 'INPUT') return;

    const noteOffset = QWERTY_TO_NOTE[e.code];
    if (noteOffset !== undefined) {
      e.preventDefault();
      releaseNote(noteOffset);
    }
  });
}

/**
 * Get current octave
 */
export function getOctave(): number {
  return keyboardOctave;
}

