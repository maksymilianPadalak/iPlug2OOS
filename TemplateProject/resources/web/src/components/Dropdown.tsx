/**
 * Dropdown component - Berlin Brutalism Style
 */

import React from 'react';
import { EParams } from '../config/constants';
import { useParameters } from './ParameterContext';
import { sendParameterEnum } from '../communication/iplug-bridge';
import { isUpdatingFromProcessor } from './ParameterContext';

interface DropdownProps {
  paramIdx: EParams;
  label: string;
  options: string[];
}

export function Dropdown({ paramIdx, label, options }: DropdownProps) {
  const { paramValues, setParamValue } = useParameters();
  const value = paramValues.get(paramIdx) ?? 0;
  const selectedIndex = Math.round(value);

  const handleChange = (e: React.ChangeEvent<HTMLSelectElement>) => {
    const newIndex = e.target.selectedIndex;
    
    setParamValue(paramIdx, newIndex);
    
    if (!isUpdatingFromProcessor()) {
      sendParameterEnum(paramIdx, newIndex);
    }
  };

  return (
    <div className="flex flex-col items-center gap-1">
      <label className="text-white text-xs font-mono uppercase tracking-wider">
        {label}
      </label>
      <select
        value={selectedIndex}
        onChange={handleChange}
        className="bg-black border-2 border-white text-white px-2 py-1 font-mono text-xs uppercase tracking-wider focus:outline-none focus:bg-white focus:text-black cursor-pointer"
      >
        {options.map((option, index) => (
          <option key={index} value={index} className="bg-black text-white">
            {option}
          </option>
        ))}
      </select>
    </div>
  );
}
