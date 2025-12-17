/**
 * FuturisticMeter Component
 *
 * Horizontal LED bar meter with cyberpunk styling.
 * Neon glow effects with cyan/magenta color scheme.
 *
 * TODO: Add decay animation so meter falls to 0 when audio stops.
 * The decay should be handled either here or in the meterStore.
 */

import { useMeter } from '../glue/hooks/useMeter';

export type FuturisticMeterProps = {
  channel: 0 | 1;
  label?: string;
  color?: 'cyan' | 'magenta' | 'green' | 'orange';
  showDb?: boolean;
};

const colorConfig = {
  cyan: {
    normal: '#00ffff',
    hot: '#ffff00',
    clip: '#ff0055',
    glow: 'rgba(0, 255, 255, 0.6)',
    dim: 'rgba(0, 255, 255, 0.15)',
  },
  magenta: {
    normal: '#ff00ff',
    hot: '#ffff00',
    clip: '#ff0055',
    glow: 'rgba(255, 0, 255, 0.6)',
    dim: 'rgba(255, 0, 255, 0.15)',
  },
  green: {
    normal: '#00ff88',
    hot: '#ffff00',
    clip: '#ff0055',
    glow: 'rgba(0, 255, 136, 0.6)',
    dim: 'rgba(0, 255, 136, 0.15)',
  },
  orange: {
    normal: '#ff8800',
    hot: '#ffff00',
    clip: '#ff0055',
    glow: 'rgba(255, 136, 0, 0.6)',
    dim: 'rgba(255, 136, 0, 0.15)',
  },
};

const SEGMENT_COUNT = 24;
const HOT_THRESHOLD = 0.7; // -3dB roughly
const CLIP_THRESHOLD = 0.95; // -0.5dB roughly

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

  // Calculate how many segments are lit
  const litSegments = Math.floor(normalizedValue * SEGMENT_COUNT);

  const isClipping = meter.peak >= CLIP_THRESHOLD;
  const channelLabel = label || (channel === 0 ? 'L' : 'R');

  return (
    <div className="flex flex-col gap-1">
      {/* Label and dB readout */}
      <div className="flex items-center justify-between px-1">
        <span
          className="text-[10px] font-futuristic font-bold uppercase tracking-wider"
          style={{ color: colors.normal, textShadow: `0 0 8px ${colors.glow}` }}
        >
          {channelLabel}
        </span>
        {showDb && (
          <span
            className="text-[10px] font-futuristic font-medium tabular-nums"
            style={{
              color: isClipping ? colors.clip : colors.normal,
              textShadow: `0 0 8px ${isClipping ? 'rgba(255,0,85,0.8)' : colors.glow}`,
            }}
          >
            {formatDb(peakDb)} dB
          </span>
        )}
      </div>

      {/* Meter bar container */}
      <div
        className="relative h-3 rounded-sm overflow-hidden"
        style={{
          background: 'linear-gradient(180deg, #0a0a0a 0%, #1a1a1a 100%)',
          boxShadow: `inset 0 1px 3px rgba(0,0,0,0.8), 0 0 1px ${colors.dim}`,
          border: `1px solid ${colors.dim}`,
        }}
      >
        {/* LED segments */}
        <div className="absolute inset-0 flex gap-[2px] p-[2px]">
          {Array.from({ length: SEGMENT_COUNT }).map((_, i) => {
            const isLit = i < litSegments;
            const segmentPosition = i / SEGMENT_COUNT;
            const isHotSegment = segmentPosition >= HOT_THRESHOLD;
            const isClipSegment = segmentPosition >= CLIP_THRESHOLD;

            let segmentColor = colors.dim;
            let glowColor = 'transparent';

            if (isLit) {
              if (isClipSegment) {
                segmentColor = colors.clip;
                glowColor = 'rgba(255, 0, 85, 0.8)';
              } else if (isHotSegment) {
                segmentColor = colors.hot;
                glowColor = 'rgba(255, 255, 0, 0.6)';
              } else {
                segmentColor = colors.normal;
                glowColor = colors.glow;
              }
            }

            return (
              <div
                key={i}
                className="flex-1 rounded-[1px] transition-all duration-75"
                style={{
                  background: isLit
                    ? `linear-gradient(180deg, ${segmentColor} 0%, ${segmentColor}dd 50%, ${segmentColor}99 100%)`
                    : colors.dim,
                  boxShadow: isLit ? `0 0 6px ${glowColor}, inset 0 1px 1px rgba(255,255,255,0.3)` : 'none',
                }}
              />
            );
          })}
        </div>

        {/* Scanline overlay */}
        <div
          className="absolute inset-0 pointer-events-none opacity-20"
          style={{
            background: 'repeating-linear-gradient(0deg, transparent, transparent 1px, rgba(0,0,0,0.3) 1px, rgba(0,0,0,0.3) 2px)',
          }}
        />
      </div>
    </div>
  );
}
