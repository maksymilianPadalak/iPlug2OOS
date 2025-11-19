/**
 * WebControls Section
 * MIDI and audio status controls (only visible in WAM mode)
 */

import React from 'react';

interface WebControlsProps {
  audioStatus: 'working' | 'not-working' | null;
}

export function WebControls({ audioStatus }: WebControlsProps) {
  return (
    <>
      {/* Web Controls - Only visible in WAM mode */}
      <div className="wam-only mb-2 max-w-7xl mx-auto">
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
        <div className="flex flex-col items-center mb-2">
          <h2 className="text-orange-300 text-xs font-bold uppercase tracking-widest mb-2">MIDI</h2>
          <div className="flex gap-4 justify-center">
            <div className="flex flex-col items-center gap-1">
              <label htmlFor="midiInSelect" className="text-orange-200 text-xs font-bold uppercase tracking-wider">
                INPUT
              </label>
              <select
                id="midiInSelect"
                disabled={true}
                className="bg-gradient-to-b from-orange-900 to-orange-950 border border-orange-600/50 text-orange-100 px-4 py-2 rounded font-bold text-xs uppercase tracking-wider disabled:opacity-50 disabled:cursor-not-allowed hover:from-orange-800 hover:to-orange-900 focus:outline-none focus:ring-2 focus:ring-orange-500/50 cursor-pointer transition-all"
              >
                <option value="default">MIDI INPUT</option>
              </select>
            </div>
            <div className="flex flex-col items-center gap-1">
              <label htmlFor="midiOutSelect" className="text-orange-200 text-xs font-bold uppercase tracking-wider">
                OUTPUT
              </label>
              <select
                id="midiOutSelect"
                disabled={true}
                className="bg-gradient-to-b from-orange-900 to-orange-950 border border-orange-600/50 text-orange-100 px-4 py-2 rounded font-bold text-xs uppercase tracking-wider disabled:opacity-50 disabled:cursor-not-allowed hover:from-orange-800 hover:to-orange-900 focus:outline-none focus:ring-2 focus:ring-orange-500/50 cursor-pointer transition-all"
              >
                <option value="default">MIDI OUTPUT</option>
              </select>
            </div>
          </div>
        </div>
      </div>

      {/* Horizontal Separator - Only visible in WAM mode */}
      <div className="wam-only border-t border-orange-900/50 mb-2 max-w-7xl mx-auto"></div>
    </>
  );
}
