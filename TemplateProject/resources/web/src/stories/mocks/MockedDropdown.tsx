/**
 * Mocked Dropdown for Storybook
 */

import React, { useState } from 'react';
import { EParams } from '../../config/constants';

type MockedDropdownProps = {
  paramId: EParams;
  label?: string;
  options: string[];
  initialIndex?: number;
};

export function MockedDropdown({ paramId, label, options, initialIndex = 0 }: MockedDropdownProps) {
  const [selectedIndex, setSelectedIndex] = useState(initialIndex);

  const handleChange = (e: React.ChangeEvent<HTMLSelectElement>) => {
    setSelectedIndex(e.target.selectedIndex);
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

