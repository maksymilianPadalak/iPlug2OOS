/**
 * Dropdown component for enum parameters
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
    
    // Update React state immediately
    setParamValue(paramIdx, newIndex);
    
    // Send to processor only if not updating from processor
    if (!isUpdatingFromProcessor()) {
      sendParameterEnum(paramIdx, newIndex);
    }
  };

  return (
    <div>
      <label style={{ color: '#ffffff', fontSize: '12px', display: 'block', marginBottom: '5px' }}>
        {label}
      </label>
      <select
        value={selectedIndex}
        onChange={handleChange}
        style={{
          width: '100%',
          padding: '5px',
          background: '#000000',
          color: '#ffffff',
          border: '1px solid #ffffff',
        }}
      >
        {options.map((option, index) => (
          <option key={index} value={index}>
            {option}
          </option>
        ))}
      </select>
    </div>
  );
}

