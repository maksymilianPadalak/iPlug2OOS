import React from 'react';

export function ModulationSection() {
  return (
    <section
      aria-label="Modulation"
      className="rounded-xl border border-orange-900/40 bg-black/40 p-4 text-orange-100"
    >
      <header className="mb-2 text-xs font-semibold uppercase tracking-[0.3em] text-orange-400">
        Modulation
      </header>
      <p className="text-sm text-orange-200/80">
        TODO: Surface LFOs, envelopes, and modulation routing grid.
      </p>
    </section>
  );
}

