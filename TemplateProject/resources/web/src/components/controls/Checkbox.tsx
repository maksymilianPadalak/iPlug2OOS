import React from 'react';
import { EParams } from '../../config/constants';
import { sendParameterValue } from '../../glue/iplugBridge/iplugBridge';
import { useParameters } from '../system/ParameterContext';
import { isUpdatingFromProcessor } from '../system/ParameterContext';

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

    setParamValue(paramIdx, newValue);

    if (!isUpdatingFromProcessor()) {
      sendParameterValue(paramIdx, newValue);
    }
  };

  return (
    <label className="flex items-center text-white text-xs font-mono uppercase tracking-wider cursor-pointer">
      <input
        type="checkbox"
        checked={checked}
        onChange={handleChange}
        className="w-3 h-3 border-2 border-white bg-black accent-white mr-2 cursor-pointer"
      />
      {label}
    </label>
  );
}
