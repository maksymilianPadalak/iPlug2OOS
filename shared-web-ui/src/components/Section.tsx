/**
 * Section Layout Component
 *
 * Groups related controls with a title and optional description.
 * Streamline Moderne style with brass accents and cream backgrounds.
 */

import React from 'react';
import type { SectionProps } from './componentProps';

export type { SectionProps };

export function Section({ title, description, children, variant = 'light', size = 'compact' }: SectionProps) {
  const isDark = variant === 'dark';

  const sizeClasses = {
    compact: 'col-span-1',
    wide: 'col-span-2',
    full: 'col-span-full',
  };

  return (
    <section
      aria-label={title}
      className={`relative rounded-2xl p-5 h-full ${sizeClasses[size]} ${
        isDark
          ? 'bg-[#1c1917] border border-orange-900/40'
          : 'bg-gradient-to-br from-white/60 to-white/30 backdrop-blur-sm border border-[#B8860B]/20'
      }`}
      style={!isDark ? {
        boxShadow: 'inset 0 1px 2px rgba(255,255,255,0.8), 0 4px 12px rgba(0,0,0,0.08)',
      } : undefined}
    >
      {/* Corner accent */}
      {!isDark && (
        <div className="absolute top-2 right-2 w-6 h-6 border-r border-t border-[#B8860B]/30 rounded-tr-lg" />
      )}

      <header className="mb-4 flex items-center gap-3">
        {/* Decorative bar */}
        <div className={`w-1 h-4 rounded-full ${
          isDark
            ? 'bg-gradient-to-b from-orange-400 to-orange-600'
            : 'bg-gradient-to-b from-[#B8860B] to-[#8B6914]'
        }`} />
        <div className="flex items-center justify-between flex-1">
          <h2 className={`text-[12px] font-black uppercase tracking-[0.15em] ${
            isDark ? 'text-orange-400' : 'text-[#1a1a1a]'
          }`}>
            {title}
          </h2>
          {description && (
            <p className={`text-[10px] font-semibold uppercase tracking-[0.1em] ${
              isDark ? 'text-orange-200/60' : 'text-[#3a3a3a]'
            }`}>
              {description}
            </p>
          )}
        </div>
      </header>
      <div className="flex flex-wrap items-start gap-4 [&>*]:flex-shrink-0">
        {children}
      </div>
    </section>
  );
}

/**
 * ControlRow Layout Component
 *
 * Arranges controls horizontally with consistent spacing.
 */
export type ControlRowProps = {
  children: React.ReactNode;
};

export function ControlRow({ children }: ControlRowProps) {
  return (
    <div className="flex flex-wrap items-end gap-4">
      {children}
    </div>
  );
}
