/**
 * Slider component - Berlin Brutalism Style
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
    
    setParamValue(paramIdx, newValue);
    
    if (!isUpdatingFromProcessor()) {
      sendParameterValue(paramIdx, newValue);
    }
  };

  return (
    <div className="space-y-2">
      <label className="block text-white text-xs font-mono uppercase tracking-wider">
        {label}
      </label>
      <input
        type="range"
        min={min}
        max={max}
        step={step}
        value={value}
        onChange={handleChange}
        className="w-full h-2 bg-black border-2 border-white accent-white cursor-pointer"
        style={{
          background: `linear-gradient(to right, #ffffff 0%, #ffffff ${(value - min) / (max - min) * 100}%, #000000 ${(value - min) / (max - min) * 100}%, #000000 100%)`
        }}
      />
      <div className="text-gray-400 text-xs font-mono text-right">
        {normalizedToDisplay(paramIdx, value)}
      </div>
    </div>
  );
}
