/**
 * Piano keyboard component - Shows multiple octaves
 */

import React, { useState, useCallback, useEffect } from 'react';
import { sendNoteOn, sendNoteOff } from '../../communication/iplug-bridge';

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

  // Always display octaves 3, 4, 5 (fixed display)
  const displayOctaves = [3, 4, 5];

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
        // Play in the current baseOctave
        playNote(baseOctave, noteOffset);
      }
    };

    const handleKeyUp = (e: KeyboardEvent) => {
      if (document.activeElement && document.activeElement.tagName === 'INPUT') return;

      const noteOffset = QWERTY_TO_NOTE[e.code];
      if (noteOffset !== undefined) {
        e.preventDefault();
        // Release from the current baseOctave
        releaseNote(baseOctave, noteOffset);
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
        <div className="text-amber-300 text-[10px] font-mono uppercase tracking-wider">
          OCTAVES 3-5 (Z/X = SHIFT)
        </div>
      </div>
      
      {/* Render all 3 octaves in one continuous line */}
      <div className="relative h-16">
        {/* Render all white keys first */}
        <div className="flex">
          {displayOctaves.map((octave) => (
            notes.map((note, index) => {
              if (!isBlack[index]) {
                const noteOffset = index;
                const noteNum = octave * 12 + noteOffset;
                const isPressed = pressedKeys.has(noteNum);
                return (
                  <div
                    key={`white-${octave}-${index}`}
                    onMouseDown={() => playNote(octave, noteOffset)}
                    onMouseUp={() => releaseNote(octave, noteOffset)}
                    onMouseLeave={() => releaseNote(octave, noteOffset)}
                    onTouchStart={(e) => { e.preventDefault(); playNote(octave, noteOffset); }}
                    onTouchEnd={(e) => { e.preventDefault(); releaseNote(octave, noteOffset); }}
                    className={`w-10 h-16 border-2 border-amber-800 cursor-pointer select-none ${
                      isPressed ? 'bg-amber-200' : 'bg-amber-50'
                    }`}
                  />
                );
              }
              return null;
            })
          ))}
        </div>
        
        {/* Render all black keys absolutely positioned */}
        {displayOctaves.map((octave) => {
          let whiteKeyIndex = 0;
          return notes.map((note, index) => {
            if (isBlack[index]) {
              const noteOffset = index;
              const noteNum = octave * 12 + noteOffset;
              const isPressed = pressedKeys.has(noteNum);
              // Calculate position: octave offset (octave - 3) * 7 white keys * 40px + white key index * 40px - 10px
              const octaveOffset = (octave - 3) * 7 * 40;
              const keyOffset = whiteKeyIndex * 40 - 10;
              const totalOffset = octaveOffset + keyOffset;
              
              return (
                <div
                  key={`black-${octave}-${index}`}
                  onMouseDown={() => playNote(octave, noteOffset)}
                  onMouseUp={() => releaseNote(octave, noteOffset)}
                  onMouseLeave={() => releaseNote(octave, noteOffset)}
                  onTouchStart={(e) => { e.preventDefault(); playNote(octave, noteOffset); }}
                  onTouchEnd={(e) => { e.preventDefault(); releaseNote(octave, noteOffset); }}
                  className={`absolute top-0 w-5 h-12 border-2 border-amber-600 cursor-pointer z-10 select-none ${
                    isPressed ? 'bg-amber-800' : 'bg-amber-950'
                  }`}
                  style={{ left: `${totalOffset}px` }}
                />
              );
            } else {
              whiteKeyIndex++;
              return null;
            }
          });
        })}
      </div>
    </div>
  );
}

