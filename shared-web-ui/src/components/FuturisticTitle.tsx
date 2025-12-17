/**
 * FuturisticTitle Component
 *
 * A cyberpunk-style title with neon glow and animated effects.
 * Uses Orbitron font - ensure it's loaded in your HTML.
 */

import React from 'react';

export type FuturisticTitleProps = {
  title: string;
  version?: string;
  color?: 'cyan' | 'magenta' | 'green' | 'orange';
};

const colorStyles = {
  cyan: {
    text: 'text-cyan-400',
    border: 'border-cyan-500/30',
    glow: 'drop-shadow-[0_0_8px_rgba(0,255,255,0.8)] drop-shadow-[0_0_16px_rgba(0,255,255,0.4)]',
    bar: 'bg-cyan-400',
    scanline: 'from-transparent via-cyan-400 to-transparent',
  },
  magenta: {
    text: 'text-fuchsia-400',
    border: 'border-fuchsia-500/30',
    glow: 'drop-shadow-[0_0_8px_rgba(255,0,255,0.8)] drop-shadow-[0_0_16px_rgba(255,0,255,0.4)]',
    bar: 'bg-fuchsia-400',
    scanline: 'from-transparent via-fuchsia-400 to-transparent',
  },
  green: {
    text: 'text-emerald-400',
    border: 'border-emerald-500/30',
    glow: 'drop-shadow-[0_0_8px_rgba(0,255,128,0.8)] drop-shadow-[0_0_16px_rgba(0,255,128,0.4)]',
    bar: 'bg-emerald-400',
    scanline: 'from-transparent via-emerald-400 to-transparent',
  },
  orange: {
    text: 'text-orange-400',
    border: 'border-orange-500/30',
    glow: 'drop-shadow-[0_0_8px_rgba(255,165,0,0.8)] drop-shadow-[0_0_16px_rgba(255,165,0,0.4)]',
    bar: 'bg-orange-400',
    scanline: 'from-transparent via-orange-400 to-transparent',
  },
};

export function FuturisticTitle({
  title,
  version,
  color = 'cyan',
}: FuturisticTitleProps) {
  const styles = colorStyles[color];

  return (
    <header className={`relative flex items-center justify-between pb-4 border-b ${styles.border} overflow-hidden`}>
      {/* Animated scanline */}
      <div
        className={`absolute top-0 left-0 right-0 h-[1px] bg-gradient-to-r ${styles.scanline} animate-scanline`}
      />

      <div className="flex items-center gap-4">
        {/* Geometric accent bars */}
        <div className="flex flex-col gap-[2px]">
          <div className={`w-8 h-1 ${styles.bar} rounded-sm ${styles.glow}`} />
          <div className={`w-6 h-1 ${styles.bar} rounded-sm opacity-60`} />
          <div className={`w-4 h-1 ${styles.bar} rounded-sm opacity-30`} />
        </div>

        {/* Title */}
        <h1
          className={`text-2xl font-futuristic font-bold uppercase tracking-[0.2em] ${styles.text} ${styles.glow} animate-neon-pulse`}
        >
          {title}
        </h1>
      </div>

      {/* Version */}
      {version && (
        <div className="flex items-center gap-2">
          <div className={`w-2 h-2 rounded-full ${styles.bar} animate-pulse`} />
          <span className={`${styles.text} text-xs font-futuristic font-medium uppercase tracking-wider opacity-80`}>
            v{version}
          </span>
        </div>
      )}
    </header>
  );
}
