/**
 * Mocked Knob for Storybook
 * Wraps the real Knob with the mock parameter context
 */

import React, { useState } from 'react';

type KnobSize = 'sm' | 'md' | 'lg';

type MockedKnobProps = {
  paramId: number;
  label?: string;
  size?: KnobSize;
  initialValue?: number;
};

const SIZE_CONFIG = {
  sm: { outer: 44, ring: 15, center: 10, labelSize: 'text-[9px]', valueSize: 'text-[9px]', gap: 'gap-1' },
  md: { outer: 56, ring: 20, center: 14, labelSize: 'text-[10px]', valueSize: 'text-[10px]', gap: 'gap-1.5' },
  lg: { outer: 80, ring: 30, center: 22, labelSize: 'text-xs', valueSize: 'text-xs', gap: 'gap-2' },
} as const;

const PRIMARY_COLOR = '#fb923c';

export function MockedKnob({ paramId, label, size = 'md', initialValue = 0.75 }: MockedKnobProps) {
  const [value, setValue] = useState(initialValue);
  const [isDragging, setIsDragging] = useState(false);

  const config = SIZE_CONFIG[size];
  const normalizedValue = Math.max(0, Math.min(1, value));
  const circumference = 2 * Math.PI * config.ring;
  const dash = circumference * normalizedValue;

  const handleMouseDown = (e: React.MouseEvent) => {
    e.preventDefault();
    const startY = e.clientY;
    const startValue = value;
    setIsDragging(true);

    const handleMove = (ev: MouseEvent) => {
      const deltaY = startY - ev.clientY;
      const newValue = Math.max(0, Math.min(1, startValue + deltaY * 0.004));
      setValue(newValue);
    };

    const handleUp = () => {
      setIsDragging(false);
      document.removeEventListener('mousemove', handleMove);
      document.removeEventListener('mouseup', handleUp);
    };

    document.addEventListener('mousemove', handleMove);
    document.addEventListener('mouseup', handleUp);
  };

  const outerSize = config.outer;
  const center = outerSize / 2;
  const displayValue = (value * 100).toFixed(0) + '%';

  return (
    <div className={`flex flex-col items-center ${config.gap}`}>
      {label && (
        <label className={`text-orange-200 ${config.labelSize} font-bold uppercase tracking-wider`}>
          {label}
        </label>
      )}

      <div
        onMouseDown={handleMouseDown}
        className="relative rounded-full bg-gradient-to-br from-stone-900 to-black border-2 border-orange-600/50 cursor-pointer select-none"
        style={{
          width: outerSize,
          height: outerSize,
          display: 'flex',
          alignItems: 'center',
          justifyContent: 'center',
          boxShadow: isDragging
            ? `inset 0 4px 12px rgba(0,0,0,0.8), 0 0 16px rgba(251,146,60,0.3)`
            : '0 3px 10px rgba(0,0,0,0.5)',
        }}
      >
        <svg width={outerSize} height={outerSize} viewBox={`0 0 ${outerSize} ${outerSize}`}>
          <circle
            cx={center}
            cy={center}
            r={config.ring}
            fill="none"
            stroke="#ffffff15"
            strokeWidth={4}
          />
          <circle
            cx={center}
            cy={center}
            r={config.ring}
            fill="none"
            stroke={PRIMARY_COLOR}
            strokeWidth={4}
            strokeLinecap="round"
            strokeDasharray={`${dash} ${circumference - dash}`}
            transform={`rotate(-90 ${center} ${center})`}
            opacity={0.9}
          />
          <defs>
            <radialGradient id={`knob-grad-${paramId}`} cx="50%" cy="40%" r="70%">
              <stop offset="0%" stopColor="#292524" />
              <stop offset="100%" stopColor="#0c0a09" />
            </radialGradient>
          </defs>
          <circle
            cx={center}
            cy={center}
            r={config.center}
            fill={`url(#knob-grad-${paramId})`}
            stroke={PRIMARY_COLOR}
            strokeWidth={2}
          />
        </svg>
      </div>

      <div className={`text-orange-300 ${config.valueSize} font-bold text-center min-w-[40px]`}>
        {displayValue}
      </div>
    </div>
  );
}


