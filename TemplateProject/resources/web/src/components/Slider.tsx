/**
 * Slider component for parameter control
 */

import React from 'react';
import { EParams } from '../config/constants';
import { normalizedToDisplay } from '../utils/parameter';
import { useParameters } from './ParameterContext';
import { sendParameterValue } from '../communication/iplug-bridge';
import { isUpdatingFromProcessor } from './ParameterContext';

interface SliderProps {
  paramIdx: EParams;
  label: string;
  min?: number;
  max?: number;
  step?: number;
}

export function Slider({ paramIdx, label, min = 0, max = 1, step = 0.001 }: SliderProps) {
  const { paramValues, setParamValue } = useParameters();
  const value = paramValues.get(paramIdx) ?? 0;

  const handleChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    const newValue = Number(e.target.value);
    
    // Update React state immediately for responsive UI
    setParamValue(paramIdx, newValue);
    
    // Send to processor only if not updating from processor
    if (!isUpdatingFromProcessor()) {
      sendParameterValue(paramIdx, newValue);
    }
  };

  return (
    <div>
      <label style={{ color: '#ffffff', fontSize: '12px', display: 'block', marginBottom: '5px' }}>
        {label}
      </label>
      <input
        type="range"
        min={min}
        max={max}
        step={step}
        value={value}
        onChange={handleChange}
        style={{ width: '100%' }}
      />
      <div style={{ color: '#cccccc', fontSize: '10px', textAlign: 'right' }}>
        {normalizedToDisplay(paramIdx, value)}
      </div>
    </div>
  );
}

