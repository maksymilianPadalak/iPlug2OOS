import React from 'react';
import { useMeter } from '@/glue/hooks/useMeter';

interface MeterProps {
  channel: 0 | 1;
  compact?: boolean;
}

export function Meter({ channel, compact = false }: MeterProps) {
  const meter = useMeter(channel);

  // Calculate dB values from linear amplitude (0-1)
  const peakDb = meter.peak > 0.0001 ? 20 * Math.log10(meter.peak) : -Infinity;
  const rmsValue = meter.rms !== undefined ? meter.rms : meter.peak * 0.7;
  const rmsDb = rmsValue > 0.0001 ? 20 * Math.log10(rmsValue) : -Infinity;

  // Format dB display
  const formatDb = (db: number): string => {
    if (db === -Infinity || db <= -60) return '−∞';
    return db >= 0 ? `+${db.toFixed(1)}` : `${db.toFixed(1)}`;
  };

  const dbValue = peakDb;
  const isClipping = dbValue >= -0.5;
  const isHot = dbValue >= -6 && dbValue < -0.5;

  return (
    <div className="flex items-center gap-2 bg-gradient-to-br from-stone-900/60 to-black/60 border border-orange-700/30 rounded-lg px-4 py-2 shadow-md w-24">
      {/* Numeric display - fixed width to prevent layout shift */}
      <div className={`text-lg font-black font-mono tabular-nums text-center w-full transition-colors ${
        isClipping ? 'text-red-400' : isHot ? 'text-orange-400' : 'text-orange-300/70'
      }`}>
        {formatDb(peakDb)} <span className="text-xs">dB</span>
      </div>
    </div>
  );
}
