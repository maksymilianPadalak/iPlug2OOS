/**
 * Title Component
 *
 * Clean circuit-style title with extending lines and subtle glow.
 */

import type { TitleProps } from './componentProps';

export type { TitleProps };

const colorConfig = {
  cyan: {
    primary: '#00ffff',
    dim: 'rgba(0, 255, 255, 0.3)',
    glow: 'rgba(0, 255, 255, 0.8)',
  },
  magenta: {
    primary: '#ff00ff',
    dim: 'rgba(255, 0, 255, 0.3)',
    glow: 'rgba(255, 0, 255, 0.8)',
  },
  green: {
    primary: '#00ff88',
    dim: 'rgba(0, 255, 136, 0.3)',
    glow: 'rgba(0, 255, 136, 0.8)',
  },
  orange: {
    primary: '#ff8800',
    dim: 'rgba(255, 136, 0, 0.3)',
    glow: 'rgba(255, 136, 0, 0.8)',
  },
};

export function Title({
  title,
  version,
  color = 'cyan',
}: TitleProps) {
  const colors = colorConfig[color];

  return (
    <header className="relative py-1">
      {/* Main title row with circuit lines */}
      <div className="flex items-center gap-3">
        {/* Left accent bars */}
        <div className="flex flex-col gap-1">
          <div
            className="h-[2px] rounded-full"
            style={{
              width: '20px',
              backgroundColor: colors.primary,
              boxShadow: `0 0 8px ${colors.glow}`,
            }}
          />
          <div
            className="h-[2px] rounded-full"
            style={{
              width: '12px',
              backgroundColor: colors.primary,
              opacity: 0.5,
            }}
          />
        </div>

        {/* Title */}
        <h1
          className="text-xl font-bold uppercase tracking-[0.25em]"
          style={{
            color: colors.primary,
            textShadow: `0 0 10px ${colors.glow}, 0 0 20px ${colors.dim}`,
            fontFamily: "'Orbitron', 'Rajdhani', monospace",
          }}
        >
          {title}
        </h1>

        {/* Extending line */}
        <div
          className="h-[1px] flex-1"
          style={{
            background: `linear-gradient(to right, ${colors.primary}80, transparent)`,
          }}
        />

        {/* Version */}
        {version && (
          <div className="flex items-center gap-2">
            <div
              className="w-1.5 h-1.5 rounded-full"
              style={{
                backgroundColor: colors.primary,
                boxShadow: `0 0 6px ${colors.glow}`,
              }}
            />
            <span
              className="text-xs font-bold uppercase tracking-wider"
              style={{
                color: colors.primary,
                opacity: 0.7,
                fontFamily: "'Orbitron', 'Rajdhani', monospace",
              }}
            >
              V{version}
            </span>
          </div>
        )}
      </div>
    </header>
  );
}
