/**
 * Drum Pads Component
 *
 * Interactive drum pad grid for triggering drum sounds via MIDI.
 * Uses GM (General MIDI) standard drum note mapping.
 * Supports mouse/touch interaction and QWERTY keyboard input.
 */

import { useState, useCallback, useEffect, useRef } from 'react';

// GM Standard drum note mapping
const DRUM_PADS = [
  { name: 'KICK', note: 36, key: 'KeyA', color: 'orange' },
  { name: 'RIM', note: 37, key: 'KeyS', color: 'cyan' },
  { name: 'SNARE', note: 38, key: 'KeyD', color: 'cyan' },
  { name: 'CLAP', note: 39, key: 'KeyF', color: 'magenta' },
  { name: 'HH-C', note: 42, key: 'KeyG', color: 'orange' },
  { name: 'TOM', note: 45, key: 'KeyH', color: 'green' },
  { name: 'HH-O', note: 46, key: 'KeyJ', color: 'orange' },
] as const;

type DrumColor = 'cyan' | 'magenta' | 'green' | 'orange';

const COLOR_CLASSES: Record<DrumColor, { idle: string; active: string; glow: string }> = {
  cyan: {
    idle: 'from-cyan-900/50 to-cyan-950/80 border-cyan-500/40',
    active: 'from-cyan-500 to-cyan-600 border-cyan-400',
    glow: 'shadow-[0_0_20px_rgba(0,255,255,0.5)]',
  },
  magenta: {
    idle: 'from-pink-900/50 to-pink-950/80 border-pink-500/40',
    active: 'from-pink-500 to-pink-600 border-pink-400',
    glow: 'shadow-[0_0_20px_rgba(255,0,128,0.5)]',
  },
  green: {
    idle: 'from-green-900/50 to-green-950/80 border-green-500/40',
    active: 'from-green-500 to-green-600 border-green-400',
    glow: 'shadow-[0_0_20px_rgba(0,255,100,0.5)]',
  },
  orange: {
    idle: 'from-orange-900/50 to-orange-950/80 border-orange-500/40',
    active: 'from-orange-500 to-orange-600 border-orange-400',
    glow: 'shadow-[0_0_20px_rgba(255,165,0,0.5)]',
  },
};

export type DrumPadsProps = {
  activeNotes: Map<number, number> | Set<number>;
  onNoteOn: (noteNum: number, velocity: number) => void;
  onNoteOff: (noteNum: number, velocity: number) => void;
};

export function DrumPads({ activeNotes, onNoteOn, onNoteOff }: DrumPadsProps) {
  const [pressedPads, setPressedPads] = useState<Set<number>>(new Set());
  const keyToNoteMap = useRef<Map<string, number>>(new Map());

  const isPadActive = useCallback((note: number): boolean => {
    return pressedPads.has(note) || activeNotes.has(note);
  }, [pressedPads, activeNotes]);

  const triggerPad = useCallback((note: number) => {
    setPressedPads(prev => {
      if (prev.has(note)) return prev;
      onNoteOn(note, 127);
      return new Set(prev).add(note);
    });
  }, [onNoteOn]);

  const releasePad = useCallback((note: number) => {
    setPressedPads(prev => {
      if (!prev.has(note)) return prev;
      onNoteOff(note, 0);
      const next = new Set(prev);
      next.delete(note);
      return next;
    });
  }, [onNoteOff]);

  // QWERTY keyboard support
  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      if (document.activeElement && document.activeElement.tagName === 'INPUT') return;
      if (e.repeat) return;

      const pad = DRUM_PADS.find(p => p.key === e.code);
      if (pad && !keyToNoteMap.current.has(e.code)) {
        e.preventDefault();
        keyToNoteMap.current.set(e.code, pad.note);
        triggerPad(pad.note);
      }
    };

    const handleKeyUp = (e: KeyboardEvent) => {
      if (document.activeElement && document.activeElement.tagName === 'INPUT') return;

      const note = keyToNoteMap.current.get(e.code);
      if (note !== undefined) {
        e.preventDefault();
        keyToNoteMap.current.delete(e.code);
        releasePad(note);
      }
    };

    document.addEventListener('keydown', handleKeyDown);
    document.addEventListener('keyup', handleKeyUp);

    return () => {
      document.removeEventListener('keydown', handleKeyDown);
      document.removeEventListener('keyup', handleKeyUp);
    };
  }, [triggerPad, releasePad]);

  return (
    <div className="bg-gradient-to-b from-[#1a1a1a] to-[#0a0a0a] rounded-2xl p-4 border border-pink-500/30">
      {/* Header */}
      <div className="flex justify-between items-center mb-3">
        <div className="flex items-center gap-4">
          <div className="flex gap-0.5">
            <div className="w-0.5 h-4 bg-gradient-to-b from-pink-500 to-pink-700 rounded-full" />
            <div className="w-0.5 h-3 bg-gradient-to-b from-pink-500 to-pink-700 rounded-full mt-0.5" />
          </div>
          <h3 className="text-pink-200 text-xs font-bold uppercase tracking-[0.2em]">Drum Pads</h3>
        </div>
        <div className="text-pink-500/60 text-[10px] font-mono uppercase tracking-wider">
          A S D F G H J
        </div>
      </div>

      {/* Drum pads grid */}
      <div className="grid grid-cols-7 gap-2">
        {DRUM_PADS.map((pad) => {
          const isActive = isPadActive(pad.note);
          const colorClass = COLOR_CLASSES[pad.color as DrumColor];

          return (
            <div
              key={pad.note}
              onMouseDown={() => triggerPad(pad.note)}
              onMouseUp={() => releasePad(pad.note)}
              onMouseLeave={() => releasePad(pad.note)}
              onTouchStart={(e) => { e.preventDefault(); triggerPad(pad.note); }}
              onTouchEnd={(e) => { e.preventDefault(); releasePad(pad.note); }}
              className={`
                relative h-20 cursor-pointer select-none rounded-lg
                bg-gradient-to-b border-2 transition-all duration-75
                flex flex-col items-center justify-center gap-1
                ${isActive
                  ? `${colorClass.active} ${colorClass.glow} scale-95`
                  : `${colorClass.idle} hover:brightness-110`
                }
              `}
            >
              <span className={`text-xs font-bold tracking-wider ${isActive ? 'text-white' : 'text-gray-300'}`}>
                {pad.name}
              </span>
              <span className={`text-[10px] font-mono ${isActive ? 'text-white/80' : 'text-gray-500'}`}>
                {pad.key.replace('Key', '')}
              </span>
            </div>
          );
        })}
      </div>
    </div>
  );
}
