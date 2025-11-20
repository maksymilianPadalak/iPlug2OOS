/**
 * Web Controls Section
 * Audio input controls for WAM mode (mic/file toggle, playback controls)
 */

import React from 'react';
import { TestToneButton } from '../inputs/TestToneButton';
import { AudioFileInput } from '../inputs/AudioFileInput';

type InputMode = 'mic' | 'file';

interface WebControlsProps {
  audioStatus: 'working' | 'not-working' | null;
  inputMode: InputMode;
  onInputModeChange: (mode: InputMode) => void;
  testToneEnabled: boolean;
  onTestToneToggle: () => void;
  audioFilePlaying: boolean;
  onFileSelected: (file: File) => Promise<void>;
  onPlayAudioFile: () => Promise<void>;
  onStopAudioFile: () => void;
  currentFileName?: string;
}

export function WebControls({
  audioStatus,
  inputMode,
  onInputModeChange,
  testToneEnabled,
  onTestToneToggle,
  audioFilePlaying,
  onFileSelected,
  onPlayAudioFile,
  onStopAudioFile,
  currentFileName,
}: WebControlsProps) {
  // Only render if audio status is available (WAM mode)
  if (!audioStatus) {
    return null;
  }

  return (
    <div className="wam-only flex items-center justify-between p-3 border-b border-cyan-600/20">
      {/* Audio Input Controls - Only visible in WAM mode (web), not in AU */}
      <div className="flex flex-col items-center gap-2 px-4 py-2 border border-cyan-600/30 bg-black/40 rounded max-w-xs">
        <span className="text-cyan-300 text-xs font-bold uppercase tracking-wider whitespace-nowrap">
          Audio Input
        </span>

        {/* Input Mode Selector */}
        <div className="flex gap-2 p-1 bg-black/60 rounded border border-cyan-600/30 w-full">
          <button
            onClick={() => onInputModeChange('mic')}
            className={`flex-1 px-3 py-1 rounded text-xs font-bold uppercase tracking-wider transition-all whitespace-nowrap ${
              inputMode === 'mic'
                ? 'bg-cyan-600 text-white'
                : 'bg-transparent text-cyan-300/60 hover:text-cyan-300'
            }`}
          >
            🎤 MIC
          </button>
          <button
            onClick={() => onInputModeChange('file')}
            className={`flex-1 px-3 py-1 rounded text-xs font-bold uppercase tracking-wider transition-all whitespace-nowrap ${
              inputMode === 'file'
                ? 'bg-cyan-600 text-white'
                : 'bg-transparent text-cyan-300/60 hover:text-cyan-300'
            }`}
          >
            📁 FILE
          </button>
        </div>

        {/* Conditional Input Controls */}
        <div className="flex items-center justify-center w-full">
          {inputMode === 'file' ? (
            <AudioFileInput
              onFileSelected={onFileSelected}
              onPlay={onPlayAudioFile}
              onStop={onStopAudioFile}
              isPlaying={audioFilePlaying}
              hasFile={!!currentFileName}
              currentFileName={currentFileName}
            />
          ) : (
            <TestToneButton
              isPlaying={testToneEnabled}
              onToggle={onTestToneToggle}
            />
          )}
        </div>
      </div>

      {/* Audio Status */}
      <div className={`px-3 py-1 rounded text-xs font-bold uppercase tracking-wider whitespace-nowrap ${
        audioStatus === 'working' ? 'bg-emerald-500/20 text-emerald-400' : 'bg-rose-500/20 text-rose-400'
      }`}>
        {audioStatus === 'working' ? '✓ AUDIO READY' : '✗ AUDIO ERROR'}
      </div>
    </div>
  );
}
