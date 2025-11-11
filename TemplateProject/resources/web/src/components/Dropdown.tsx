/**
 * Dropdown component - Berlin Brutalism Style
 */

import React from 'react';
import { EParams } from '../config/constants';
import { useParameters } from './ParameterContext';
import { sendParameterValue } from '../communication/iplug-bridge';
import { isUpdatingFromProcessor } from './ParameterContext';

interface DropdownProps {
  paramIdx: EParams;
  label: string;
  options: string[];
}

export function Dropdown({ paramIdx, label, options }: DropdownProps) {
  const { paramValues, setParamValue } = useParameters();
  const value = paramValues.get(paramIdx) ?? 0;
  
  // Convert normalized value to enum index
  const normalizedToEnumIndex = (norm: number): number => {
    return Math.round(norm * (options.length - 1));
  };
  
  const enumIndexToNormalized = (idx: number): number => {
    return idx / (options.length - 1);
  };
  
  const selectedIndex = normalizedToEnumIndex(value);

  const handleChange = (e: React.ChangeEvent<HTMLSelectElement>) => {
    const newIndex = e.target.selectedIndex;
    const normalizedValue = enumIndexToNormalized(newIndex);
    
    setParamValue(paramIdx, normalizedValue);
    
    if (!isUpdatingFromProcessor()) {
      sendParameterValue(paramIdx, normalizedValue);
    }
  };

  return (
    <div className="flex flex-col items-center gap-2">
      <label className="text-orange-200 text-xs font-bold uppercase tracking-wider">
        {label}
      </label>
      <select
        value={selectedIndex}
        onChange={handleChange}
        className="bg-gradient-to-b from-stone-800 to-stone-900 border border-orange-600/50 text-orange-100 px-3 py-2 rounded font-bold text-xs uppercase tracking-wider focus:outline-none focus:ring-2 focus:ring-orange-500/50 focus:border-orange-500 hover:from-stone-700 hover:to-stone-800 cursor-pointer transition-all shadow-md"
      >
        {options.map((option, index) => (
          <option key={index} value={index} className="bg-stone-900 text-orange-100">
            {option}
          </option>
        ))}
      </select>
    </div>
  );
}
