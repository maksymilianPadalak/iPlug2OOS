/**
 * Dropdown Control Component
 *
 * Selection control for enum parameters (waveform, mode, etc.)
 */

import React from 'react';
import { useParameter } from '@/glue/hooks/useParameter';

type DropdownProps = {
  paramId: number;
  label?: string;
  options: string[];
};

export function Dropdown({ paramId, label, options }: DropdownProps) {
  const { value, setValue, beginChange, endChange } = useParameter(paramId);

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

    beginChange(); // Signal start of parameter change for automation
    setValue(normalizedValue);
    endChange(); // Signal end of parameter change for automation
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
