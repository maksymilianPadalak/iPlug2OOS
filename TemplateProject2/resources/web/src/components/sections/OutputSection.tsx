/**
 * Output Section
 * Displays L/R output level meters
 */

import React from 'react';
import { Meter } from '../visualizations/Meter';

export function OutputSection() {
  return (
    <div className="flex justify-center">
      <div className="flex flex-col md:flex-row items-center gap-3 px-4 py-3 border border-cyan-600/40 bg-black/60 rounded-md w-full md:w-auto">
        <div className="flex flex-col items-center md:items-start gap-1">
          <span className="text-cyan-300 text-[10px] font-bold uppercase tracking-[0.3em]">
            OUTPUT MONITOR
          </span>
          <span className="text-cyan-500/70 text-[9px] font-mono uppercase tracking-[0.22em]">
            Keep clouds under control
          </span>
        </div>
        <div className="flex items-center justify-center gap-4">
          <div className="flex flex-col items-center gap-1">
            <span className="text-cyan-200 text-[10px] font-bold uppercase">L</span>
            <Meter channel={0} compact={true} />
          </div>
          <div className="flex flex-col items-center gap-1">
            <span className="text-cyan-200 text-[10px] font-bold uppercase">R</span>
            <Meter channel={1} compact={true} />
          </div>
        </div>
      </div>
    </div>
  );
}
