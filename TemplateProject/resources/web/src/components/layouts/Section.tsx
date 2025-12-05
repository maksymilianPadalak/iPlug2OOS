/**
 * Section Layout Component
 * 
 * Groups related controls with a title and optional description.
 * AI generates sections based on parameter group field.
 */

import React from 'react';

type SectionProps = {
  title: string;
  description?: string;
  children: React.ReactNode;
  variant?: 'dark' | 'light';
};

export function Section({ title, description, children, variant = 'dark' }: SectionProps) {
  const bgClass = variant === 'light'
    ? 'bg-[#E8D4B8] border-[#1a1a1a]/20'
    : 'bg-[#1c1917] border-orange-900/40';

  const titleClass = variant === 'light'
    ? 'text-[#1a1a1a]'
    : 'text-orange-400';

  const descClass = variant === 'light'
    ? 'text-[#1a1a1a]/60'
    : 'text-orange-200/60';

  return (
    <section
      aria-label={title}
      className={`rounded-xl border p-5 ${bgClass}`}
    >
      <header className="mb-4 flex items-center justify-between">
        <h2 className={`text-xs font-semibold uppercase tracking-[0.3em] ${titleClass}`}>
          {title}
        </h2>
        {description && (
          <p className={`text-[11px] font-medium uppercase tracking-[0.2em] ${descClass}`}>
            {description}
          </p>
        )}
      </header>
      <div className="flex flex-wrap items-start gap-6">
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
type ControlRowProps = {
  children: React.ReactNode;
};

export function ControlRow({ children }: ControlRowProps) {
  return (
    <div className="flex flex-wrap items-end gap-4">
      {children}
    </div>
  );
}




