/**
 * Piano keyboard component - Shows multiple octaves
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
  const [baseOctave, setBaseOctave] = useState(3);
  const [pressedKeys, setPressedKeys] = useState<Set<number>>(new Set());

  const notes = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];
  const isBlack = [false, true, false, true, false, false, true, false, true, false, true, false];

  // Show 3 octaves starting from baseOctave
  const octavesToShow = [baseOctave, baseOctave + 1, baseOctave + 2];

  const playNote = useCallback((octave: number, noteOffset: number) => {
    const noteNum = octave * 12 + noteOffset;
    if (pressedKeys.has(noteNum)) return;

    setPressedKeys(prev => new Set(prev).add(noteNum));
    sendNoteOn(noteNum, 127);
  }, [pressedKeys]);

  const releaseNote = useCallback((octave: number, noteOffset: number) => {
    const noteNum = octave * 12 + noteOffset;
    if (!pressedKeys.has(noteNum)) return;

    setPressedKeys(prev => {
      const next = new Set(prev);
      next.delete(noteNum);
      return next;
    });
    sendNoteOff(noteNum, 0);
  }, [pressedKeys]);

  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      if (document.activeElement && document.activeElement.tagName === 'INPUT') return;

      if (e.code === 'KeyZ') {
        setBaseOctave(prev => Math.max(0, prev - 1));
        // Release all notes when changing octave
        pressedKeys.forEach(noteNum => {
          sendNoteOff(noteNum, 0);
        });
        setPressedKeys(new Set());
        return;
      }
      if (e.code === 'KeyX') {
        setBaseOctave(prev => Math.min(7, prev + 1));
        // Release all notes when changing octave
        pressedKeys.forEach(noteNum => {
          sendNoteOff(noteNum, 0);
        });
        setPressedKeys(new Set());
        return;
      }

      const noteOffset = QWERTY_TO_NOTE[e.code];
      if (noteOffset !== undefined) {
        e.preventDefault();
        // Play in middle octave (baseOctave + 1)
        playNote(baseOctave + 1, noteOffset);
      }
    };

    const handleKeyUp = (e: KeyboardEvent) => {
      if (document.activeElement && document.activeElement.tagName === 'INPUT') return;

      const noteOffset = QWERTY_TO_NOTE[e.code];
      if (noteOffset !== undefined) {
        e.preventDefault();
        // Release from middle octave (baseOctave + 1)
        releaseNote(baseOctave + 1, noteOffset);
      }
    };

    document.addEventListener('keydown', handleKeyDown);
    document.addEventListener('keyup', handleKeyUp);

    return () => {
      document.removeEventListener('keydown', handleKeyDown);
      document.removeEventListener('keyup', handleKeyUp);
    };
  }, [baseOctave, playNote, releaseNote, pressedKeys]);

  return (
    <div>
      <div className="flex justify-between items-center mb-2">
        <h3 className="text-white text-xs font-mono uppercase tracking-wider">KEYBOARD</h3>
        <div className="text-gray-400 text-[10px] font-mono uppercase tracking-wider">
          Octaves {baseOctave}-{baseOctave + 2} (Z/X = shift)
        </div>
      </div>
      
      {/* Render 3 octaves */}
      {octavesToShow.map((octave, octaveIdx) => (
        <div key={octave} className="mb-2">
          <div className="text-gray-400 text-[9px] font-mono uppercase mb-1">
            Octave {octave}
          </div>
          <OctaveKeyboard
            octave={octave}
            notes={notes}
            isBlack={isBlack}
            pressedKeys={pressedKeys}
            onNoteDown={(noteOffset) => playNote(octave, noteOffset)}
            onNoteUp={(noteOffset) => releaseNote(octave, noteOffset)}
          />
        </div>
      ))}
    </div>
  );
}

function OctaveKeyboard({
  octave,
  notes,
  isBlack,
  pressedKeys,
  onNoteDown,
  onNoteUp,
}: {
  octave: number;
  notes: string[];
  isBlack: boolean[];
  pressedKeys: Set<number>;
  onNoteDown: (noteOffset: number) => void;
  onNoteUp: (noteOffset: number) => void;
}) {
  let whiteKeyIndex = 0;

  return (
    <div className="flex relative h-16 mx-auto max-w-2xl">
      {notes.map((note, index) => {
        const noteOffset = index;
        const noteNum = octave * 12 + noteOffset;
        const isPressed = pressedKeys.has(noteNum);
        const isBlackKey = isBlack[index];

        if (isBlackKey) {
          const leftPos = whiteKeyIndex * 40 - 10;
          return (
            <div
              key={index}
              onMouseDown={() => onNoteDown(noteOffset)}
              onMouseUp={() => onNoteUp(noteOffset)}
              onMouseLeave={() => onNoteUp(noteOffset)}
              onTouchStart={(e) => { e.preventDefault(); onNoteDown(noteOffset); }}
              onTouchEnd={(e) => { e.preventDefault(); onNoteUp(noteOffset); }}
              className={`absolute w-5 h-12 border-2 border-white cursor-pointer z-10 ${
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
              onMouseDown={() => onNoteDown(noteOffset)}
              onMouseUp={() => onNoteUp(noteOffset)}
              onMouseLeave={() => onNoteUp(noteOffset)}
              onTouchStart={(e) => { e.preventDefault(); onNoteDown(noteOffset); }}
              onTouchEnd={(e) => { e.preventDefault(); onNoteUp(noteOffset); }}
              className={`relative inline-block w-10 h-16 border-2 border-black cursor-pointer ${
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
  );
}
