/**
 * Plugin Header
 * Displays plugin title and subtitle
 */

import React from 'react';

export function PluginHeader() {
  return (
    <div className="flex flex-col items-center gap-2 mb-8">
      <h1 className="text-4xl font-black uppercase tracking-tighter text-transparent bg-clip-text bg-gradient-to-r from-cyan-200 via-cyan-400 to-cyan-200">
        DELAY
      </h1>
      <div className="h-0.5 w-12 bg-gradient-to-r from-transparent via-cyan-500 to-transparent" />
      <span className="text-cyan-300/60 text-[10px] uppercase tracking-widest font-bold">
        Effect Plugin
      </span>
    </div>
  );
}
