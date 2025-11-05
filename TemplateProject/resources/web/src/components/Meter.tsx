/**
 * Audio meter component
 */

import React from 'react';
import { useParameters } from './ParameterContext';

interface MeterProps {
  channel: 0 | 1;
}

export function Meter({ channel }: MeterProps) {
  const { meterValues } = useParameters();
  const meter = channel === 0 ? meterValues.left : meterValues.right;

  // Convert linear value to dB and then to percentage
  const db = meter.peak > 0.0001 ? 20 * Math.log10(meter.peak) : -60;
  const percentage = Math.max(0, Math.min(100, ((db + 60) / 60) * 100));

  return (
    <div
      style={{
        flex: 1,
        display: 'flex',
        flexDirection: 'column-reverse',
        gap: '2px',
        background: 'rgba(255,255,255,0.1)',
        border: '2px solid #ffffff',
        position: 'relative',
      }}
    >
      <div
        style={{
          background: 'linear-gradient(to top, #ff0000 0%, #ffaa00 70%, #00ff00 100%)',
          width: '100%',
          minHeight: '0%',
          height: `${percentage}%`,
          transition: 'height 0.05s linear',
        }}
      />
    </div>
  );
}

