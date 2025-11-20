/**
 * Plugin Header
 * Dreamy Psychedelic Reverb / Space Delay
 */

import React from 'react';

export function PluginHeader() {
  return (
    <div className="flex flex-col md:flex-row md:items-end items-center justify-between gap-3 mb-1 border-b border-cyan-700/40 pb-3">
      <div className="flex flex-col items-center md:items-start gap-1">
        <h1 className="text-3xl md:text-4xl font-black uppercase tracking-tight text-transparent bg-clip-text bg-gradient-to-r from-cyan-100 via-emerald-300 to-violet-300">
          BONIVERB
        </h1>
        <div className="h-0.5 w-16 bg-gradient-to-r from-transparent via-cyan-500 to-transparent" />
        <span className="text-cyan-300/70 text-[9px] md:text-[10px] uppercase tracking-[0.35em] font-bold">
          Dreamy Psychedelic Space
        </span>
      </div>

      <div className="flex flex-col items-center md:items-end gap-1 text-[9px] font-mono uppercase tracking-[0.25em] text-cyan-400/80">
        <span className="px-2 py-1 border border-cyan-600/60 bg-black/60">
          Input → Clouds → Output
        </span>
        <span className="px-2 py-0.5 border border-cyan-600/30 bg-black/40">
          Bon Iver • Hazy Choir Tails
        </span>
      </div>
    </div>
  );
}
