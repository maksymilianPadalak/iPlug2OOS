/**
 * Dropdown Control Component
 *
 * Selection control for enum parameters (waveform, mode, etc.)
 * Takes paramId and uses useParameter internally.
 * Options are read from RuntimeParametersProvider context.
 */

import React from 'react';
import { useParameter } from '../glue/hooks/useParameter';
import { useRuntimeParameters } from '../glue/RuntimeParametersProvider';
import type { DropdownProps } from './componentProps';

export type { DropdownProps };

export function Dropdown({ paramId, label }: DropdownProps) {
  const { value, setValue, beginChange, endChange } = useParameter(paramId);
  const runtimeParameters = useRuntimeParameters();

  // Get options from runtimeParameters
  const param = runtimeParameters.find((p) => p.id === paramId);
  const options = param?.enumValues || [];

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

    beginChange();
    setValue(normalizedValue);
    endChange();
  };

  if (options.length === 0) {
    return <div className="text-red-500 text-xs">No options for param {paramId}</div>;
  }

  return (
    <div className="flex flex-col items-center gap-1.5 min-w-0">
      {label && (
        <label className="text-[#1a1a1a] text-[10px] font-bold uppercase tracking-[0.08em]">
          {label}
        </label>
      )}
      <select
        value={selectedIndex}
        onChange={handleChange}
        className="
          w-full max-w-[200px]
          bg-gradient-to-b from-[#F8F4EF] to-[#EDE5DA]
          border border-[#B8860B]/40
          text-[#1a1a1a] px-3 py-1.5 rounded-lg
          font-bold text-[11px] uppercase tracking-wider
          focus:outline-none focus:ring-2 focus:ring-[#B8860B]/30 focus:border-[#B8860B]
          hover:from-white hover:to-[#F5EDE2]
          cursor-pointer transition-all truncate
        "
        style={{
          boxShadow: 'inset 0 1px 2px rgba(255,255,255,0.8), 0 2px 4px rgba(0,0,0,0.08)',
        }}
      >
        {options.map((option, index) => (
          <option key={index} value={index} className="bg-[#F5EDE2] text-[#1a1a1a]">
            {option}
          </option>
        ))}
      </select>
    </div>
  );
}
