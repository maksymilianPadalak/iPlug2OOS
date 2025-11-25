/**
 * Mocked AudioFileInput for Storybook
 * File input and playback controls without actual audio
 */

import React, { useState } from 'react';

type MockedAudioFileInputProps = {
  initialHasFile?: boolean;
  initialIsPlaying?: boolean;
  initialFileName?: string;
  onFileSelected?: (file: File) => void;
  onPlay?: () => void;
  onStop?: () => void;
};

export function MockedAudioFileInput({ 
  initialHasFile = false,
  initialIsPlaying = false,
  initialFileName = '',
  onFileSelected,
  onPlay,
  onStop
}: MockedAudioFileInputProps) {
  const [hasFile, setHasFile] = useState(initialHasFile);
  const [isPlaying, setIsPlaying] = useState(initialIsPlaying);
  const [fileName, setFileName] = useState(initialFileName);

  const handleFileChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    const file = e.target.files?.[0];
    if (file) {
      setHasFile(true);
      setFileName(file.name);
      setIsPlaying(false);
      onFileSelected?.(file);
      console.log(`[Storybook] File selected: ${file.name}`);
    }
  };

  const handlePlay = () => {
    setIsPlaying(true);
    onPlay?.();
    console.log('[Storybook] Play clicked');
  };

  const handleStop = () => {
    setIsPlaying(false);
    onStop?.();
    console.log('[Storybook] Stop clicked');
  };

  return (
    <div className="flex flex-col items-center gap-2">
      <input
        type="file"
        accept="audio/*"
        onChange={handleFileChange}
        className="hidden"
        id="storybook-audio-input"
      />
      <div className="flex gap-2">
        <label
          htmlFor="storybook-audio-input"
          className={`px-4 py-2 rounded font-bold text-xs uppercase tracking-wider transition-all cursor-pointer ${
            isPlaying || hasFile
              ? 'bg-cyan-600 hover:bg-cyan-700 text-white'
              : 'bg-cyan-800/50 hover:bg-cyan-800 text-cyan-200 border border-cyan-600/50'
          }`}
        >
          {hasFile ? '🔄 CHANGE FILE' : '📁 SELECT FILE'}
        </label>
        {hasFile && !isPlaying && (
          <button
            onClick={handlePlay}
            className="px-4 py-2 rounded font-bold text-xs uppercase tracking-wider transition-all bg-emerald-600 hover:bg-emerald-700 text-white"
          >
            ▶️ PLAY
          </button>
        )}
        {isPlaying && (
          <button
            onClick={handleStop}
            className="px-4 py-2 rounded font-bold text-xs uppercase tracking-wider transition-all bg-rose-600 hover:bg-rose-700 text-white"
          >
            ⏹️ STOP
          </button>
        )}
      </div>
      {fileName && (
        <span className="text-cyan-300/80 text-[10px] uppercase tracking-wider max-w-[200px] truncate">
          {fileName}
        </span>
      )}
    </div>
  );
}

