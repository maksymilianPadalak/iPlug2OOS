/**
 * XYPad Component
 *
 * Interactive 2D parameter controller with animated waveform visualization.
 * Cyberpunk style with smooth dragging and GPU-accelerated rendering.
 */

import React, { useRef, useCallback, useEffect, useState } from 'react';
import { useParameter } from '../glue/hooks/useParameter';
import type { XYPadProps } from './componentProps';

export type { XYPadProps };

// Cursor color
const cursorColor = { color: '#00ffff', glow: 'rgba(0, 255, 255, 0.8)' };

export function XYPad({
  paramIdX,
  paramIdY,
  labelX = 'X',
  labelY = 'Y',
  size = 288,
}: XYPadProps) {
  const xParam = useParameter(paramIdX);
  const yParam = useParameter(paramIdY);
  const xyRef = useRef<HTMLDivElement>(null);
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const isDragging = useRef(false);

  // Local state for smooth dragging - bypasses React re-render latency
  const localPosRef = useRef({ x: 0, y: 0 });
  const [, forceUpdate] = React.useReducer(x => x + 1, 0);

  // Track dragging state for cursor size change
  const [isDraggingState, setIsDraggingState] = useState(false);

  // Store param values in refs so animation doesn't restart
  const xValueRef = useRef(xParam.value);
  const yValueRef = useRef(yParam.value);
  xValueRef.current = xParam.value;
  yValueRef.current = yParam.value;

  // Animated waveform background - runs once, reads values from refs
  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    let animationId: number;
    let phase = 0;

    const draw = () => {
      const w = canvas.width;
      const h = canvas.height;
      ctx.clearRect(0, 0, w, h);

      const xVal = xValueRef.current;
      const yVal = yValueRef.current;

      // Draw horizontal waveforms (affected by Y param)
      const maxHWaves = 8;
      const rawHWaveCount = yVal * maxHWaves;
      const hAmplitude = 5 + yVal * 20;

      for (let i = 0; i < maxHWaves; i++) {
        const waveOpacity = Math.max(0, Math.min(1, rawHWaveCount - i));
        if (waveOpacity <= 0) continue;

        const yPos = h - (h / (maxHWaves + 1)) * (i + 1);

        ctx.beginPath();
        ctx.strokeStyle = `rgba(255, 0, 255, ${0.28 * waveOpacity})`;
        ctx.lineWidth = 1.5;
        ctx.lineCap = 'round';
        ctx.lineJoin = 'round';

        for (let x = 0; x < w; x++) {
          const y = yPos + Math.sin(x * 0.03 + phase + i * 0.5) * hAmplitude;
          if (x === 0) ctx.moveTo(x, y);
          else ctx.lineTo(x, y);
        }
        ctx.stroke();
      }

      // Draw vertical waveforms (affected by X param)
      const maxVWaves = 8;
      const rawVWaveCount = xVal * maxVWaves;
      const vAmplitude = 5 + xVal * 20;

      for (let i = 0; i < maxVWaves; i++) {
        const waveOpacity = Math.max(0, Math.min(1, rawVWaveCount - i));
        if (waveOpacity <= 0) continue;

        const xPos = (w / (maxVWaves + 1)) * (i + 1);

        ctx.beginPath();
        ctx.strokeStyle = `rgba(0, 255, 255, ${0.28 * waveOpacity})`;
        ctx.lineWidth = 1.5;
        ctx.lineCap = 'round';
        ctx.lineJoin = 'round';

        for (let y = 0; y < h; y++) {
          const x = xPos + Math.sin(y * 0.03 + phase + i * 0.5) * vAmplitude;
          if (y === 0) ctx.moveTo(x, y);
          else ctx.lineTo(x, y);
        }
        ctx.stroke();
      }

      phase += 0.02;
      animationId = requestAnimationFrame(draw);
    };

    draw();
    return () => cancelAnimationFrame(animationId);
  }, []);

  const handleMouseDown = useCallback((e: React.MouseEvent) => {
    if (!xyRef.current) return;
    isDragging.current = true;
    setIsDraggingState(true);
    xParam.beginChange();
    yParam.beginChange();

    let rafId: number | null = null;
    let pendingX = 0;
    let pendingY = 0;

    const updatePosition = (clientX: number, clientY: number) => {
      if (!xyRef.current) return;
      const rect = xyRef.current.getBoundingClientRect();
      pendingX = Math.max(0, Math.min(1, (clientX - rect.left) / rect.width));
      pendingY = Math.max(0, Math.min(1, 1 - (clientY - rect.top) / rect.height));

      localPosRef.current = { x: pendingX, y: pendingY };
      forceUpdate();

      if (rafId === null) {
        rafId = requestAnimationFrame(() => {
          xParam.setValue(pendingX);
          yParam.setValue(pendingY);
          rafId = null;
        });
      }
    };

    updatePosition(e.clientX, e.clientY);

    const handleMouseMove = (e: MouseEvent) => {
      if (isDragging.current) {
        updatePosition(e.clientX, e.clientY);
      }
    };

    const handleMouseUp = () => {
      isDragging.current = false;
      setIsDraggingState(false);
      if (rafId !== null) {
        cancelAnimationFrame(rafId);
      }
      xParam.setValue(pendingX);
      yParam.setValue(pendingY);
      xParam.endChange();
      yParam.endChange();
      window.removeEventListener('mousemove', handleMouseMove);
      window.removeEventListener('mouseup', handleMouseUp);
    };

    window.addEventListener('mousemove', handleMouseMove);
    window.addEventListener('mouseup', handleMouseUp);
  }, [xParam, yParam]);

  const displayX = isDragging.current ? localPosRef.current.x : xParam.value;
  const displayY = isDragging.current ? localPosRef.current.y : yParam.value;

  return (
    <div
      ref={xyRef}
      className="relative rounded-lg cursor-crosshair overflow-hidden"
      style={{
        width: `${size}px`,
        height: `${size}px`,
        backgroundColor: 'rgba(0,0,0,0.7)',
        border: '1px solid rgba(0,255,255,0.4)',
        boxShadow: 'inset 0 0 40px rgba(0,255,255,0.08), 0 0 20px rgba(0,255,255,0.15)',
      }}
      onMouseDown={handleMouseDown}
    >
      {/* Animated waveform canvas */}
      <canvas
        ref={canvasRef}
        width={size}
        height={size}
        className="absolute inset-0 pointer-events-none"
      />

      {/* Target reticle - concentric circles */}
      <div className="absolute inset-0 pointer-events-none flex items-center justify-center">
        <div className="absolute w-[90%] h-[90%] rounded-full border border-cyan-500/10" />
        <div className="absolute w-[70%] h-[70%] rounded-full border border-cyan-500/15" />
        <div className="absolute w-[50%] h-[50%] rounded-full border border-cyan-500/20" />
        <div className="absolute w-[30%] h-[30%] rounded-full border" style={{ borderColor: 'rgba(255,0,255,0.2)' }} />
      </div>

      {/* Edge tick marks */}
      <div className="absolute inset-0 pointer-events-none">
        {[0.25, 0.5, 0.75].map((pos) => (
          <div key={`t-${pos}`} className="absolute top-0 w-[1px] h-2" style={{ left: `${pos * 100}%`, backgroundColor: 'rgba(0,255,255,0.4)' }} />
        ))}
        {[0.25, 0.5, 0.75].map((pos) => (
          <div key={`b-${pos}`} className="absolute bottom-0 w-[1px] h-2" style={{ left: `${pos * 100}%`, backgroundColor: 'rgba(0,255,255,0.4)' }} />
        ))}
        {[0.25, 0.5, 0.75].map((pos) => (
          <div key={`l-${pos}`} className="absolute left-0 h-[1px] w-2" style={{ top: `${pos * 100}%`, backgroundColor: 'rgba(255,0,255,0.4)' }} />
        ))}
        {[0.25, 0.5, 0.75].map((pos) => (
          <div key={`r-${pos}`} className="absolute right-0 h-[1px] w-2" style={{ top: `${pos * 100}%`, backgroundColor: 'rgba(255,0,255,0.4)' }} />
        ))}
      </div>

      {/* Full crosshair lines through cursor position */}
      <div
        className="absolute top-0 bottom-0 w-[1px] left-0 pointer-events-none"
        style={{
          transform: `translateX(${displayX * size}px)`,
          willChange: 'transform',
          background: 'linear-gradient(to bottom, rgba(0,255,255,0.1), rgba(0,255,255,0.4), rgba(0,255,255,0.1))',
        }}
      />
      <div
        className="absolute left-0 right-0 h-[1px] top-0 pointer-events-none"
        style={{
          transform: `translateY(${(1 - displayY) * size}px)`,
          willChange: 'transform',
          background: 'linear-gradient(to right, rgba(255,0,255,0.1), rgba(255,0,255,0.4), rgba(255,0,255,0.1))',
        }}
      />

      {/* Axis labels */}
      <span
        className="absolute bottom-1 left-1/2 -translate-x-1/2 text-[9px] uppercase tracking-wider"
        style={{ color: 'rgba(0,255,255,0.6)', fontFamily: "'Orbitron', monospace" }}
      >
        {labelX}
      </span>
      <span
        className="absolute left-1 top-1/2 text-[9px] uppercase tracking-wider origin-center"
        style={{
          color: 'rgba(255,0,255,0.6)',
          fontFamily: "'Orbitron', monospace",
          writingMode: 'vertical-rl',
          transform: 'rotate(180deg) translateY(50%)',
        }}
      >
        {labelY}
      </span>

      {/* Cursor with reticle */}
      <div
        className="absolute left-0 top-0 pointer-events-none"
        style={{
          transform: `translate(${displayX * size - 25}px, ${(1 - displayY) * size - 25}px)`,
          willChange: 'transform',
          width: '50px',
          height: '50px',
        }}
      >
        {/* Outer glow */}
        <div
          className="absolute inset-0 rounded-full"
          style={{
            background: `radial-gradient(circle, ${cursorColor.glow.replace('0.8', '0.3')} 0%, transparent 70%)`,
            transform: isDraggingState ? 'scale(1.4)' : 'scale(1)',
            transition: 'transform 0.25s ease',
          }}
        />
        {/* Rotating reticle ring */}
        <div
          className="absolute rounded-full"
          style={{
            width: '24px',
            height: '24px',
            left: '13px',
            top: '13px',
            border: `1px solid ${cursorColor.glow.replace('0.8', '0.5')}`,
            animation: 'spin 4s linear infinite',
            transform: isDraggingState ? 'scale(1.4)' : 'scale(1)',
            transformOrigin: 'center',
            transition: 'transform 0.25s ease',
          }}
        />
        {/* Center hollow ring */}
        <div
          className="absolute w-4 h-4 rounded-full"
          style={{
            left: '17px',
            top: '17px',
            border: `2px solid ${cursorColor.color}`,
            boxShadow: `0 0 8px ${cursorColor.glow}, inset 0 0 4px ${cursorColor.glow.replace('0.8', '0.3')}`,
            transform: isDraggingState ? 'scale(1.4)' : 'scale(1)',
            transformOrigin: 'center',
            transition: 'transform 0.25s ease',
          }}
        />
      </div>
    </div>
  );
}
