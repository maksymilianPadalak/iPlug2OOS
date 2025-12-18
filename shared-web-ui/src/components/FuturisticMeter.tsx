/**
 * FuturisticMeter Component
 *
 * Sleek continuous bar meter with smooth gradient and glow effects.
 */

import { useMeter } from '../glue/hooks/useMeter';
import type { FuturisticMeterProps } from './componentProps';

export type { FuturisticMeterProps };

const colorConfig = {
  cyan: {
    primary: '#00ffff',
    glow: 'rgba(0, 255, 255, 0.6)',
    dim: 'rgba(0, 255, 255, 0.1)',
  },
  magenta: {
    primary: '#ff00ff',
    glow: 'rgba(255, 0, 255, 0.6)',
    dim: 'rgba(255, 0, 255, 0.1)',
  },
  green: {
    primary: '#00ff88',
    glow: 'rgba(0, 255, 136, 0.6)',
    dim: 'rgba(0, 255, 136, 0.1)',
  },
  orange: {
    primary: '#ff8800',
    glow: 'rgba(255, 136, 0, 0.6)',
    dim: 'rgba(255, 136, 0, 0.1)',
  },
};

const HOT_THRESHOLD = 0.75;
const CLIP_THRESHOLD = 0.95;

export function FuturisticMeter({
  channel,
  label,
  color = 'cyan',
  showDb = true,
}: FuturisticMeterProps) {
  const meter = useMeter(channel);
  const colors = colorConfig[color];

  // Calculate dB from linear amplitude
  const peakDb = meter.peak > 0.0001 ? 20 * Math.log10(meter.peak) : -Infinity;

  // Format dB display
  const formatDb = (db: number): string => {
    if (db === -Infinity || db <= -60) return '-∞';
    return db >= 0 ? `+${db.toFixed(1)}` : db.toFixed(1);
  };

  // Normalize peak to 0-1 range (map -48dB to 0dB)
  const minDb = -48;
  const maxDb = 0;
  const clampedDb = Math.max(minDb, Math.min(maxDb, peakDb === -Infinity ? minDb : peakDb));
  const normalizedValue = (clampedDb - minDb) / (maxDb - minDb);

  const isHot = normalizedValue >= HOT_THRESHOLD;
  const isClipping = meter.peak >= CLIP_THRESHOLD;
  const channelLabel = label || (channel === 0 ? 'L' : 'R');

  // Determine bar color based on level
  const getBarGradient = () => {
    if (isClipping) {
      return `linear-gradient(to right, ${colors.primary}, #ffff00 70%, #ff0055 90%)`;
    }
    if (isHot) {
      return `linear-gradient(to right, ${colors.primary}, #ffff00)`;
    }
    return colors.primary;
  };

  const getGlowColor = () => {
    if (isClipping) return 'rgba(255, 0, 85, 0.8)';
    if (isHot) return 'rgba(255, 255, 0, 0.5)';
    return colors.glow;
  };

  return (
    <div className="flex items-center gap-3">
      {/* Channel label */}
      <span
        className="text-[10px] font-bold uppercase tracking-wider w-3"
        style={{
          color: colors.primary,
          textShadow: `0 0 6px ${colors.glow}`,
          fontFamily: "'Orbitron', 'Rajdhani', monospace",
        }}
      >
        {channelLabel}
      </span>

      {/* Meter track */}
      <div
        className="flex-1 h-[6px] rounded-full relative overflow-hidden"
        style={{
          backgroundColor: 'rgba(0,0,0,0.8)',
          border: `1px solid ${colors.primary}40`,
          boxShadow: `inset 0 0 4px ${colors.dim}`,
        }}
      >
        {/* Empty track indicator */}
        <div
          className="absolute inset-0 rounded-full"
          style={{
            background: `linear-gradient(to right, ${colors.primary}15, ${colors.primary}08)`,
          }}
        />
        {/* Fill bar */}
        <div
          className="absolute inset-y-0 left-0 rounded-full transition-all duration-75"
          style={{
            width: `${normalizedValue * 100}%`,
            background: getBarGradient(),
            boxShadow: `0 0 8px ${getGlowColor()}, 0 0 2px ${colors.primary}`,
          }}
        />
      </div>

      {/* dB readout */}
      {showDb && (
        <span
          className="text-[10px] font-medium tabular-nums w-12 text-right"
          style={{
            color: isClipping ? '#ff0055' : colors.primary,
            opacity: 0.8,
            fontFamily: "'Orbitron', 'Rajdhani', monospace",
          }}
        >
          {formatDb(peakDb)} dB
        </span>
      )}
    </div>
  );
}
