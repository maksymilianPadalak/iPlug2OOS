/**
 * Piano keyboard component - Berlin Brutalism Style
 */

import React, { useState, useCallback, useEffect } from 'react';
import { sendNoteOn, sendNoteOff } from '../communication/iplug-bridge';

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

export function PianoKeyboard() {
  const [octave, setOctave] = useState(4);
  const [pressedKeys, setPressedKeys] = useState<Set<number>>(new Set());

  const notes = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B', 'C'];
  const isBlack = [false, true, false, true, false, false, true, false, true, false, true, false, false];

  const playNote = useCallback((noteOffset: number) => {
    const noteNum = octave * 12 + noteOffset;
    if (pressedKeys.has(noteNum)) return;

    setPressedKeys(prev => new Set(prev).add(noteNum));
    sendNoteOn(noteNum, 127);
  }, [octave, pressedKeys]);

  const releaseNote = useCallback((noteOffset: number) => {
    const noteNum = octave * 12 + noteOffset;
    if (!pressedKeys.has(noteNum)) return;

    setPressedKeys(prev => {
      const next = new Set(prev);
      next.delete(noteNum);
      return next;
    });
    sendNoteOff(noteNum, 0);
  }, [octave, pressedKeys]);

  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      if (document.activeElement && document.activeElement.tagName === 'INPUT') return;

      if (e.code === 'KeyZ') {
        setOctave(prev => Math.max(0, prev - 1));
        setPressedKeys(new Set());
        return;
      }
      if (e.code === 'KeyX') {
        setOctave(prev => Math.min(10, prev + 1));
        setPressedKeys(new Set());
        return;
      }

      const noteOffset = QWERTY_TO_NOTE[e.code];
      if (noteOffset !== undefined) {
        e.preventDefault();
        playNote(noteOffset);
      }
    };

    const handleKeyUp = (e: KeyboardEvent) => {
      if (document.activeElement && document.activeElement.tagName === 'INPUT') return;

      const noteOffset = QWERTY_TO_NOTE[e.code];
      if (noteOffset !== undefined) {
        e.preventDefault();
        releaseNote(noteOffset);
      }
    };

    document.addEventListener('keydown', handleKeyDown);
    document.addEventListener('keyup', handleKeyUp);

    return () => {
      document.removeEventListener('keydown', handleKeyDown);
      document.removeEventListener('keyup', handleKeyUp);
    };
  }, [playNote, releaseNote]);

  let whiteKeyIndex = 0;

  return (
    <div>
      <div className="flex justify-between items-center mb-4">
        <h2 className="brutal-title text-xl">KEYBOARD</h2>
        <div className="text-white text-xs font-mono uppercase tracking-wider">
          Octave: C{octave} | Use QWERTY keys: A-W-S-E-D-F-T-G-Y-H-U-J-K-O-L (Z/X = octave)
        </div>
      </div>
      <div className="flex relative h-32 mx-auto max-w-4xl">
        {notes.map((note, index) => {
          const noteOffset = index;
          const isPressed = pressedKeys.has(octave * 12 + noteOffset);
          const isBlackKey = isBlack[index];

          if (isBlackKey) {
            const leftPos = whiteKeyIndex * 70 - 14;
            return (
              <div
                key={index}
                onMouseDown={() => playNote(noteOffset)}
                onMouseUp={() => releaseNote(noteOffset)}
                onMouseLeave={() => releaseNote(noteOffset)}
                onTouchStart={(e) => { e.preventDefault(); playNote(noteOffset); }}
                onTouchEnd={(e) => { e.preventDefault(); releaseNote(noteOffset); }}
                className={`absolute w-7 h-20 border-2 border-white cursor-pointer z-10 ${
                  isPressed ? 'bg-gray-600' : 'bg-black'
                }`}
                style={{
                  left: `${leftPos}px`,
                  userSelect: 'none',
                }}
              />
            );
          } else {
            whiteKeyIndex++;
            return (
              <div
                key={index}
                onMouseDown={() => playNote(noteOffset)}
                onMouseUp={() => releaseNote(noteOffset)}
                onMouseLeave={() => releaseNote(noteOffset)}
                onTouchStart={(e) => { e.preventDefault(); playNote(noteOffset); }}
                onTouchEnd={(e) => { e.preventDefault(); releaseNote(noteOffset); }}
                className={`relative inline-block w-[70px] h-32 border-2 border-black cursor-pointer ${
                  isPressed ? 'bg-gray-300' : 'bg-white'
                }`}
                style={{
                  userSelect: 'none',
                }}
              />
            );
          }
        })}
      </div>
    </div>
  );
}
