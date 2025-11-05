/**
 * Checkbox component for boolean parameters
 */

import React from 'react';
import { EParams } from '../config/constants';
import { useParameters } from './ParameterContext';
import { sendParameterValue } from '../communication/iplug-bridge';
import { isUpdatingFromProcessor } from './ParameterContext';

interface CheckboxProps {
  paramIdx: EParams;
  label: string;
}

export function Checkbox({ paramIdx, label }: CheckboxProps) {
  const { paramValues, setParamValue } = useParameters();
  const value = paramValues.get(paramIdx) ?? 0;
  const checked = value > 0.5;

  const handleChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    const newValue = e.target.checked ? 1.0 : 0.0;
    
    // Update React state immediately
    setParamValue(paramIdx, newValue);
    
    // Send to processor only if not updating from processor
    if (!isUpdatingFromProcessor()) {
      sendParameterValue(paramIdx, newValue);
    }
  };

  return (
    <label style={{ color: '#ffffff', fontSize: '12px', display: 'block', marginBottom: '5px' }}>
      <input
        type="checkbox"
        checked={checked}
        onChange={handleChange}
        style={{ marginRight: '5px' }}
      />
      {label}
    </label>
  );
}

