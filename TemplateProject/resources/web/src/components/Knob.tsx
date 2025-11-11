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
    const colors = ['#fb923c', '#f97316', '#ea580c', '#dc2626', '#b91c1c']; // Warm orange-to-red gradient

    return (
      <div className="flex flex-col items-center gap-3">
        <label className="text-orange-200 text-xs font-bold uppercase tracking-widest">
          {label}
        </label>
        <div
          ref={knobRef}
          onMouseDown={handleMouseDown}
          onTouchStart={handleTouchStart}
          className={`relative rounded-full bg-gradient-to-br from-stone-900 to-black border-2 border-orange-600/50 cursor-pointer select-none knob-outer shadow-xl`
          }
          style={{ width: `${size / 1.2}px`, height: `${size / 1.2}px`, display: 'flex', alignItems: 'center', justifyContent: 'center', boxShadow: isDragging ? 'inset 0 8px 20px rgba(0,0,0,0.8), 0 0 20px rgba(251,146,60,0.3)' : '0 4px 16px rgba(0,0,0,0.6)', animation: isDragging ? 'knob-pulse 0.9s infinite' : undefined }}
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
                  opacity={0.92 - i * 0.08}
                />
              );
            })}

            {/* Center plate */}
            <defs>
              <radialGradient id="kg" cx="50%" cy="40%" r="70%">
                <stop offset="0%" stopColor="#292524" />
                <stop offset="100%" stopColor="#0c0a09" />
              </radialGradient>
            </defs>

            <circle cx={center} cy={center} r={28} fill="url(#kg)" stroke="#fb923c" strokeWidth={3} />
          </svg>
        </div>
        <div className="text-orange-300 text-xs font-bold text-center min-w-[60px]">
          {normalizedToDisplay(paramIdx, value)}
        </div>
      </div>
    );
  }

  // Default small knob with modern gradient styling
  const smallSize = 52;
  const center = smallSize / 2;
  const ringRadius = 18;
  const circumference = 2 * Math.PI * ringRadius;
  const progress = normalizedValue;
  const dash = circumference * progress;

  return (
    <div className="flex flex-col items-center gap-1">
      <label className="text-orange-200 text-[10px] font-bold uppercase tracking-wider">
        {label}
      </label>

      <div
        ref={knobRef}
        onMouseDown={handleMouseDown}
        onTouchStart={handleTouchStart}
        className="relative bg-gradient-to-br from-stone-900 to-black border-2 border-orange-600/50 cursor-pointer select-none transition-all"
        style={{ width: `${smallSize}px`, height: `${smallSize}px`, display: 'flex', alignItems: 'center', justifyContent: 'center', boxShadow: isDragging ? 'inset 0 6px 14px rgba(0,0,0,0.8), 0 0 12px rgba(251,146,60,0.3)' : '0 3px 10px rgba(0,0,0,0.5)', borderRadius: 999 }}
      >
        <svg width={smallSize} height={smallSize} viewBox={`0 0 ${smallSize} ${smallSize}`}>
          {/* Outer progress ring */}
          <circle
            cx={center}
            cy={center}
            r={ringRadius}
            fill="none"
            stroke="#ffffff15"
            strokeWidth={5}
          />

          <circle
            cx={center}
            cy={center}
            r={ringRadius}
            fill="none"
            stroke="#fb923c"
            strokeWidth={5}
            strokeLinecap="round"
            strokeDasharray={`${dash} ${circumference - dash}`}
            transform={`rotate(-90 ${center} ${center})`}
            opacity={0.9}
          />

          {/* Center disc with subtle sheen */}
          <defs>
            <radialGradient id={`g-${paramIdx}`} cx="50%" cy="40%" r="70%">
              <stop offset="0%" stopColor="#292524" />
              <stop offset="100%" stopColor="#0c0a09" />
            </radialGradient>
          </defs>
          <circle cx={center} cy={center} r={13} fill={`url(#g-${paramIdx})`} stroke="#fb923c" strokeWidth={2} />
        </svg>
      </div>

      <div className="text-orange-300 text-[10px] font-bold text-center min-w-[50px]">
        {normalizedToDisplay(paramIdx, value)}
      </div>
    </div>
  );
}
