/**
 * KeyboardSection
 * Piano keyboard for MIDI input
 */

import React from 'react';
import { PianoKeyboard } from '../visualizations/PianoKeyboard';

export function KeyboardSection() {
  return (
    <div className="bg-gradient-to-b from-stone-800 to-stone-900 border border-orange-700/30 rounded-lg p-2 shadow-lg">
      <PianoKeyboard />
    </div>
  );
}
