/**
 * Mocked Meter for Storybook
 */

import React from 'react';

type MockedMeterProps = {
  peakDb?: number;
  channel?: 0 | 1;
  compact?: boolean;
};

export function MockedMeter({ peakDb = -12, channel = 0, compact = false }: MockedMeterProps) {
  // Format dB display
  const formatDb = (db: number): string => {
    if (db === -Infinity || db <= -60) return '−∞';
    return db >= 0 ? `+${db.toFixed(1)}` : `${db.toFixed(1)}`;
  };

  const isClipping = peakDb >= -0.5;
  const isHot = peakDb >= -6 && peakDb < -0.5;

  return (
    <div className="flex items-center gap-2 bg-gradient-to-br from-stone-900/60 to-black/60 border border-orange-700/30 rounded-lg px-4 py-2 shadow-md">
      {/* Numeric display */}
      <div className={`text-lg font-black font-mono transition-colors ${
        isClipping ? 'text-red-400' : isHot ? 'text-orange-400' : 'text-orange-300/70'
      }`}>
        {formatDb(peakDb)} <span className="text-xs">dB</span>
      </div>
    </div>
  );
}

// Bar meter variant for more visual stories
type MockedBarMeterProps = {
  value?: number; // 0-1 normalized
  label?: string;
  orientation?: 'horizontal' | 'vertical';
};

export function MockedBarMeter({ value = 0.7, label, orientation = 'vertical' }: MockedBarMeterProps) {
  const percentage = Math.max(0, Math.min(1, value)) * 100;
  const isClipping = value >= 0.95;
  const isHot = value >= 0.7 && value < 0.95;

  const getColor = () => {
    if (isClipping) return 'bg-red-500';
    if (isHot) return 'bg-orange-500';
    return 'bg-emerald-500';
  };

  if (orientation === 'horizontal') {
    return (
      <div className="flex flex-col gap-1">
        {label && (
          <label className="text-orange-200 text-[10px] font-bold uppercase tracking-wider">
            {label}
          </label>
        )}
        <div className="w-32 h-4 bg-stone-900 border border-orange-700/30 rounded overflow-hidden">
          <div
            className={`h-full transition-all ${getColor()}`}
            style={{ width: `${percentage}%` }}
          />
        </div>
      </div>
    );
  }

  return (
    <div className="flex flex-col items-center gap-1">
      {label && (
        <label className="text-orange-200 text-[10px] font-bold uppercase tracking-wider">
          {label}
        </label>
      )}
      <div className="w-4 h-24 bg-stone-900 border border-orange-700/30 rounded overflow-hidden flex flex-col-reverse">
        <div
          className={`w-full transition-all ${getColor()}`}
          style={{ height: `${percentage}%` }}
        />
      </div>
    </div>
  );
}

