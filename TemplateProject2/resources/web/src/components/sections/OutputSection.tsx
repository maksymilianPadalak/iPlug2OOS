/**
 * Output Section
 * Displays L/R output level meters
 */

import React from 'react';
import { Meter } from '../visualizations/Meter';

export function OutputSection() {
  return (
    <div className="flex justify-center mb-6">
      <div className="flex flex-col items-center gap-3 px-6 py-4 border-2 border-cyan-600/30 bg-black/40 rounded min-w-[200px]">
        <span className="text-cyan-300 text-xs font-bold uppercase tracking-wider">
          Output
        </span>
        <div className="flex items-center justify-center gap-4">
          <div className="flex flex-col items-center gap-2">
            <span className="text-cyan-200 text-xs font-bold uppercase">L</span>
            <Meter channel={0} compact={true} />
          </div>
          <div className="flex flex-col items-center gap-2">
            <span className="text-cyan-200 text-xs font-bold uppercase">R</span>
            <Meter channel={1} compact={true} />
          </div>
        </div>
      </div>
    </div>
  );
}
