/**
 * Audio meter component - Berlin Brutalism Style
 */

import React from 'react';
import { useParameters } from './ParameterContext';

interface MeterProps {
  channel: 0 | 1;
}

export function Meter({ channel }: MeterProps) {
  const { meterValues } = useParameters();
  const meter = channel === 0 ? meterValues.left : meterValues.right;

  const db = meter.peak > 0.0001 ? 20 * Math.log10(meter.peak) : -60;
  const percentage = Math.max(0, Math.min(100, ((db + 60) / 60) * 100));

  return (
    <div className="flex-1 flex flex-col-reverse gap-0.5 bg-black border-2 border-white h-full relative">
      <div
        className="w-full bg-gradient-to-t from-red-500 via-yellow-500 to-green-500 transition-all duration-75"
        style={{
          minHeight: '0%',
          height: `${percentage}%`,
        }}
      />
    </div>
  );
}
