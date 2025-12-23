/**
 * AudioFileInput Component
 *
 * File input for selecting and playing audio files.
 */

import React from 'react';

export type AudioFileInputProps = {
  onFileSelected: (file: File) => void;
  onPlay: () => void;
  onStop: () => void;
  isPlaying: boolean;
  hasFile: boolean;
  currentFileName?: string;
};

export function AudioFileInput({ onFileSelected, onPlay, onStop, isPlaying, hasFile, currentFileName }: AudioFileInputProps) {
  const fileInputRef = React.useRef<HTMLInputElement>(null);

  const handleFileChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    const file = e.target.files?.[0];
    if (file) {
      // Validate file type
      if (!file.type.startsWith('audio/')) {
        alert('Please select an audio file');
        return;
      }
      onFileSelected(file);
    }
  };

  const handleSelectFileClick = () => {
    fileInputRef.current?.click();
  };

  const handlePlayClick = () => {
    onPlay();
  };

  const handleStopClick = () => {
    onStop();
  };

  return (
    <div className="flex items-center gap-3">
      <input
        ref={fileInputRef}
        type="file"
        accept="audio/*"
        onChange={handleFileChange}
        className="hidden"
      />
      <div className="flex gap-2">
        <button
          onClick={handleSelectFileClick}
          className={`px-4 py-2 rounded-lg font-bold text-xs uppercase tracking-wider transition-all ${
            hasFile
              ? 'bg-cyan-500 hover:bg-cyan-600 text-white shadow-[0_0_12px_rgba(34,211,238,0.3)]'
              : 'bg-white/10 hover:bg-white/20 text-white/70 border border-white/20'
          }`}
        >
          {hasFile ? 'Change File' : 'Select File'}
        </button>
        {hasFile && !isPlaying && (
          <button
            onClick={handlePlayClick}
            className="px-4 py-2 rounded-lg font-bold text-xs uppercase tracking-wider transition-all bg-emerald-500 hover:bg-emerald-600 text-white shadow-[0_0_12px_rgba(16,185,129,0.3)]"
          >
            Play
          </button>
        )}
        {isPlaying && (
          <button
            onClick={handleStopClick}
            className="px-4 py-2 rounded-lg font-bold text-xs uppercase tracking-wider transition-all bg-rose-500 hover:bg-rose-600 text-white shadow-[0_0_12px_rgba(244,63,94,0.3)]"
          >
            Stop
          </button>
        )}
      </div>
      {currentFileName && (
        <span className="text-white/40 text-[10px] uppercase tracking-wider max-w-[200px] truncate">
          {currentFileName}
        </span>
      )}
    </div>
  );
}
