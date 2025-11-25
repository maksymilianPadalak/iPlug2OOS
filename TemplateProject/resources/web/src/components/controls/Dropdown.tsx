/**
 * Dropdown Control Component
 * 
 * Selection control for enum parameters (waveform, mode, etc.)
 */

import React from 'react';
import { sendParameterValue } from '../../glue/iplugBridge/iplugBridge';
import { useParameters, isUpdatingFromProcessor } from '../system/ParameterContext';

type DropdownProps = {
  paramId: number;
  label?: string;
  options: string[];
};

export function Dropdown({ paramId, label, options }: DropdownProps) {
  const { paramValues, setParamValue } = useParameters();
  const value = paramValues.get(paramId) ?? 0;

  const normalizedToIndex = (norm: number): number => {
    return Math.round(norm * (options.length - 1));
  };

  const indexToNormalized = (idx: number): number => {
    return options.length > 1 ? idx / (options.length - 1) : 0;
  };

  const selectedIndex = normalizedToIndex(value);

  const handleChange = (e: React.ChangeEvent<HTMLSelectElement>) => {
    const newIndex = e.target.selectedIndex;
    const normalizedValue = indexToNormalized(newIndex);

    setParamValue(paramId, normalizedValue);

    if (!isUpdatingFromProcessor()) {
      sendParameterValue(paramId, normalizedValue);
    }
  };

  return (
    <div className="flex flex-col items-center gap-1.5">
      {label && (
        <label className="text-orange-200 text-[10px] font-bold uppercase tracking-wider">
          {label}
        </label>
      )}
      <select
        value={selectedIndex}
        onChange={handleChange}
        className="
          bg-gradient-to-b from-stone-800 to-stone-900 
          border-2 border-orange-600/40 
          text-orange-100 px-3 py-1.5 rounded-md 
          font-bold text-[11px] uppercase tracking-wider 
          focus:outline-none focus:ring-2 focus:ring-orange-500/50 focus:border-orange-500 
          hover:from-stone-700 hover:to-stone-800 
          cursor-pointer transition-all shadow-md
        "
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
