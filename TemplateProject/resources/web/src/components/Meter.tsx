/**
 * Audio meter component - Simple numeric display
 */

import React from 'react';
import { useParameters } from './ParameterContext';

interface MeterProps {
  channel: 0 | 1;
  compact?: boolean;
}

export function Meter({ channel, compact = false }: MeterProps) {
  const { meterValues } = useParameters();
  const meter = channel === 0 ? meterValues.left : meterValues.right;

  // Calculate dB values from linear amplitude (0-1)
  const peakDb = meter.peak > 0.0001 ? 20 * Math.log10(meter.peak) : -Infinity;
  const rmsValue = meter.rms !== undefined ? meter.rms : meter.peak * 0.7;
  const rmsDb = rmsValue > 0.0001 ? 20 * Math.log10(rmsValue) : -Infinity;

  // Format dB display
  const formatDb = (db: number): string => {
    if (db === -Infinity || db <= -60) return '−∞';
    return db >= 0 ? `+${db.toFixed(1)}` : `${db.toFixed(1)}`;
  };

  return (
    <div className="flex flex-col items-center gap-1">
      {/* Channel label */}
      <div className="text-white text-xs font-mono uppercase tracking-wider">
        {channel === 0 ? 'L' : 'R'}
      </div>

      {/* Numeric display */}
      <div className="text-white text-lg font-mono font-bold">
        {formatDb(peakDb)} dB
      </div>
    </div>
  );
}
