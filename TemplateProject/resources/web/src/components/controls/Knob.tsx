/**
 * Knob Control Component
 * 
 * Rotary control for continuous parameters (gain, frequency, etc.)
 * Supports sm/md/lg sizes for visual hierarchy.
 */

import React, { useRef, useState, useCallback, useEffect } from 'react';
import { normalizedToDisplay } from '../../utils/parameter';
import { useParameter } from '../../glue/hooks/useParameter';

type KnobSize = 'sm' | 'md' | 'lg';

type KnobProps = {
  paramId: number;
  label?: string;
  size?: KnobSize;
};

const SIZE_CONFIG = {
  sm: { outer: 44, ring: 15, center: 10, labelSize: 'text-[9px]', valueSize: 'text-[9px]', gap: 'gap-1' },
  md: { outer: 56, ring: 20, center: 14, labelSize: 'text-[10px]', valueSize: 'text-[10px]', gap: 'gap-1.5' },
  lg: { outer: 80, ring: 30, center: 22, labelSize: 'text-xs', valueSize: 'text-xs', gap: 'gap-2' },
} as const;

const PRIMARY_COLOR = '#fb923c';

function hexToRgba(hex: string, alpha = 1) {
  const h = hex.replace('#', '');
  const bigint = parseInt(h.length === 3 ? h.split('').map(c => c + c).join('') : h, 16);
  const r = (bigint >> 16) & 255;
  const g = (bigint >> 8) & 255;
  const b = bigint & 255;
  return `rgba(${r}, ${g}, ${b}, ${alpha})`;
}

function formatDisplayValue(displayStr: string): string {
  const match = displayStr.match(/^([\d.+-]+)\s*(.*)$/);
  if (!match) return displayStr;

  const num = parseFloat(match[1]);
  const unit = match[2];

  if (isNaN(num)) return displayStr;

  const formatted = Math.abs(num) >= 10 ? num.toFixed(1) : num.toFixed(2);
  return unit ? `${formatted} ${unit}` : formatted;
}

export function Knob({ paramId, label, size = 'md' }: KnobProps) {
  const { value, setValue, beginChange, endChange } = useParameter(paramId);
  const knobRef = useRef<HTMLDivElement>(null);
  const [isDragging, setIsDragging] = useState(false);

  const config = SIZE_CONFIG[size];
  const normalizedValue = Math.max(0, Math.min(1, value));
  const circumference = 2 * Math.PI * config.ring;
  const dash = circumference * normalizedValue;

  const clamp = (v: number) => Math.max(0, Math.min(1, v));

  const startDrag = useCallback((startClientY: number, startValue: number) => {
    setIsDragging(true);
    beginChange(); // Signal start of parameter change for automation
    const sensitivity = 0.004;

    const handleMove = (clientY: number) => {
      const deltaY = startClientY - clientY;
      const newValue = clamp(startValue + deltaY * sensitivity);

      setValue(newValue);
    };

    const onMouseMove = (ev: MouseEvent) => {
      ev.preventDefault();
      handleMove(ev.clientY);
    };

    const onTouchMove = (ev: TouchEvent) => {
      ev.preventDefault();
      if (ev.touches[0]) handleMove(ev.touches[0].clientY);
    };

    const onEndDrag = () => {
      endChange(); // Signal end of parameter change for automation
      setIsDragging(false);
      document.removeEventListener('mousemove', onMouseMove);
      document.removeEventListener('mouseup', onEndDrag);
      document.removeEventListener('touchmove', onTouchMove);
      document.removeEventListener('touchend', onEndDrag);
    };

    document.addEventListener('mousemove', onMouseMove, { passive: false });
    document.addEventListener('mouseup', onEndDrag);
    document.addEventListener('touchmove', onTouchMove, { passive: false });
    document.addEventListener('touchend', onEndDrag);
  }, [setValue, beginChange, endChange]);

  const handleMouseDown = useCallback((e: React.MouseEvent) => {
    e.preventDefault();
    startDrag(e.clientY, normalizedValue);
  }, [startDrag, normalizedValue]);

  const handleTouchStart = useCallback((e: React.TouchEvent) => {
    e.preventDefault();
    if (e.touches[0]) startDrag(e.touches[0].clientY, normalizedValue);
  }, [startDrag, normalizedValue]);

  // Inject keyframes once
  useEffect(() => {
    if (document.getElementById('knob-styles')) return;
    const style = document.createElement('style');
    style.id = 'knob-styles';
    style.innerHTML = `
      @keyframes knob-glow { 
        0%, 100% { filter: drop-shadow(0 0 0 rgba(251,146,60,0)); } 
        50% { filter: drop-shadow(0 4px 8px rgba(251,146,60,0.15)); } 
      }
    `;
    document.head.appendChild(style);
  }, []);

  const outerSize = config.outer;
  const center = outerSize / 2;

  return (
    <div className={`flex flex-col items-center ${config.gap}`}>
      {label && (
        <label className={`text-orange-200 ${config.labelSize} font-bold uppercase tracking-wider`}>
          {label}
        </label>
      )}

      <div
        ref={knobRef}
        onMouseDown={handleMouseDown}
        onTouchStart={handleTouchStart}
        className="relative rounded-full bg-gradient-to-br from-stone-900 to-black border-2 border-orange-600/50 cursor-pointer select-none"
        style={{
          width: outerSize,
          height: outerSize,
          display: 'flex',
          alignItems: 'center',
          justifyContent: 'center',
          boxShadow: isDragging
            ? `inset 0 4px 12px rgba(0,0,0,0.8), 0 0 16px ${hexToRgba(PRIMARY_COLOR, 0.3)}`
            : '0 3px 10px rgba(0,0,0,0.5)',
          animation: isDragging ? 'knob-glow 0.8s infinite' : undefined,
        }}
      >
        <svg width={outerSize} height={outerSize} viewBox={`0 0 ${outerSize} ${outerSize}`}>
          {/* Background ring */}
          <circle
            cx={center}
            cy={center}
            r={config.ring}
            fill="none"
            stroke="#ffffff15"
            strokeWidth={4}
          />

          {/* Progress ring */}
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

          {/* Center disc */}
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
        {formatDisplayValue(normalizedToDisplay(paramId, value))}
      </div>
    </div>
  );
}
