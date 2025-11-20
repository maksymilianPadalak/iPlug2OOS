import React from 'react';

type AudioFileInputProps = {
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
    <div className="flex flex-col items-center gap-2">
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
          className={`px-4 py-2 rounded font-bold text-xs uppercase tracking-wider transition-all ${
            isPlaying || hasFile
              ? 'bg-cyan-600 hover:bg-cyan-700 text-white'
              : 'bg-cyan-800/50 hover:bg-cyan-800 text-cyan-200 border border-cyan-600/50'
          }`}
        >
          {hasFile ? '🔄 CHANGE FILE' : '📁 SELECT FILE'}
        </button>
        {hasFile && !isPlaying && (
          <button
            onClick={handlePlayClick}
            className="px-4 py-2 rounded font-bold text-xs uppercase tracking-wider transition-all bg-emerald-600 hover:bg-emerald-700 text-white"
          >
            ▶️ PLAY
          </button>
        )}
        {isPlaying && (
          <button
            onClick={handleStopClick}
            className="px-4 py-2 rounded font-bold text-xs uppercase tracking-wider transition-all bg-rose-600 hover:bg-rose-700 text-white"
          >
            ⏹️ STOP
          </button>
        )}
      </div>
      {currentFileName && (
        <span className="text-cyan-300/80 text-[10px] uppercase tracking-wider max-w-[200px] truncate">
          {currentFileName}
        </span>
      )}
    </div>
  );
}

