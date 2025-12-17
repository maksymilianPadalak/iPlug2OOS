/**
 * Knob Control Component
 *
 * Rotary control for continuous parameters.
 * Takes paramId and uses useParameter internally.
 * Drag up/down to change value.
 */

import { useRef, useState, useCallback } from 'react';
import { useParameter } from '../glue/hooks/useParameter';

export type KnobProps = {
  /** Parameter ID from EParams enum */
  paramId: number;
  /** Label displayed above the knob */
  label?: string;
};

export function Knob({ paramId, label }: KnobProps) {
  const { value, setValue, beginChange, endChange } = useParameter(paramId);
  const [isDragging, setIsDragging] = useState(false);
  const knobRef = useRef<HTMLDivElement>(null);

  const clamp = (v: number) => Math.max(0, Math.min(1, v));

  const startDrag = useCallback(
    (startY: number, startValue: number) => {
      setIsDragging(true);
      beginChange();

      const onMove = (y: number) => {
        const delta = (startY - y) * 0.005;
        setValue(clamp(startValue + delta));
      };

      const onMouseMove = (e: MouseEvent) => {
        e.preventDefault();
        onMove(e.clientY);
      };

      const onTouchMove = (e: TouchEvent) => {
        e.preventDefault();
        if (e.touches[0]) onMove(e.touches[0].clientY);
      };

      const onEnd = () => {
        endChange();
        setIsDragging(false);
        document.removeEventListener('mousemove', onMouseMove);
        document.removeEventListener('mouseup', onEnd);
        document.removeEventListener('touchmove', onTouchMove);
        document.removeEventListener('touchend', onEnd);
      };

      document.addEventListener('mousemove', onMouseMove, { passive: false });
      document.addEventListener('mouseup', onEnd);
      document.addEventListener('touchmove', onTouchMove, { passive: false });
      document.addEventListener('touchend', onEnd);
    },
    [setValue, beginChange, endChange]
  );

  const handleMouseDown = (e: React.MouseEvent) => {
    e.preventDefault();
    startDrag(e.clientY, value);
  };

  const handleTouchStart = (e: React.TouchEvent) => {
    e.preventDefault();
    if (e.touches[0]) startDrag(e.touches[0].clientY, value);
  };

  // Arc calculation
  const size = 48;
  const stroke = 4;
  const radius = (size - stroke) / 2;
  const circumference = 2 * Math.PI * radius;
  // Arc spans 270 degrees (from -135° to +135°), so use 0.75 of circumference
  const arcLength = circumference * 0.75;
  const progress = arcLength * value;

  return (
    <div className="flex flex-col items-center gap-1.5">
      {label && (
        <label className="text-[#1a1a1a] text-[10px] font-bold uppercase tracking-[0.08em]">
          {label}
        </label>
      )}

      <div
        ref={knobRef}
        onMouseDown={handleMouseDown}
        onTouchStart={handleTouchStart}
        className={`cursor-pointer select-none transition-transform ${isDragging ? 'scale-105' : 'hover:scale-102'}`}
        style={{
          filter: isDragging
            ? 'drop-shadow(0 4px 8px rgba(184, 134, 11, 0.3))'
            : 'drop-shadow(0 2px 4px rgba(0,0,0,0.1))',
        }}
      >
        <svg width={size} height={size}>
          {/* Outer ring shadow effect */}
          <circle
            cx={size / 2}
            cy={size / 2}
            r={radius + 2}
            fill="none"
            stroke="rgba(0,0,0,0.05)"
            strokeWidth={1}
          />
          {/* Background track - 270 degree arc */}
          <circle
            cx={size / 2}
            cy={size / 2}
            r={radius}
            fill="none"
            stroke="#D4C4B0"
            strokeWidth={stroke}
            strokeLinecap="round"
            strokeDasharray={`${arcLength} ${circumference}`}
            transform={`rotate(135 ${size / 2} ${size / 2})`}
          />
          {/* Progress arc - brass/gold gradient effect */}
          <circle
            cx={size / 2}
            cy={size / 2}
            r={radius}
            fill="none"
            stroke="#B8860B"
            strokeWidth={stroke}
            strokeLinecap="round"
            strokeDasharray={`${progress} ${circumference}`}
            transform={`rotate(135 ${size / 2} ${size / 2})`}
          />
          {/* Center knob with metallic effect */}
          <circle
            cx={size / 2}
            cy={size / 2}
            r={8}
            fill="url(#knobGradient)"
            stroke="#B8860B"
            strokeWidth={1}
          />
          {/* Indicator line */}
          <line
            x1={size / 2}
            y1={size / 2 - 3}
            x2={size / 2}
            y2={size / 2 - 6}
            stroke="#5D4E37"
            strokeWidth={1.5}
            strokeLinecap="round"
            transform={`rotate(${value * 270 - 135} ${size / 2} ${size / 2})`}
          />
          <defs>
            <radialGradient id="knobGradient" cx="30%" cy="30%">
              <stop offset="0%" stopColor="#FFF8E7" />
              <stop offset="100%" stopColor="#E8D4B8" />
            </radialGradient>
          </defs>
        </svg>
      </div>

      <div className="text-[#2a2a2a] text-[11px] font-semibold tabular-nums">
        {Math.round(value * 100)}%
      </div>
    </div>
  );
}
