/**
 * Knob Control Component
 *
 * Futuristic HUD-style rotary control with chaotic rotating ovals.
 * Takes paramId and uses useParameter internally.
 * Drag up/down to change value.
 */

import { useRef, useState, useCallback, useMemo } from 'react';
import { useParameter } from '../glue/hooks/useParameter';
import { useRuntimeParameters, fromNormalized } from '../glue/RuntimeParametersProvider';
import type { KnobProps } from './componentProps';

export type { KnobProps };

const colorConfig = {
  cyan: {
    primary: '#00ffff',
    glow: 'rgba(0, 255, 255, 0.8)',
    dim: 'rgba(0, 255, 255, 0.3)',
    track: 'rgba(0, 255, 255, 0.15)',
  },
  magenta: {
    primary: '#ff00ff',
    glow: 'rgba(255, 0, 255, 0.8)',
    dim: 'rgba(255, 0, 255, 0.3)',
    track: 'rgba(255, 0, 255, 0.15)',
  },
  green: {
    primary: '#00ff88',
    glow: 'rgba(0, 255, 136, 0.8)',
    dim: 'rgba(0, 255, 136, 0.3)',
    track: 'rgba(0, 255, 136, 0.15)',
  },
  orange: {
    primary: '#ff8800',
    glow: 'rgba(255, 136, 0, 0.8)',
    dim: 'rgba(255, 136, 0, 0.3)',
    track: 'rgba(255, 136, 0, 0.15)',
  },
};

const SIZE_CONFIG = {
  small: { knobSize: 60, labelFont: 'text-[8px]', valueFont: 'text-[9px]', gap: 'gap-1' },
  medium: { knobSize: 90, labelFont: 'text-[10px]', valueFont: 'text-[11px]', gap: 'gap-2' },
};

// Oval configurations for CSS animation
// speed is degrees/second, duration = 360/speed
const ovalConfigs = [
  { rxRatio: 0.92, ryRatio: 0.72, duration: 8, direction: 'normal', opacity: 0.5, strokeWidth: 1.2, startAngle: 23 },
  { rxRatio: 0.68, ryRatio: 0.88, duration: 12, direction: 'reverse', opacity: 0.4, strokeWidth: 1, startAngle: 156 },
  { rxRatio: 0.85, ryRatio: 0.60, duration: 6, direction: 'normal', opacity: 0.6, strokeWidth: 1.5, startAngle: 89 },
  { rxRatio: 0.75, ryRatio: 0.95, duration: 15, direction: 'reverse', opacity: 0.35, strokeWidth: 0.8, startAngle: 267 },
  { rxRatio: 0.98, ryRatio: 0.78, duration: 10, direction: 'normal', opacity: 0.45, strokeWidth: 1.1, startAngle: 312 },
  { rxRatio: 0.62, ryRatio: 0.82, duration: 18, direction: 'reverse', opacity: 0.3, strokeWidth: 0.7, startAngle: 45 },
  { rxRatio: 0.80, ryRatio: 0.65, duration: 7, direction: 'normal', opacity: 0.55, strokeWidth: 1.3, startAngle: 198 },
  { rxRatio: 0.70, ryRatio: 0.90, duration: 14, direction: 'reverse', opacity: 0.38, strokeWidth: 0.9, startAngle: 134 },
  { rxRatio: 0.88, ryRatio: 0.58, duration: 9, direction: 'normal', opacity: 0.48, strokeWidth: 1.0, startAngle: 278 },
];

export function Knob({ paramId, label, color = 'cyan', size = 'medium' }: KnobProps) {
  const { value, setValue, beginChange, endChange } = useParameter(paramId);
  const runtimeParameters = useRuntimeParameters();
  const [isDragging, setIsDragging] = useState(false);
  const knobRef = useRef<HTMLDivElement>(null);

  const colors = colorConfig[color];
  const sizeConfig = SIZE_CONFIG[size];

  // Get parameter metadata for formatting
  const paramMeta = useMemo(
    () => runtimeParameters.find(p => p.id === paramId),
    [runtimeParameters, paramId]
  );

  // Format display value using min/max/unit/shape
  const displayValue = useMemo(() => {
    if (!paramMeta || paramMeta.min === undefined || paramMeta.max === undefined) {
      return `${Math.round(value * 100)}%`;
    }
    const actualValue = fromNormalized(
      value,
      paramMeta.min,
      paramMeta.max,
      paramMeta.shape,
      paramMeta.shapeParameter
    );
    const unit = paramMeta.unit || '';
    // Round appropriately based on range
    const rounded = actualValue >= 100 ? Math.round(actualValue) : Math.round(actualValue * 10) / 10;
    return `${rounded}${unit}`;
  }, [value, paramMeta]);

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

  const svgSize = sizeConfig.knobSize;
  const centerX = svgSize / 2;
  const centerY = svgSize / 2;
  const maxR = svgSize / 2 - 2;

  // Knob core
  const coreRadius = maxR * 0.32;

  // Progress arc
  const progressRadius = maxR * 0.55;
  const circumference = 2 * Math.PI * progressRadius;
  const arcLength = circumference * 0.75;

  return (
    <div className={`flex flex-col items-center ${sizeConfig.gap}`}>
      {/* CSS keyframes for knob rotation - injected once */}
      <style>{`
        @keyframes knobSpin {
          from { transform: rotate(0deg); }
          to { transform: rotate(360deg); }
        }
      `}</style>

      {label && (
        <label
          className={`font-bold uppercase tracking-[0.15em] ${sizeConfig.labelFont}`}
          style={{ color: colors.primary, textShadow: `0 0 8px ${colors.glow}` }}
        >
          {label}
        </label>
      )}

      <div
        ref={knobRef}
        onMouseDown={handleMouseDown}
        onTouchStart={handleTouchStart}
        className="cursor-pointer select-none"
        style={{
          filter: `drop-shadow(0 0 ${isDragging ? 12 : 8}px ${colors.glow})`,
          transition: 'filter 0.2s ease',
        }}
      >
        <svg width={svgSize} height={svgSize}>
          <defs>
            <radialGradient id={`knobGrad-${paramId}`} cx="35%" cy="35%">
              <stop offset="0%" stopColor="#252540" />
              <stop offset="100%" stopColor="#0a0a14" />
            </radialGradient>
          </defs>

          {/* Chaotic rotating ovals - pure CSS animation */}
          {ovalConfigs.map((oval, idx) => {
            // Calculate animation delay to start at the correct angle
            // startAngle / 360 * duration gives us how far into the cycle we should start
            const delay = -(oval.startAngle / 360) * oval.duration;
            return (
              <ellipse
                key={`oval-${idx}`}
                cx={centerX}
                cy={centerY}
                rx={maxR * oval.rxRatio}
                ry={maxR * oval.ryRatio}
                fill="none"
                stroke={colors.primary}
                strokeWidth={oval.strokeWidth}
                opacity={oval.opacity}
                style={{
                  transformOrigin: `${centerX}px ${centerY}px`,
                  animation: `knobSpin ${oval.duration}s linear infinite ${oval.direction}`,
                  animationDelay: `${delay}s`,
                }}
              />
            );
          })}

          {/* Progress arc background - more visible track */}
          <circle
            cx={centerX}
            cy={centerY}
            r={progressRadius}
            fill="none"
            stroke={colors.dim}
            strokeWidth={4}
            strokeDasharray={`${arcLength} ${circumference}`}
            transform={`rotate(135 ${centerX} ${centerY})`}
            opacity={0.6}
          />

          {/* Outer glow layer - intense */}
          <circle
            cx={centerX}
            cy={centerY}
            r={progressRadius}
            fill="none"
            stroke={colors.primary}
            strokeWidth={8}
            strokeLinecap="round"
            strokeDasharray={`${arcLength * value} ${circumference}`}
            transform={`rotate(135 ${centerX} ${centerY})`}
            opacity={0.5}
            style={{
              filter: `blur(4px)`,
              transition: isDragging ? 'none' : 'stroke-dasharray 0.1s ease',
            }}
          />
          {/* Mid glow layer */}
          <circle
            cx={centerX}
            cy={centerY}
            r={progressRadius}
            fill="none"
            stroke={colors.primary}
            strokeWidth={4}
            strokeLinecap="round"
            strokeDasharray={`${arcLength * value} ${circumference}`}
            transform={`rotate(135 ${centerX} ${centerY})`}
            style={{
              filter: `drop-shadow(0 0 6px ${colors.glow}) drop-shadow(0 0 12px ${colors.glow})`,
              transition: isDragging ? 'none' : 'stroke-dasharray 0.1s ease',
            }}
          />
          {/* Progress arc - white hot core */}
          <circle
            cx={centerX}
            cy={centerY}
            r={progressRadius}
            fill="none"
            stroke="#ffffff"
            strokeWidth={2}
            strokeLinecap="round"
            strokeDasharray={`${arcLength * value} ${circumference}`}
            transform={`rotate(135 ${centerX} ${centerY})`}
            style={{
              filter: `drop-shadow(0 0 2px #fff) drop-shadow(0 0 4px #fff) drop-shadow(0 0 6px ${colors.primary})`,
              transition: isDragging ? 'none' : 'stroke-dasharray 0.1s ease',
            }}
          />

          {/* Center knob */}
          <circle
            cx={centerX}
            cy={centerY}
            r={coreRadius}
            fill={`url(#knobGrad-${paramId})`}
            stroke={colors.dim}
            strokeWidth={1.5}
          />

          {/* Inner ring */}
          <circle
            cx={centerX}
            cy={centerY}
            r={coreRadius - 4}
            fill="none"
            stroke={colors.primary}
            strokeWidth={0.5}
            opacity={0.3}
          />

          {/* Center dot */}
          <circle
            cx={centerX}
            cy={centerY}
            r={3}
            fill={isDragging ? colors.primary : colors.dim}
            style={{
              filter: isDragging ? `drop-shadow(0 0 6px ${colors.glow})` : 'none',
              transition: 'fill 0.15s ease',
            }}
          />
        </svg>
      </div>

      <div
        className={`font-bold tabular-nums tracking-wider ${sizeConfig.valueFont}`}
        style={{ color: colors.primary, textShadow: `0 0 6px ${colors.dim}` }}
      >
        {displayValue}
      </div>
    </div>
  );
}
