/**
 * OscillatorWaveform Component
 * Displays waveform visualization and dropdown for a single oscillator
 */

import React from 'react';
import { EParams } from '../../config/constants';
import { useParameters } from '../system/ParameterContext';
import { Dropdown } from '../controls/Dropdown';
import { WaveformDisplay } from './WaveformDisplay';

interface OscillatorWaveformProps {
  oscNum: 1 | 2 | 3 | 4;
}

export function OscillatorWaveform({ oscNum }: OscillatorWaveformProps) {
  let waveParam = EParams.kParamOsc1Wave;

  switch (oscNum) {
    case 1: waveParam = EParams.kParamOsc1Wave; break;
    case 2: waveParam = EParams.kParamOsc2Wave; break;
    case 3: waveParam = EParams.kParamOsc3Wave; break;
    case 4: waveParam = EParams.kParamOsc4Wave; break;
  }

  const { paramValues } = useParameters();
  const waveValue = paramValues.get(waveParam) ?? 0;
  const waveIndex = Math.round(waveValue * 3);

  // Map oscillator to its knob primary color so the waveform matches the knob
  const primaryColor = React.useMemo(() => {
    switch (oscNum) {
      case 1: return '#06b6d4'; // Cyan/teal for oscillator 1
      case 2: return '#34d399'; // green
      case 3: return '#a78bfa'; // purple
      case 4: return '#f472b6'; // pink
      default: return '#fb923c';
    }
  }, [oscNum]);

  return (
    <div className="flex flex-col items-center gap-2 w-[180px]">
      <div className="text-orange-300 text-xs font-black uppercase tracking-wider">OSC {oscNum}</div>
      <Dropdown
        paramIdx={waveParam}
        label="WAVEFORM"
        options={["Sine", "Saw", "Square", "Triangle"]}
      />
      <WaveformDisplay waveIndex={waveIndex} color={primaryColor} />
    </div>
  );
}
