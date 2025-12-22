/**
 * Voice Card Component
 *
 * Universal card for instrument voices with optional trigger pulse animation.
 */

import React, { useRef, useEffect } from 'react';
import { triggerStore } from '../glue/state/triggerStore';
import type { VoiceCardProps } from './componentProps';

export type { VoiceCardProps };

type VoiceColor = NonNullable<VoiceCardProps['color']>;
type IconType = NonNullable<VoiceCardProps['icon']>;

const COLOR_CONFIG: Record<VoiceColor, {
  glow: string;
  glowBright: string;
  border: string;
  text: string;
}> = {
  orange: {
    glow: 'rgba(255,165,0,0.8)',
    glowBright: 'rgba(255,165,0,1)',
    border: 'border-orange-500/30',
    text: 'text-orange-400',
  },
  cyan: {
    glow: 'rgba(0,255,255,0.8)',
    glowBright: 'rgba(0,255,255,1)',
    border: 'border-cyan-500/30',
    text: 'text-cyan-400',
  },
  magenta: {
    glow: 'rgba(255,0,128,0.8)',
    glowBright: 'rgba(255,0,128,1)',
    border: 'border-pink-500/30',
    text: 'text-pink-400',
  },
  green: {
    glow: 'rgba(0,255,100,0.8)',
    glowBright: 'rgba(0,255,100,1)',
    border: 'border-green-500/30',
    text: 'text-green-400',
  },
};

// SVG waveform icons - universal names describing the visual/sonic character
const ICONS: Record<IconType, React.ReactNode> = {
  // Exponential decay envelope (bass thump, kick-like)
  decay: (
    <svg viewBox="0 0 40 24" className="w-10 h-6">
      <path d="M2 12 Q6 2, 10 12 Q14 22, 18 12 Q20 8, 22 12 Q24 14, 28 12 L38 12" fill="none" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" />
    </svg>
  ),
  // Noise burst with attack transient
  burst: (
    <svg viewBox="0 0 40 24" className="w-10 h-6">
      <path d="M2 12 L4 4 L6 18 L8 6 L10 16 L12 8 L14 14 L16 10 L18 13 L20 11 L24 12 L38 12" fill="none" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" />
    </svg>
  ),
  // High-frequency shimmer/oscillation
  shimmer: (
    <svg viewBox="0 0 40 24" className="w-10 h-6">
      <path d="M2 12 L4 8 L6 16 L8 6 L10 18 L12 8 L14 16 L16 10 L18 14 L20 11 L22 13 L24 12 L26 12 L38 12" fill="none" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" />
    </svg>
  ),
  // Pitched resonant decay
  resonant: (
    <svg viewBox="0 0 40 24" className="w-10 h-6">
      <path d="M2 12 Q5 4, 8 12 Q11 18, 14 12 Q16 8, 18 12 Q20 14, 24 12 Q26 11, 30 12 L38 12" fill="none" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" />
    </svg>
  ),
  // Multiple staccato hits/transients
  staccato: (
    <svg viewBox="0 0 40 24" className="w-10 h-6">
      <path d="M2 12 L4 6 L6 18 L8 12 L12 12 L14 8 L16 16 L18 12 L22 12 L24 10 L26 14 L28 12 L38 12" fill="none" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" />
    </svg>
  ),
  // Sharp transient spike
  spike: (
    <svg viewBox="0 0 40 24" className="w-10 h-6">
      <path d="M2 12 L8 12 L10 2 L12 12 L38 12" fill="none" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" />
    </svg>
  ),
  // Sine wave
  sine: (
    <svg viewBox="0 0 40 24" className="w-10 h-6">
      <path d="M2 12 Q8 2, 14 12 Q20 22, 26 12 Q32 2, 38 12" fill="none" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" />
    </svg>
  ),
  // Sawtooth wave
  saw: (
    <svg viewBox="0 0 40 24" className="w-10 h-6">
      <path d="M2 12 L10 4 L10 20 L20 4 L20 20 L30 4 L30 20 L38 12" fill="none" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" />
    </svg>
  ),
  // Square wave
  square: (
    <svg viewBox="0 0 40 24" className="w-10 h-6">
      <path d="M2 12 L2 4 L10 4 L10 20 L20 20 L20 4 L30 4 L30 20 L38 20 L38 12" fill="none" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" />
    </svg>
  ),
  // Random noise
  noise: (
    <svg viewBox="0 0 40 24" className="w-10 h-6">
      <path d="M2 12 L4 8 L6 15 L8 10 L10 14 L12 7 L14 16 L16 9 L18 13 L20 11 L22 15 L24 8 L26 14 L28 10 L30 13 L32 9 L34 14 L36 11 L38 12" fill="none" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" />
    </svg>
  ),
};

export function VoiceCard({
  title,
  color = 'cyan',
  triggerId,
  icon,
  children,
}: VoiceCardProps) {
  const config = COLOR_CONFIG[color];
  const indicatorRef = useRef<HTMLDivElement>(null);

  // Subscribe to trigger events - animate via CSS class (no React re-render)
  useEffect(() => {
    if (triggerId === undefined) return;

    let wasActive = false;

    const unsubscribe = triggerStore.subscribe(() => {
      const isActive = triggerStore.isActive(triggerId);

      if (isActive && !wasActive && indicatorRef.current) {
        const el = indicatorRef.current;
        el.classList.remove('triggered');
        void el.offsetWidth; // Force reflow
        el.classList.add('triggered');
      }

      wasActive = isActive;
    });

    return unsubscribe;
  }, [triggerId]);

  return (
    <section
      aria-label={title}
      className={`relative col-span-2 rounded-lg p-4 bg-black/30 ${config.border} border overflow-hidden`}
      style={{
        boxShadow: 'inset 0 1px 2px rgba(0,255,255,0.05), 0 2px 8px rgba(0,0,0,0.3)',
      }}
    >
      {/* Corner accent */}
      <div className="absolute top-1.5 left-1.5 w-4 h-4 border-l border-t border-cyan-500/40" />

      {/* Header */}
      <header className="mb-3 flex items-center gap-2">
        {/* Trigger indicator - only shown if triggerId is provided */}
        {triggerId !== undefined && (
          <div
            ref={indicatorRef}
            className="trigger-indicator w-4 h-4 rounded-full flex-shrink-0"
            style={{
              '--pulse-color': config.glow,
              backgroundColor: config.glowBright,
              boxShadow: `0 0 6px ${config.glow}, inset 0 0 2px rgba(255,255,255,0.3)`,
            } as React.CSSProperties}
          />
        )}

        {/* Waveform icon */}
        {icon && (
          <div className="opacity-40" style={{ color: config.glowBright }}>
            {ICONS[icon]}
          </div>
        )}

        {/* Decorative bar */}
        <div className="w-0.5 h-3 bg-cyan-500/60 rounded-full" />

        {/* Title */}
        <h2 className={`text-[11px] font-bold uppercase tracking-widest ${config.text}`}>
          {title}
        </h2>
      </header>

      {/* Content */}
      <div className="flex flex-col gap-3">
        {children}
      </div>
    </section>
  );
}
