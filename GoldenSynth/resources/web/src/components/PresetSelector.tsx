/**
 * Preset Selector Component
 *
 * Dropdown to select factory presets. Sends message to C++ to restore preset.
 */

import { useState } from 'react';
import { sendArbitraryMessage } from 'sharedUi/bridge';
import { EMsgTags } from '@/config/runtimeParameters';

const PRESETS = ['Init', 'Classic Lead'];

export function PresetSelector() {
  const [selectedIndex, setSelectedIndex] = useState(0);

  const handleChange = (e: React.ChangeEvent<HTMLSelectElement>) => {
    const newIndex = parseInt(e.target.value, 10);
    setSelectedIndex(newIndex);

    // Send preset index as 4-byte int32
    const buffer = new ArrayBuffer(4);
    new DataView(buffer).setInt32(0, newIndex, true); // little-endian
    sendArbitraryMessage(EMsgTags.kMsgTagRestorePreset, -1, buffer);
  };

  return (
    <div className="flex items-center gap-2">
      <label className="text-cyan-400/80 text-[10px] font-bold uppercase tracking-wider">
        Preset
      </label>
      <select
        value={selectedIndex}
        onChange={handleChange}
        className="
          bg-[#0a0a10] border border-cyan-500/30
          text-cyan-300 px-3 py-1 rounded
          text-xs font-medium
          focus:outline-none focus:ring-1 focus:ring-cyan-500/50 focus:border-cyan-500/60
          hover:border-cyan-500/50 hover:bg-[#0c0c14]
          cursor-pointer transition-all
        "
      >
        {PRESETS.map((name, index) => (
          <option key={index} value={index} className="bg-[#0a0a10] text-cyan-300">
            {name}
          </option>
        ))}
      </select>
    </div>
  );
}
