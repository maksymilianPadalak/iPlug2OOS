/**
 * PluginHeader Section
 * Plugin title and version display
 */

import React from 'react';

export function PluginHeader() {
  return (
    <div className="flex items-center justify-between mb-2">
      <h1 className="text-2xl font-black uppercase tracking-tight">
        <span className="bg-gradient-to-r from-orange-400 via-orange-500 to-red-500 text-transparent bg-clip-text">
          Simple
        </span>
        <span className="bg-gradient-to-r from-red-500 via-rose-500 to-orange-500 text-transparent bg-clip-text">
          Synth
        </span>
      </h1>
      <div className="flex items-center gap-3">
        <span className="text-orange-400/60 text-xs font-bold uppercase tracking-wider">Version 1.0</span>
      </div>
    </div>
  );
}
