/**
 * Mocked TestToneButton for Storybook
 * Microphone toggle button without actual audio
 */

import React, { useState } from 'react';

type MockedTestToneButtonProps = {
  initialIsPlaying?: boolean;
  onToggle?: (isPlaying: boolean) => void;
  label?: string;
};

export function MockedTestToneButton({ 
  initialIsPlaying = false,
  onToggle,
  label = 'Microphone Input'
}: MockedTestToneButtonProps) {
  const [isPlaying, setIsPlaying] = useState(initialIsPlaying);

  const handleToggle = () => {
    const newState = !isPlaying;
    setIsPlaying(newState);
    onToggle?.(newState);
    console.log(`[Storybook] Mic ${newState ? 'ON' : 'OFF'}`);
  };

  return (
    <div className="flex flex-col items-center gap-3">
      <label className="text-cyan-200 text-sm font-bold uppercase tracking-widest">
        {label}
      </label>

      <button
        onClick={handleToggle}
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
        {isPlaying ? '🎤 MIC ON' : '🔇 MIC OFF'}
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

// Compact variant
type MockedCompactMicButtonProps = {
  initialIsPlaying?: boolean;
  onToggle?: (isPlaying: boolean) => void;
};

export function MockedCompactMicButton({ 
  initialIsPlaying = false,
  onToggle
}: MockedCompactMicButtonProps) {
  const [isPlaying, setIsPlaying] = useState(initialIsPlaying);

  const handleToggle = () => {
    const newState = !isPlaying;
    setIsPlaying(newState);
    onToggle?.(newState);
  };

  return (
    <button
      onClick={handleToggle}
      className={`
        px-4 py-2 font-bold uppercase tracking-wider text-xs
        border-2 rounded transition-all cursor-pointer
        ${isPlaying
          ? 'bg-cyan-500 border-cyan-300 text-black'
          : 'bg-stone-900 border-cyan-600/50 text-cyan-200 hover:bg-stone-800'
        }
      `}
    >
      {isPlaying ? '🎤 ON' : '🔇 OFF'}
    </button>
  );
}

