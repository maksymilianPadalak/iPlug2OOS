/**
 * Brutalist Knob component - Berlin Brutalism Style
 */

import React, { useRef, useState, useCallback, useEffect } from 'react';
import { EParams } from '../config/constants';
import { normalizedToDisplay } from '../utils/parameter';
import { useParameters } from './ParameterContext';
import { sendParameterValue } from '../communication/iplug-bridge';
import { isUpdatingFromProcessor } from './ParameterContext';

interface KnobProps {
  paramIdx: EParams;
  label: string;
  min?: number;
  max?: number;
  step?: number;
}

export function Knob({ paramIdx, label, min = 0, max = 1, step = 0.001 }: KnobProps) {
  const { paramValues, setParamValue } = useParameters();
  const value = paramValues.get(paramIdx) ?? 0;
  const knobRef = useRef<HTMLDivElement>(null);
  const [isDragging, setIsDragging] = useState(false);

  // compute normalized value in 0..1 regardless of min/max
  const normalizedValue = Math.max(0, Math.min(1, (value - min) / (max - min)));
  const rotation = normalizedValue * 270 - 135; // -135° to +135°

  // Helper to clamp
  const clamp = (v: number, a = 0, b = 1) => Math.max(a, Math.min(b, v));

  // Unified move handler (works for mouse and touch)
  const startDrag = useCallback((startClientY: number, startValue: number) => {
    setIsDragging(true);
    const startY = startClientY;
    const startVal = startValue;

    const sensitivity = 0.0045; // tuned for expressiveness without being twitchy

    const handleMove = (clientY: number) => {
      const deltaY = startY - clientY; // upward increases
      // Creative non-linear response: combine linear + subtle exponential for finer low-end control
      const linear = startVal + deltaY * sensitivity;
      const eased = Math.pow(clamp(linear), 1.05);
      const newValue = clamp(eased);

      setParamValue(paramIdx, newValue);

      if (!isUpdatingFromProcessor()) {
        sendParameterValue(paramIdx, newValue);
      }
    };

    const onMouseMove = (ev: MouseEvent) => {
      ev.preventDefault();
      handleMove(ev.clientY);
    };

    const onTouchMove = (ev: TouchEvent) => {
      ev.preventDefault();
      if (ev.touches && ev.touches[0]) handleMove(ev.touches[0].clientY);
    };

    const endDrag = () => {
      setIsDragging(false);
      document.removeEventListener('mousemove', onMouseMove);
      document.removeEventListener('mouseup', endDrag);
      document.removeEventListener('touchmove', onTouchMove);
      document.removeEventListener('touchend', endDrag);
    };

    document.addEventListener('mousemove', onMouseMove, { passive: false });
    document.addEventListener('mouseup', endDrag);
    document.addEventListener('touchmove', onTouchMove, { passive: false });
    document.addEventListener('touchend', endDrag);
  }, [paramIdx, setParamValue]);

  const handleMouseDown = useCallback((e: React.MouseEvent) => {
    e.preventDefault();
    startDrag(e.clientY, normalizedValue);
  }, [startDrag, normalizedValue]);

  const handleTouchStart = useCallback((e: React.TouchEvent) => {
    e.preventDefault();
    if (e.touches && e.touches[0]) {
      startDrag(e.touches[0].clientY, normalizedValue);
    }
  }, [startDrag, normalizedValue]);

  // Inject small keyframes for gentle glowing effect once
  useEffect(() => {
    if (document.getElementById('knob-styles')) return;
    const style = document.createElement('style');
    style.id = 'knob-styles';
    style.innerHTML = `
      @keyframes knob-pulse { 0% { filter: drop-shadow(0 0 0 rgba(255,255,255,0)); } 50% { filter: drop-shadow(0 6px 10px rgba(255,255,255,0.06)); } 100% { filter: drop-shadow(0 0 0 rgba(255,255,255,0)); } }
      .knob-outer { transition: box-shadow 120ms ease, transform 140ms ease; }
      .knob-pointer { transition: transform 120ms cubic-bezier(.22,.9,.4,1); }
      .knob-inner-gear { transition: transform 220ms cubic-bezier(.22,.9,.4,1); }
    `;
    document.head.appendChild(style);
  }, []);

  // Special main GAIN knob styled like a Valhalla-style multicircle main knob
  if (paramIdx === EParams.kParamGain) {
    const size = 120; // SVG size
    const center = size / 2;
    const ringCount = 5;
    const baseRadius = 44;
    const ringSpacing = 8;
    const colors = ['#ff4d6d', '#ffb86b', '#ffd56b', '#6be3ff', '#9b6bff'];

    return (
      <div className="flex flex-col items-center gap-2">
        <label className="text-white text-xs font-mono uppercase tracking-wider">
          {label}
        </label>
        <div
          ref={knobRef}
          onMouseDown={handleMouseDown}
          onTouchStart={handleTouchStart}
          className={`relative rounded-full bg-black border-4 border-white cursor-pointer select-none knob-outer`
          }
          style={{ width: `${size / 1.2}px`, height: `${size / 1.2}px`, display: 'flex', alignItems: 'center', justifyContent: 'center', boxShadow: isDragging ? 'inset 0 6px 18px rgba(0,0,0,0.6)' : undefined, animation: isDragging ? 'knob-pulse 0.9s infinite' : undefined }}
        >
          <svg width={size} height={size} viewBox={`0 0 ${size} ${size}`}>
            {/* Multicolored rings showing the value */}
            {Array.from({ length: ringCount }).map((_, i) => {
              const radius = baseRadius - i * ringSpacing;
              const circumference = 2 * Math.PI * radius;
              const progress = normalizedValue; // same progress for all rings
              const dash = circumference * progress;

              return (
                <circle
                  key={i}
                  cx={center}
                  cy={center}
                  r={radius}
                  fill="none"
                  stroke={colors[i % colors.length]}
                  strokeWidth={6}
                  strokeLinecap="round"
                  strokeDasharray={`${dash} ${circumference - dash}`}
                  transform={`rotate(-90 ${center} ${center})`}
                  opacity={0.95 - i * 0.08}
                />
              );
            })}

            {/* Center plate */}
            <defs>
              <radialGradient id="kg" cx="50%" cy="40%" r="70%">
                <stop offset="0%" stopColor="#1b1b1b" />
                <stop offset="100%" stopColor="#000000" />
              </radialGradient>
            </defs>

            <circle cx={center} cy={center} r={28} fill="url(#kg)" stroke="#ffffff" strokeWidth={4} />

            {/* Pointer line - use CSS transform (translate to center then rotate) to avoid SVG attribute/CSS transform conflicts */}
            <g
              className="knob-pointer"
              style={{ transform: `translate(${center}px, ${center}px) rotate(${rotation}deg)` }}
            >
              {/* rect positioned relative to translated origin (center) */}
              <rect x={-3} y={-44} width={6} height={18} rx={3} fill="#ffffff" />
            </g>

            {/* Decorative inner small rotating gear (responds to value) */}
            <g className="knob-inner-gear" transform={`rotate(${rotation * 0.6} ${center} ${center})`}>
              <circle cx={center} cy={center} r={12} fill="#111" />
              {Array.from({ length: 6 }).map((_, i) => {
                const a = (i / 6) * Math.PI * 2;
                const x = center + Math.cos(a) * 18;
                const y = center + Math.sin(a) * 18;
                return <rect key={i} x={x - 2} y={y - 2} width={4} height={4} rx={1} fill="#222" transform={`rotate(${(i * 60)} ${x} ${y})`} />;
              })}
            </g>

            {/* Decorative inner small rings */}
            <circle cx={center} cy={center} r={18} fill="none" stroke="#333" strokeWidth={2} />
            <circle cx={center} cy={center} r={12} fill="#111" />
          </svg>
        </div>
        <div className="text-gray-400 text-[10px] font-mono text-center min-w-[60px]">
          {normalizedToDisplay(paramIdx, value)}
        </div>
      </div>
    );
  }

  // Default small brutalist knob - upgraded visuals but NO size changes on interaction
  const smallSize = 56;
  const center = smallSize / 2;
  const ringRadius = 20;
  const circumference = 2 * Math.PI * ringRadius;
  const progress = normalizedValue;
  const dash = circumference * progress;

  return (
    <div className="flex flex-col items-center gap-1">
      <label className="text-white text-xs font-mono uppercase tracking-wider">
        {label}
      </label>

      <div
        ref={knobRef}
        onMouseDown={handleMouseDown}
        onTouchStart={handleTouchStart}
        className="relative bg-black border-4 border-blue-500 cursor-pointer select-none"
        style={{ width: `${smallSize}px`, height: `${smallSize}px`, display: 'flex', alignItems: 'center', justifyContent: 'center', boxShadow: isDragging ? 'inset 0 6px 14px rgba(0,0,0,0.6)' : '0 2px 8px rgba(0,0,0,0.35)', borderRadius: 999 }}
      >
        <svg width={smallSize} height={smallSize} viewBox={`0 0 ${smallSize} ${smallSize}`}>
          {/* Outer progress ring */}
          <circle
            cx={center}
            cy={center}
            r={ringRadius}
            fill="none"
            stroke="#ffffff22"
            strokeWidth={6}
          />

          <circle
            cx={center}
            cy={center}
            r={ringRadius}
            fill="none"
            stroke="#9be7ff"
            strokeWidth={6}
            strokeLinecap="round"
            strokeDasharray={`${dash} ${circumference - dash}`}
            transform={`rotate(-90 ${center} ${center})`}
            opacity={0.95}
          />

          {/* Center disc with subtle sheen */}
          <defs>
            <radialGradient id={`g-${paramIdx}`} cx="50%" cy="40%" r="70%">
              <stop offset="0%" stopColor="#111" />
              <stop offset="100%" stopColor="#000" />
            </radialGradient>
          </defs>
          <circle cx={center} cy={center} r={12} fill={`url(#g-${paramIdx})`} stroke="#ffffff" strokeWidth={2} />

          {/* Pointer - use CSS transform for stable rotation */}
          <g
            className="knob-pointer"
            style={{ transform: `translate(${center}px, ${center}px) rotate(${rotation}deg)` }}
          >
            <rect x={-2} y={-14} width={4} height={10} rx={2} fill="#ffffff" />
          </g>

          {/* Tiny decorative ticks */}
          {Array.from({ length: 8 }).map((_, i) => {
            const a = ((i / 8) * 270 - 135) * (Math.PI / 180);
            const x1 = center + Math.cos(a) * (ringRadius + 6);
            const y1 = center + Math.sin(a) * (ringRadius + 6);
            const x2 = center + Math.cos(a) * (ringRadius + 10);
            const y2 = center + Math.sin(a) * (ringRadius + 10);
            return <line key={i} x1={x1} y1={y1} x2={x2} y2={y2} stroke="#ffffff33" strokeWidth={2} strokeLinecap="round" />;
          })}
        </svg>
      </div>

      <div className="text-gray-400 text-[10px] font-mono text-center min-w-[50px]">
        {normalizedToDisplay(paramIdx, value)}
      </div>
    </div>
  );
}
