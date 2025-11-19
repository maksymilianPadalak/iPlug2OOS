/**
 * OutputMeters Section
 * Stereo L/R output level meters
 */

import React from 'react';
import { Meter } from '../visualizations/Meter';

export function OutputMeters() {
  return (
    <div className="flex items-center justify-center gap-6 mb-3">
      <div className="flex items-center gap-2">
        <span className="text-orange-300 text-xs font-bold uppercase tracking-wider">L</span>
        <Meter channel={0} compact={true} />
      </div>
      <div className="flex items-center gap-2">
        <span className="text-orange-300 text-xs font-bold uppercase tracking-wider">R</span>
        <Meter channel={1} compact={true} />
      </div>
    </div>
  );
}
