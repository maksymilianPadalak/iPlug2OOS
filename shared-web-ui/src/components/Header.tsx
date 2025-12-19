/**
 * Header Layout Component
 *
 * Top region for Meters and main Gain knob.
 * Meters should be wrapped in a flex-1 container to expand.
 */

import React from 'react';
import type { HeaderProps } from './componentProps';

export type { HeaderProps };

export function Header({ children }: HeaderProps) {
  return (
    <div className="flex items-center gap-4 pb-4 border-b border-cyan-500/20">
      {children}
    </div>
  );
}
