/**
 * SubGroup Layout Component
 *
 * Groups related controls within a Section with optional title and layout.
 * Lighter version of Section styling.
 * Use grid-4 for ADSR sets, grid-2 for stereo pairs, row for default.
 */

import React from 'react';
import type { SubGroupProps } from './componentProps';

export type { SubGroupProps };

export function SubGroup({ title, layout = 'row', children }: SubGroupProps) {
  const layoutClass = {
    'row': 'flex flex-wrap items-center justify-center gap-4',
    'grid-2': 'grid grid-cols-2 gap-4 justify-items-center',
    'grid-3': 'grid grid-cols-3 gap-4 justify-items-center',
    'grid-4': 'grid grid-cols-4 gap-4 justify-items-center',
  }[layout];

  return (
    <div className="relative flex-1 min-w-0">
      {title && (
        <h3 className="text-[9px] font-semibold uppercase tracking-wider text-cyan-500/50 mb-2">
          {title}
        </h3>
      )}
      <div className={layoutClass}>{children}</div>
    </div>
  );
}
