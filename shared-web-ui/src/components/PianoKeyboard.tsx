/**
 * Piano Keyboard Component
 *
 * Interactive piano keyboard for MIDI input testing.
 * Supports QWERTY keyboard input and mouse/touch interaction.
 */

import { useState, useCallback, useEffect, useRef } from 'react';

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

export type PianoKeyboardProps = {
  activeNotes: Map<number, number> | Set<number>;
  onNoteOn: (noteNum: number, velocity: number) => void;
  onNoteOff: (noteNum: number, velocity: number) => void;
};

export function PianoKeyboard({ activeNotes, onNoteOn, onNoteOff }: PianoKeyboardProps) {
  const [baseOctave, setBaseOctave] = useState(4);
  const [pressedKeys, setPressedKeys] = useState<Set<number>>(new Set());
  // Track which keyboard key is playing which note (to handle octave changes)
  const keyToNoteMap = useRef<Map<string, number>>(new Map());

  const NOTE_NAMES = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];
  const isBlack = [false, true, false, true, false, false, true, false, true, false, true, false];

  // Combine locally pressed keys with processor-echoed active notes
  const isKeyActive = useCallback((noteNum: number): boolean => {
    return pressedKeys.has(noteNum) || activeNotes.has(noteNum);
  }, [pressedKeys, activeNotes]);

  // Always display octaves 3, 4, 5, 6 (4 octaves)
  const displayOctaves = [3, 4, 5, 6];

  const playNoteNum = useCallback((noteNum: number) => {
    setPressedKeys(prev => {
      if (prev.has(noteNum)) return prev;
      onNoteOn(noteNum, 127);
      return new Set(prev).add(noteNum);
    });
  }, [onNoteOn]);

  const releaseNoteNum = useCallback((noteNum: number) => {
    setPressedKeys(prev => {
      if (!prev.has(noteNum)) return prev;
      onNoteOff(noteNum, 0);
      const next = new Set(prev);
      next.delete(noteNum);
      return next;
    });
  }, [onNoteOff]);

  // For mouse/touch on visual keyboard
  const playNote = useCallback((octave: number, noteOffset: number) => {
    playNoteNum(octave * 12 + noteOffset);
  }, [playNoteNum]);

  const releaseNote = useCallback((octave: number, noteOffset: number) => {
    releaseNoteNum(octave * 12 + noteOffset);
  }, [releaseNoteNum]);

  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      if (document.activeElement && document.activeElement.tagName === 'INPUT') return;
      if (e.repeat) return;

      if (e.code === 'KeyZ') {
        setBaseOctave(prev => Math.max(0, prev - 1));
        return;
      }
      if (e.code === 'KeyX') {
        setBaseOctave(prev => Math.min(7, prev + 1));
        return;
      }

      const noteOffset = QWERTY_TO_NOTE[e.code];
      if (noteOffset !== undefined && !keyToNoteMap.current.has(e.code)) {
        e.preventDefault();
        const noteNum = baseOctave * 12 + noteOffset;
        keyToNoteMap.current.set(e.code, noteNum);
        playNoteNum(noteNum);
      }
    };

    const handleKeyUp = (e: KeyboardEvent) => {
      if (document.activeElement && document.activeElement.tagName === 'INPUT') return;

      const noteNum = keyToNoteMap.current.get(e.code);
      if (noteNum !== undefined) {
        e.preventDefault();
        keyToNoteMap.current.delete(e.code);
        releaseNoteNum(noteNum);
      }
    };

    document.addEventListener('keydown', handleKeyDown);
    document.addEventListener('keyup', handleKeyUp);

    return () => {
      document.removeEventListener('keydown', handleKeyDown);
      document.removeEventListener('keyup', handleKeyUp);
    };
  }, [baseOctave, playNoteNum, releaseNoteNum]);

  // Key dimensions calculated to fit 4 octaves (28 white keys) within plugin width
  const WHITE_KEY_WIDTH = 34;
  const BLACK_KEY_WIDTH = 20;
  const KEY_GAP = 2;

  return (
    <div className="bg-gradient-to-b from-[#2a2a2a] to-[#1a1a1a] rounded-2xl p-4 border border-[#B8860B]/30">
      {/* Header */}
      <div className="flex justify-between items-center mb-3">
        <div className="flex items-center gap-4">
          {/* Art Deco accent */}
          <div className="flex gap-0.5">
            <div className="w-0.5 h-4 bg-gradient-to-b from-[#B8860B] to-[#8B6914] rounded-full" />
            <div className="w-0.5 h-3 bg-gradient-to-b from-[#B8860B] to-[#8B6914] rounded-full mt-0.5" />
          </div>
          <h3 className="text-[#E8D4B8] text-xs font-bold uppercase tracking-[0.2em]">Keyboard</h3>
        </div>
        <div className="text-[#B8860B]/60 text-[10px] font-mono uppercase tracking-wider">
          Z/X Shift Octave
        </div>
      </div>

      {/* Piano keys container */}
      <div className="relative h-24 bg-gradient-to-b from-[#1a1a1a] to-[#0a0a0a] rounded-xl p-2 border border-[#B8860B]/20 overflow-hidden">
        {/* Render all white keys first */}
        <div className="flex h-full">
          {displayOctaves.map((octave) => (
            NOTE_NAMES.map((note, index) => {
              if (!isBlack[index]) {
                const noteOffset = index;
                const noteNum = octave * 12 + noteOffset;
                const isPressed = isKeyActive(noteNum);
                return (
                  <div
                    key={`white-${octave}-${index}`}
                    onMouseDown={() => playNote(octave, noteOffset)}
                    onMouseUp={() => releaseNote(octave, noteOffset)}
                    onMouseLeave={() => releaseNote(octave, noteOffset)}
                    onTouchStart={(e) => { e.preventDefault(); playNote(octave, noteOffset); }}
                    onTouchEnd={(e) => { e.preventDefault(); releaseNote(octave, noteOffset); }}
                    className={`relative h-full cursor-pointer select-none rounded-b-md flex-shrink-0
                      ${isPressed
                        ? 'bg-gradient-to-b from-[#B8860B] to-[#8B6914]'
                        : 'bg-gradient-to-b from-[#F5F0E6] to-[#E8DCC8]'
                      }
                    `}
                    style={{
                      width: `${WHITE_KEY_WIDTH}px`,
                      boxShadow: isPressed
                        ? 'inset 0 2px 4px rgba(0,0,0,0.3)'
                        : 'inset 0 -4px 8px rgba(0,0,0,0.1), 0 2px 4px rgba(0,0,0,0.3)',
                      marginRight: '2px',
                    }}
                  />
                );
              }
              return null;
            })
          ))}
          {/* Final high C (C7) */}
          {(() => {
            const noteNum = 7 * 12; // C7
            const isPressed = isKeyActive(noteNum);
            return (
              <div
                key="white-7-0"
                onMouseDown={() => playNote(7, 0)}
                onMouseUp={() => releaseNote(7, 0)}
                onMouseLeave={() => releaseNote(7, 0)}
                onTouchStart={(e) => { e.preventDefault(); playNote(7, 0); }}
                onTouchEnd={(e) => { e.preventDefault(); releaseNote(7, 0); }}
                className={`relative h-full cursor-pointer select-none rounded-b-md flex-shrink-0
                  ${isPressed
                    ? 'bg-gradient-to-b from-[#B8860B] to-[#8B6914]'
                    : 'bg-gradient-to-b from-[#F5F0E6] to-[#E8DCC8]'
                  }
                `}
                style={{
                  width: `${WHITE_KEY_WIDTH}px`,
                  boxShadow: isPressed
                    ? 'inset 0 2px 4px rgba(0,0,0,0.3)'
                    : 'inset 0 -4px 8px rgba(0,0,0,0.1), 0 2px 4px rgba(0,0,0,0.3)',
                }}
              />
            );
          })()}
        </div>

        {/* Render all black keys absolutely positioned */}
        {displayOctaves.map((octave) => {
          let whiteKeyIndex = 0;
          return NOTE_NAMES.map((note, index) => {
            if (isBlack[index]) {
              const noteOffset = index;
              const noteNum = octave * 12 + noteOffset;
              const isPressed = isKeyActive(noteNum);
              const octaveOffset = (octave - 3) * 7 * (WHITE_KEY_WIDTH + KEY_GAP);
              const keyOffset = whiteKeyIndex * (WHITE_KEY_WIDTH + KEY_GAP) - (BLACK_KEY_WIDTH / 2);
              const totalOffset = octaveOffset + keyOffset + 8; // +8 for container padding

              return (
                <div
                  key={`black-${octave}-${index}`}
                  onMouseDown={() => playNote(octave, noteOffset)}
                  onMouseUp={() => releaseNote(octave, noteOffset)}
                  onMouseLeave={() => releaseNote(octave, noteOffset)}
                  onTouchStart={(e) => { e.preventDefault(); playNote(octave, noteOffset); }}
                  onTouchEnd={(e) => { e.preventDefault(); releaseNote(octave, noteOffset); }}
                  className={`absolute top-2 h-14 cursor-pointer z-10 select-none rounded-b-md
                    ${isPressed
                      ? 'bg-gradient-to-b from-[#B8860B] to-[#5a4510]'
                      : 'bg-gradient-to-b from-[#2a2a2a] to-[#1a1a1a]'
                    }
                  `}
                  style={{
                    width: `${BLACK_KEY_WIDTH}px`,
                    left: `${totalOffset}px`,
                    boxShadow: isPressed
                      ? 'inset 0 2px 4px rgba(0,0,0,0.5)'
                      : '0 4px 8px rgba(0,0,0,0.5), inset 0 -2px 4px rgba(255,255,255,0.05)',
                  }}
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
