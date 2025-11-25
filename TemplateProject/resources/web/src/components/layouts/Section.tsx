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
};

export function Section({ title, description, children }: SectionProps) {
  return (
    <section
      aria-label={title}
      className="rounded-xl border border-orange-900/40 bg-black/40 p-5"
    >
      <header className="mb-4 flex items-center justify-between">
        <h2 className="text-xs font-semibold uppercase tracking-[0.3em] text-orange-400">
          {title}
        </h2>
        {description && (
          <p className="text-[11px] font-medium uppercase tracking-[0.2em] text-orange-200/60">
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


