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
          className={`px-4 py-2 rounded-lg font-bold text-xs uppercase tracking-wider transition-all shadow-sm ${
            hasFile
              ? 'bg-[#B8860B] hover:bg-[#9A7209] text-white'
              : 'bg-[#B8860B]/20 hover:bg-[#B8860B]/40 text-[#5a4a2a] border border-[#B8860B]/40'
          }`}
        >
          {hasFile ? 'Change File' : 'Select File'}
        </button>
        {hasFile && !isPlaying && (
          <button
            onClick={handlePlayClick}
            className="px-4 py-2 rounded-lg font-bold text-xs uppercase tracking-wider transition-all bg-emerald-600 hover:bg-emerald-700 text-white shadow-sm"
          >
            Play
          </button>
        )}
        {isPlaying && (
          <button
            onClick={handleStopClick}
            className="px-4 py-2 rounded-lg font-bold text-xs uppercase tracking-wider transition-all bg-rose-600 hover:bg-rose-700 text-white shadow-sm"
          >
            Stop
          </button>
        )}
      </div>
      {currentFileName && (
        <span className="text-[#5a4a2a]/70 text-[10px] uppercase tracking-wider max-w-[200px] truncate">
          {currentFileName}
        </span>
      )}
    </div>
  );
}
