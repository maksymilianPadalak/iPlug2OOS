import React from 'react';

import { OutputMeters } from '../OutputMeters';

export function RoutingSection() {
  return (
    <section
      aria-label="Routing & Output"
      className="rounded-xl border border-orange-900/40 bg-black/40 p-4 text-orange-100"
    >
      <header className="mb-4 flex flex-col gap-1">
        <div className="text-xs font-semibold uppercase tracking-[0.3em] text-orange-400">
          Routing &amp; Output
        </div>
        <p className="text-xs text-orange-200/80">
          Summaries for channel meters, DAW automation, and stem routing states.
        </p>
      </header>

      <div className="flex flex-col gap-6">
        <OutputMeters />

        <div className="rounded-lg border border-orange-800/30 bg-white/5 p-4 text-sm text-orange-100/80">
          Document send/return topology, surround mappings, and automation destinations here so the
          contract step can reason about signal flow before code generation.
        </div>
      </div>
    </section>
  );
}

