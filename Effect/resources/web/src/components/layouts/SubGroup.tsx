/**
 * SubGroup Layout Component
 *
 * Groups related controls within a Section with optional title and layout.
 * Mini version of Section styling - corner accent, gradient background.
 * Use grid-4 for ADSR sets, grid-2 for stereo pairs, row for default.
 */

import React from 'react';
import type { SubGroupProps } from '@/components/uiManifest/componentProps';

export function SubGroup({ title, layout = 'row', children }: SubGroupProps) {
  const layoutClass = {
    'row': 'flex flex-wrap items-start gap-3',
    'grid-2': 'grid grid-cols-2 gap-3',
    'grid-3': 'grid grid-cols-3 gap-3',
    'grid-4': 'grid grid-cols-4 gap-3',
  }[layout];

  return (
    <div
      className="relative w-full rounded-xl p-3 bg-gradient-to-br from-white/40 to-white/20 border border-[#B8860B]/15"
      style={{
        boxShadow: 'inset 0 1px 1px rgba(255,255,255,0.6), 0 2px 6px rgba(0,0,0,0.04)',
      }}
    >
      {/* Corner accent (smaller than Section) */}
      <div className="absolute top-1.5 right-1.5 w-4 h-4 border-r border-t border-[#B8860B]/20 rounded-tr-md" />

      {title && (
        <h3 className="text-[9px] font-bold uppercase tracking-[0.12em] text-[#1a1a1a]/50 mb-2">
          {title}
        </h3>
      )}
      <div className={layoutClass}>{children}</div>
    </div>
  );
}
