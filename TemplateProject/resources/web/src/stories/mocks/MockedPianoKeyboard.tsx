/**
 * Mocked PianoKeyboard for Storybook
 * Interactive piano keyboard without iPlug2 bridge dependency
 */

import React, { useState, useCallback } from 'react';

type MockedPianoKeyboardProps = {
  octaves?: number[];
  showOctaveLabels?: boolean;
  onNoteOn?: (note: number) => void;
  onNoteOff?: (note: number) => void;
};

const NOTES = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];
const IS_BLACK = [false, true, false, true, false, false, true, false, true, false, true, false];

// Colors as hex values to ensure they work in Storybook
const COLORS = {
  whiteKey: '#fffbeb',        // amber-50
  whiteKeyPressed: '#fde68a', // amber-200
  whiteKeyHover: '#fef3c7',   // amber-100
  blackKey: '#451a03',        // amber-950
  blackKeyPressed: '#92400e', // amber-800
  blackKeyHover: '#78350f',   // amber-900
  whiteBorder: '#92400e',     // amber-800
  blackBorder: '#d97706',     // amber-600
  labelText: '#fcd34d',       // amber-300
};

export function MockedPianoKeyboard({ 
  octaves = [3, 4, 5], 
  showOctaveLabels = true,
  onNoteOn,
  onNoteOff
}: MockedPianoKeyboardProps) {
  const [pressedKeys, setPressedKeys] = useState<Set<number>>(new Set());
  const [hoveredKey, setHoveredKey] = useState<number | null>(null);

  const playNote = useCallback((noteNum: number) => {
    if (pressedKeys.has(noteNum)) return;
    setPressedKeys(prev => new Set(prev).add(noteNum));
    onNoteOn?.(noteNum);
    console.log(`[Storybook] Note On: ${noteNum}`);
  }, [pressedKeys, onNoteOn]);

  const releaseNote = useCallback((noteNum: number) => {
    if (!pressedKeys.has(noteNum)) return;
    setPressedKeys(prev => {
      const next = new Set(prev);
      next.delete(noteNum);
      return next;
    });
    onNoteOff?.(noteNum);
    console.log(`[Storybook] Note Off: ${noteNum}`);
  }, [pressedKeys, onNoteOff]);

  const getWhiteKeyColor = (noteNum: number) => {
    if (pressedKeys.has(noteNum)) return COLORS.whiteKeyPressed;
    if (hoveredKey === noteNum) return COLORS.whiteKeyHover;
    return COLORS.whiteKey;
  };

  const getBlackKeyColor = (noteNum: number) => {
    if (pressedKeys.has(noteNum)) return COLORS.blackKeyPressed;
    if (hoveredKey === noteNum) return COLORS.blackKeyHover;
    return COLORS.blackKey;
  };

  return (
    <div>
      {showOctaveLabels && (
        <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: '8px' }}>
          <h3 style={{ color: 'white', fontSize: '12px', fontFamily: 'monospace', textTransform: 'uppercase', letterSpacing: '0.05em', margin: 0 }}>
            KEYBOARD
          </h3>
          <div style={{ color: COLORS.labelText, fontSize: '10px', fontFamily: 'monospace', textTransform: 'uppercase', letterSpacing: '0.05em' }}>
            OCTAVES {octaves[0]}-{octaves[octaves.length - 1]}
          </div>
        </div>
      )}

      {/* Piano keys */}
      <div style={{ position: 'relative', height: '64px' }}>
        {/* White keys */}
        <div style={{ display: 'flex' }}>
          {octaves.map((octave) => (
            NOTES.map((note, index) => {
              if (!IS_BLACK[index]) {
                const noteNum = octave * 12 + index;
                return (
                  <div
                    key={`white-${octave}-${index}`}
                    onMouseDown={() => playNote(noteNum)}
                    onMouseUp={() => releaseNote(noteNum)}
                    onMouseLeave={() => { releaseNote(noteNum); setHoveredKey(null); }}
                    onMouseEnter={() => setHoveredKey(noteNum)}
                    onTouchStart={(e) => { e.preventDefault(); playNote(noteNum); }}
                    onTouchEnd={(e) => { e.preventDefault(); releaseNote(noteNum); }}
                    style={{
                      width: '40px',
                      height: '64px',
                      backgroundColor: getWhiteKeyColor(noteNum),
                      border: `2px solid ${COLORS.whiteBorder}`,
                      cursor: 'pointer',
                      userSelect: 'none',
                      transition: 'background-color 0.1s',
                    }}
                  />
                );
              }
              return null;
            })
          ))}
        </div>

        {/* Black keys */}
        {octaves.map((octave) => {
          let whiteKeyIndex = 0;
          return NOTES.map((note, index) => {
            if (IS_BLACK[index]) {
              const noteNum = octave * 12 + index;
              const octaveOffset = (octave - octaves[0]) * 7 * 40;
              const keyOffset = whiteKeyIndex * 40 - 10;
              const totalOffset = octaveOffset + keyOffset;

              return (
                <div
                  key={`black-${octave}-${index}`}
                  onMouseDown={() => playNote(noteNum)}
                  onMouseUp={() => releaseNote(noteNum)}
                  onMouseLeave={() => { releaseNote(noteNum); setHoveredKey(null); }}
                  onMouseEnter={() => setHoveredKey(noteNum)}
                  onTouchStart={(e) => { e.preventDefault(); playNote(noteNum); }}
                  onTouchEnd={(e) => { e.preventDefault(); releaseNote(noteNum); }}
                  style={{
                    position: 'absolute',
                    top: 0,
                    left: `${totalOffset}px`,
                    width: '20px',
                    height: '48px',
                    backgroundColor: getBlackKeyColor(noteNum),
                    border: `2px solid ${COLORS.blackBorder}`,
                    cursor: 'pointer',
                    zIndex: 10,
                    userSelect: 'none',
                    transition: 'background-color 0.1s',
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

// Compact version for smaller spaces
type MockedMiniKeyboardProps = {
  startNote?: number;
  numKeys?: number;
  onNoteOn?: (note: number) => void;
  onNoteOff?: (note: number) => void;
};

export function MockedMiniKeyboard({
  startNote = 60,
  numKeys = 13,
  onNoteOn,
  onNoteOff
}: MockedMiniKeyboardProps) {
  const [pressedKeys, setPressedKeys] = useState<Set<number>>(new Set());

  const playNote = useCallback((noteNum: number) => {
    if (pressedKeys.has(noteNum)) return;
    setPressedKeys(prev => new Set(prev).add(noteNum));
    onNoteOn?.(noteNum);
  }, [pressedKeys, onNoteOn]);

  const releaseNote = useCallback((noteNum: number) => {
    if (!pressedKeys.has(noteNum)) return;
    setPressedKeys(prev => {
      const next = new Set(prev);
      next.delete(noteNum);
      return next;
    });
    onNoteOff?.(noteNum);
  }, [pressedKeys, onNoteOff]);

  const keys = [];
  for (let i = 0; i < numKeys; i++) {
    const noteNum = startNote + i;
    const noteIndex = noteNum % 12;
    const isBlack = IS_BLACK[noteIndex];
    keys.push({ noteNum, isBlack, isPressed: pressedKeys.has(noteNum) });
  }

  return (
    <div style={{ display: 'flex' }}>
      {keys.filter(k => !k.isBlack).map((key) => (
        <div
          key={key.noteNum}
          onMouseDown={() => playNote(key.noteNum)}
          onMouseUp={() => releaseNote(key.noteNum)}
          onMouseLeave={() => releaseNote(key.noteNum)}
          style={{
            width: '24px',
            height: '40px',
            backgroundColor: key.isPressed ? COLORS.whiteKeyPressed : COLORS.whiteKey,
            border: `1px solid ${COLORS.whiteBorder}`,
            cursor: 'pointer',
            userSelect: 'none',
            transition: 'background-color 0.1s',
          }}
        />
      ))}
    </div>
  );
}

