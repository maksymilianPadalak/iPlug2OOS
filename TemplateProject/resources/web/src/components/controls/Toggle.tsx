/**
 * Toggle Control Component
 * 
 * On/off switch for boolean parameters (bypass, enable, etc.)
 */

import React from 'react';
import { sendParameterValue } from '../../glue/iplugBridge/iplugBridge';
import { useParameters, isUpdatingFromProcessor } from '../system/ParameterContext';

type ToggleProps = {
  paramId: number;
  label?: string;
};

export function Toggle({ paramId, label }: ToggleProps) {
  const { paramValues, setParamValue } = useParameters();
  const value = paramValues.get(paramId) ?? 0;
  const isOn = value > 0.5;

  const handleToggle = () => {
    const newValue = isOn ? 0.0 : 1.0;
    setParamValue(paramId, newValue);

    if (!isUpdatingFromProcessor()) {
      sendParameterValue(paramId, newValue);
    }
  };

  return (
    <div className="flex flex-col items-center gap-1.5">
      {label && (
        <label className="text-orange-200 text-[10px] font-bold uppercase tracking-wider">
          {label}
        </label>
      )}
      <button
        onClick={handleToggle}
        className={`
          relative w-12 h-6 rounded-full cursor-pointer transition-all duration-200
          border-2
          ${isOn
            ? 'bg-orange-500/80 border-orange-400 shadow-[0_0_12px_rgba(251,146,60,0.4)]'
            : 'bg-stone-800 border-stone-600'
          }
        `}
      >
        <div
          className={`
            absolute top-0.5 w-4 h-4 rounded-full transition-all duration-200
            ${isOn
              ? 'left-6 bg-orange-100 shadow-md'
              : 'left-0.5 bg-stone-400'
            }
          `}
        />
      </button>
      <div className={`text-[10px] font-bold uppercase ${isOn ? 'text-orange-400' : 'text-stone-500'}`}>
        {isOn ? 'On' : 'Off'}
      </div>
    </div>
  );
}


