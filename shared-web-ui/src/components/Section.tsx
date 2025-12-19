/**
 * Section Layout Component
 *
 * Groups related controls with a title and optional description.
 * Futuristic dark theme with cyan accents.
 */

import React from 'react';
import type { SectionProps } from './componentProps';

export type { SectionProps };

export function Section({ title, description, children, size = 'compact' }: SectionProps) {
  const sizeClasses = {
    compact: 'col-span-1',
    wide: 'col-span-2',
    full: 'col-span-full w-full',
  };

  return (
    <section
      aria-label={title}
      className={`relative rounded-lg p-4 ${sizeClasses[size]} bg-black/30 border border-cyan-500/20`}
      style={{
        boxShadow: 'inset 0 1px 2px rgba(0,255,255,0.05), 0 2px 8px rgba(0,0,0,0.3)',
      }}
    >
      {/* Corner accent */}
      <div className="absolute top-1.5 left-1.5 w-4 h-4 border-l border-t border-cyan-500/40" />

      <header className="mb-3 flex items-center gap-2">
        {/* Decorative bar */}
        <div className="w-0.5 h-3 bg-cyan-500/60 rounded-full" />
        <div className="flex items-center justify-between flex-1">
          <h2 className="text-[11px] font-bold uppercase tracking-widest text-cyan-400/90">
            {title}
          </h2>
          {description && (
            <p className="text-[9px] font-medium uppercase tracking-wide text-cyan-500/50">
              {description}
            </p>
          )}
        </div>
      </header>
      <div>
        {children}
      </div>
    </section>
  );
}
