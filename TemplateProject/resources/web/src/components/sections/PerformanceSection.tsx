import React from 'react';

import { KeyboardSection } from '../KeyboardSection';

export function PerformanceSection() {
  return (
    <section
      aria-label="Performance"
      className="rounded-xl border border-orange-900/40 bg-black/40 p-4 text-orange-100"
    >
      <header className="mb-4 flex items-center justify-between">
        <div className="text-xs font-semibold uppercase tracking-[0.3em] text-orange-400">
          Performance
        </div>
        <p className="text-[11px] font-medium uppercase tracking-[0.3em] text-orange-200/70">
          Play &amp; Capture
        </p>
      </header>

      <div className="space-y-4">
        <KeyboardSection />

        <div className="rounded-lg border border-orange-800/30 bg-white/5 p-4 text-sm text-orange-100/80">
          Add pads, arpeggiators, or MIDI learn widgets here so the AI can reason about live
          interaction requirements.
        </div>
      </div>
    </section>
  );
}

