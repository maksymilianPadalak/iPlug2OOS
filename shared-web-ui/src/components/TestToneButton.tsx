/**
 * TestToneButton Component
 *
 * Button for toggling microphone input for audio testing.
 */

import React from 'react';

export type TestToneButtonProps = {
  isPlaying: boolean;
  onToggle: () => void;
};

export function TestToneButton({ isPlaying, onToggle }: TestToneButtonProps) {
  return (
    <div className="flex flex-col items-center gap-3">
      <label className="text-cyan-200 text-sm font-bold uppercase tracking-widest">
        Microphone Input
      </label>

      <button
        onClick={onToggle}
        className={`
          px-8 py-4 font-bold uppercase tracking-widest text-sm
          border-2 transition-all cursor-pointer select-none
          ${isPlaying
            ? 'bg-cyan-500 border-cyan-300 text-black hover:bg-cyan-400'
            : 'bg-stone-900 border-cyan-600/50 text-cyan-200 hover:bg-stone-800 hover:border-cyan-500'
          }
        `}
        style={{
          boxShadow: isPlaying
            ? '0 0 20px rgba(6, 182, 212, 0.5), inset 0 2px 8px rgba(255, 255, 255, 0.2)'
            : '0 4px 12px rgba(0, 0, 0, 0.5)'
        }}
      >
        {isPlaying ? 'MIC ON' : 'MIC OFF'}
      </button>

      <div className="text-cyan-300/60 text-xs font-mono text-center max-w-[200px]">
        {isPlaying
          ? 'Speak or make sound - adjust gain to hear effect'
          : 'Click to enable microphone input'
        }
      </div>
    </div>
  );
}
