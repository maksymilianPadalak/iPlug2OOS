/**
 * Mocked Section for Storybook
 * Uses the real Section component since it has no dependencies on context
 */

import React from 'react';

type MockedSectionProps = {
  title: string;
  description?: string;
  children: React.ReactNode;
};

export function MockedSection({ title, description, children }: MockedSectionProps) {
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

