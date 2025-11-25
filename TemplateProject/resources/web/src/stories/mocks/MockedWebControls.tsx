/**
 * Mocked WebControls for Storybook
 * MIDI and audio status controls
 */

import React, { useState } from 'react';

type AudioStatus = 'working' | 'not-working' | null;

type MockedWebControlsProps = {
  audioStatus?: AudioStatus;
  showMidiControls?: boolean;
  midiInputOptions?: string[];
  midiOutputOptions?: string[];
};

export function MockedWebControls({ 
  audioStatus = null,
  showMidiControls = true,
  midiInputOptions = ['MIDI INPUT'],
  midiOutputOptions = ['MIDI OUTPUT']
}: MockedWebControlsProps) {
  const [selectedInput, setSelectedInput] = useState(0);
  const [selectedOutput, setSelectedOutput] = useState(0);

  return (
    <>
      {/* Web Controls */}
      <div className="mb-2 max-w-7xl mx-auto">
        {/* Audio Status */}
        {audioStatus && (
          <div className="flex items-center justify-center mb-2">
            <p className={`text-xs font-bold uppercase tracking-widest ${
              audioStatus === 'working' ? 'text-emerald-400' : 'text-rose-400'
            }`}>
              {audioStatus === 'working' ? '✓ AUDIO WORKING' : '✗ AUDIO NOT WORKING'}
            </p>
          </div>
        )}

        {/* MIDI Controls */}
        {showMidiControls && (
          <div className="flex flex-col items-center mb-2">
            <h2 className="text-orange-300 text-xs font-bold uppercase tracking-widest mb-2">MIDI</h2>
            <div className="flex gap-4 justify-center">
              <div className="flex flex-col items-center gap-1">
                <label className="text-orange-200 text-xs font-bold uppercase tracking-wider">
                  INPUT
                </label>
                <select
                  value={selectedInput}
                  onChange={(e) => setSelectedInput(Number(e.target.value))}
                  className="bg-gradient-to-b from-orange-900 to-orange-950 border border-orange-600/50 text-orange-100 px-4 py-2 rounded font-bold text-xs uppercase tracking-wider hover:from-orange-800 hover:to-orange-900 focus:outline-none focus:ring-2 focus:ring-orange-500/50 cursor-pointer transition-all"
                >
                  {midiInputOptions.map((option, index) => (
                    <option key={index} value={index}>{option}</option>
                  ))}
                </select>
              </div>
              <div className="flex flex-col items-center gap-1">
                <label className="text-orange-200 text-xs font-bold uppercase tracking-wider">
                  OUTPUT
                </label>
                <select
                  value={selectedOutput}
                  onChange={(e) => setSelectedOutput(Number(e.target.value))}
                  className="bg-gradient-to-b from-orange-900 to-orange-950 border border-orange-600/50 text-orange-100 px-4 py-2 rounded font-bold text-xs uppercase tracking-wider hover:from-orange-800 hover:to-orange-900 focus:outline-none focus:ring-2 focus:ring-orange-500/50 cursor-pointer transition-all"
                >
                  {midiOutputOptions.map((option, index) => (
                    <option key={index} value={index}>{option}</option>
                  ))}
                </select>
              </div>
            </div>
          </div>
        )}
      </div>

      {/* Horizontal Separator */}
      <div className="border-t border-orange-900/50 mb-2 max-w-7xl mx-auto"></div>
    </>
  );
}

// Compact audio status indicator
type MockedAudioStatusProps = {
  status: AudioStatus;
};

export function MockedAudioStatus({ status }: MockedAudioStatusProps) {
  if (!status) return null;
  
  return (
    <div className={`inline-flex items-center gap-2 px-3 py-1 rounded-full text-xs font-bold uppercase tracking-wider ${
      status === 'working' 
        ? 'bg-emerald-900/30 text-emerald-400 border border-emerald-600/30' 
        : 'bg-rose-900/30 text-rose-400 border border-rose-600/30'
    }`}>
      <span className={`w-2 h-2 rounded-full ${
        status === 'working' ? 'bg-emerald-400' : 'bg-rose-400'
      }`} />
      {status === 'working' ? 'Audio OK' : 'Audio Error'}
    </div>
  );
}

