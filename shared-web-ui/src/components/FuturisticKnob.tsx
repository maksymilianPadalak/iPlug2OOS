"use client";

/**
 * FuturisticKnob Control Component
 *
 * Cyberpunk-style rotary control with neon glow effects.
 * Drag up/down to change value.
 */

import { useRef, useState, useCallback } from 'react';

export type FuturisticKnobProps = {
  /** Normalized value (0-1) */
  value: number;
  /** Called during drag with new normalized value */
  onChange: (value: number) => void;
  /** Called when drag starts (for DAW automation) */
  onBeginChange?: () => void;
  /** Called when drag ends (for DAW automation) */
  onEndChange?: () => void;
  /** Label displayed above the knob */
  label?: string;
  /** Formatted display value shown below the knob */
  displayValue?: string;
  /** Color theme */
  color?: 'cyan' | 'magenta' | 'green' | 'orange';
  /** Size variant */
  size?: 'small' | 'medium' | 'large';
};

const colorConfig = {
  cyan: {
    primary: '#00ffff',
    glow: 'rgba(0, 255, 255, 0.8)',
    dim: 'rgba(0, 255, 255, 0.2)',
  },
  magenta: {
    primary: '#ff00ff',
    glow: 'rgba(255, 0, 255, 0.8)',
    dim: 'rgba(255, 0, 255, 0.2)',
  },
  green: {
    primary: '#00ff88',
    glow: 'rgba(0, 255, 136, 0.8)',
    dim: 'rgba(0, 255, 136, 0.2)',
  },
  orange: {
    primary: '#ff8800',
    glow: 'rgba(255, 136, 0, 0.8)',
    dim: 'rgba(255, 136, 0, 0.2)',
  },
};

const sizeConfig = {
  small: { size: 48, stroke: 3, center: 6, indicator: 4 },
  medium: { size: 64, stroke: 4, center: 8, indicator: 5 },
  large: { size: 80, stroke: 5, center: 10, indicator: 6 },
};

export function FuturisticKnob({
  value,
  onChange,
  onBeginChange,
  onEndChange,
  label,
  displayValue,
  color = 'cyan',
  size = 'medium',
}: FuturisticKnobProps) {
  const [isDragging, setIsDragging] = useState(false);
  const knobRef = useRef<HTMLDivElement>(null);
  const colors = colorConfig[color];
  const sizes = sizeConfig[size];

  const clamp = (v: number) => Math.max(0, Math.min(1, v));

  const startDrag = useCallback(
    (startY: number, startValue: number) => {
      setIsDragging(true);
      onBeginChange?.();

      const onMove = (y: number) => {
        const delta = (startY - y) * 0.005;
        onChange(clamp(startValue + delta));
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
        onEndChange?.();
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
    [onChange, onBeginChange, onEndChange]
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
  const { size: svgSize, stroke, center, indicator } = sizes;
  const radius = (svgSize - stroke) / 2 - 2;
  const circumference = 2 * Math.PI * radius;
  const arcLength = circumference * 0.75; // 270 degrees
  const progress = arcLength * value;

  // Indicator position - starts at 135° (bottom-left) and sweeps 270° clockwise
  const startAngle = 135; // degrees, matching the arc rotation
  const indicatorAngle = (startAngle + value * 270) * (Math.PI / 180);
  const indicatorX = svgSize / 2 + Math.cos(indicatorAngle) * radius;
  const indicatorY = svgSize / 2 + Math.sin(indicatorAngle) * radius;

  return (
    <div className="flex flex-col items-center gap-2">
      {label && (
        <label
          className="text-[10px] font-futuristic font-bold uppercase tracking-[0.15em]"
          style={{ color: colors.primary, textShadow: `0 0 8px ${colors.glow}` }}
        >
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
            ? `drop-shadow(0 0 12px ${colors.glow})`
            : `drop-shadow(0 0 6px ${colors.dim})`,
        }}
      >
        <svg width={svgSize} height={svgSize}>
          {/* Outer glow ring */}
          <circle
            cx={svgSize / 2}
            cy={svgSize / 2}
            r={radius + 4}
            fill="none"
            stroke={colors.dim}
            strokeWidth={1}
          />

          {/* Background track - 270 degree arc */}
          <circle
            cx={svgSize / 2}
            cy={svgSize / 2}
            r={radius}
            fill="none"
            stroke="rgba(255,255,255,0.1)"
            strokeWidth={stroke}
            strokeLinecap="round"
            strokeDasharray={`${arcLength} ${circumference}`}
            transform={`rotate(135 ${svgSize / 2} ${svgSize / 2})`}
          />

          {/* Progress arc with glow */}
          <circle
            cx={svgSize / 2}
            cy={svgSize / 2}
            r={radius}
            fill="none"
            stroke={colors.primary}
            strokeWidth={stroke}
            strokeLinecap="round"
            strokeDasharray={`${progress} ${circumference}`}
            transform={`rotate(135 ${svgSize / 2} ${svgSize / 2})`}
            style={{
              filter: `drop-shadow(0 0 4px ${colors.glow})`,
            }}
          />

          {/* Center circle - dark with subtle gradient */}
          <circle
            cx={svgSize / 2}
            cy={svgSize / 2}
            r={center + 4}
            fill="url(#futuristicKnobGradient)"
            stroke={colors.dim}
            strokeWidth={1}
          />

          {/* Inner center dot */}
          <circle
            cx={svgSize / 2}
            cy={svgSize / 2}
            r={center / 2}
            fill={isDragging ? colors.primary : colors.dim}
            style={{
              filter: isDragging ? `drop-shadow(0 0 4px ${colors.glow})` : 'none',
              transition: 'fill 0.15s ease',
            }}
          />

          {/* Indicator dot */}
          <circle
            cx={indicatorX}
            cy={indicatorY}
            r={indicator / 2}
            fill={colors.primary}
            style={{
              filter: `drop-shadow(0 0 4px ${colors.glow})`,
            }}
          />

          <defs>
            <radialGradient id="futuristicKnobGradient" cx="30%" cy="30%">
              <stop offset="0%" stopColor="#2a2a2a" />
              <stop offset="100%" stopColor="#0a0a0a" />
            </radialGradient>
          </defs>
        </svg>
      </div>

      {displayValue && (
        <div
          className="text-[11px] font-futuristic font-medium tabular-nums"
          style={{ color: colors.primary, textShadow: `0 0 6px ${colors.dim}` }}
        >
          {displayValue}
        </div>
      )}
    </div>
  );
}
