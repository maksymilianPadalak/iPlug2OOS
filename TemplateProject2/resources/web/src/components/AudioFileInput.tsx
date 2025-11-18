import React from 'react';

type AudioFileInputProps = {
  onFileSelected: (file: File) => void;
  isPlaying: boolean;
  currentFileName?: string;
};

export function AudioFileInput({ onFileSelected, isPlaying, currentFileName }: AudioFileInputProps) {
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

  const handleButtonClick = () => {
    fileInputRef.current?.click();
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
      <button
        onClick={handleButtonClick}
        className={`px-4 py-2 rounded font-bold text-xs uppercase tracking-wider transition-all ${
          isPlaying
            ? 'bg-cyan-600 hover:bg-cyan-700 text-white'
            : 'bg-cyan-800/50 hover:bg-cyan-800 text-cyan-200 border border-cyan-600/50'
        }`}
      >
        {isPlaying ? '🔄 CHANGE FILE' : '📁 SELECT FILE'}
      </button>
      {currentFileName && (
        <span className="text-cyan-300/80 text-[10px] uppercase tracking-wider max-w-[200px] truncate">
          {currentFileName}
        </span>
      )}
    </div>
  );
}

