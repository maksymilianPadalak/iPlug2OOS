/**
 * OscillatorSection
 * All 4 oscillators with waveform displays and control knobs
 */

import React from 'react';
import { OscillatorWaveform } from '../helpers/OscillatorWaveform';
import { OscillatorKnobs } from '../helpers/OscillatorKnobs';

export function OscillatorSection() {
  return (
    <div className="flex gap-6 justify-center mb-6">
      {/* Each oscillator in its own column */}
      <div className="flex flex-col items-center gap-3">
        <OscillatorWaveform oscNum={1} />
        <OscillatorKnobs oscNum={1} />
      </div>
      <div className="flex flex-col items-center gap-3">
        <OscillatorWaveform oscNum={2} />
        <OscillatorKnobs oscNum={2} />
      </div>
      <div className="flex flex-col items-center gap-3">
        <OscillatorWaveform oscNum={3} />
        <OscillatorKnobs oscNum={3} />
      </div>
      <div className="flex flex-col items-center gap-3">
        <OscillatorWaveform oscNum={4} />
        <OscillatorKnobs oscNum={4} />
      </div>
    </div>
  );
}
