/**
 * WaveformDisplay - Oscilloscope-style waveform visualization
 *
 * Performance-optimized: minimal shadowBlur, no allocations per frame.
 */

import { useRef, useEffect, useState, useCallback } from 'react';
import { useWaveform } from '../glue/hooks/useWaveform';
import type { WaveformDisplayProps } from './componentProps';

export type { WaveformDisplayProps };

const FADE_TIMEOUT_MS = 300;

export function WaveformDisplay({ ctrlTag, label }: WaveformDisplayProps) {
  const { samples, timestamp } = useWaveform(ctrlTag);
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const containerRef = useRef<HTMLDivElement>(null);
  const [canvasWidth, setCanvasWidth] = useState(200);

  // Track container width
  useEffect(() => {
    const container = containerRef.current;
    if (!container) return;

    const observer = new ResizeObserver((entries) => {
      const entry = entries[0];
      if (entry) setCanvasWidth(entry.contentRect.width);
    });

    observer.observe(container);
    return () => observer.disconnect();
  }, []);

  // Draw waveform path helper
  const drawWaveformPath = useCallback(
    (
      ctx: CanvasRenderingContext2D,
      data: Float32Array,
      width: number,
      centerY: number
    ) => {
      ctx.beginPath();
      for (let i = 0; i < data.length; i++) {
        const x = (i / data.length) * width;
        const y = centerY - data[i] * centerY * 0.8;
        if (i === 0) ctx.moveTo(x, y);
        else ctx.lineTo(x, y);
      }
    },
    []
  );

  // Render on data change
  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;

    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    const width = canvas.width;
    const height = canvas.height;
    const centerY = height / 2;

    // Clear
    ctx.clearRect(0, 0, width, height);

    // Check if data is stale
    const isStale = Date.now() - timestamp > FADE_TIMEOUT_MS;
    const baseAlpha = isStale ? 0.3 : 1;

    // Draw grid (no shadows, very fast)
    ctx.strokeStyle = `rgba(0, 212, 255, ${0.1 * baseAlpha})`;
    ctx.lineWidth = 1;

    // Center line
    ctx.beginPath();
    ctx.moveTo(0, centerY);
    ctx.lineTo(width, centerY);
    ctx.stroke();

    // 25% and 75% lines
    ctx.strokeStyle = `rgba(0, 212, 255, ${0.05 * baseAlpha})`;
    ctx.beginPath();
    ctx.moveTo(0, height * 0.25);
    ctx.lineTo(width, height * 0.25);
    ctx.moveTo(0, height * 0.75);
    ctx.lineTo(width, height * 0.75);
    ctx.stroke();

    // Vertical grid
    for (let i = 1; i < 8; i++) {
      const x = (i / 8) * width;
      ctx.beginPath();
      ctx.moveTo(x, 0);
      ctx.lineTo(x, height);
      ctx.stroke();
    }

    // Draw waveform if we have data
    if (samples.length > 0) {
      // Single glow layer (ONE shadowBlur operation)
      ctx.save();
      ctx.shadowColor = 'rgba(0, 255, 255, 0.8)';
      ctx.shadowBlur = 12;
      ctx.strokeStyle = `rgba(0, 220, 255, ${0.6 * baseAlpha})`;
      ctx.lineWidth = 3;
      drawWaveformPath(ctx, samples, width, centerY);
      ctx.stroke();
      ctx.restore();

      // Bright core line (no shadow)
      ctx.strokeStyle = `rgba(180, 255, 255, ${baseAlpha})`;
      ctx.lineWidth = 1.5;
      drawWaveformPath(ctx, samples, width, centerY);
      ctx.stroke();
    }
  }, [samples, timestamp, canvasWidth, drawWaveformPath]);

  // Check if data is stale for indicator
  const isActive = Date.now() - timestamp < FADE_TIMEOUT_MS;

  return (
    <fieldset
      className="w-full col-span-full rounded-xl border border-cyan-500/40 px-3 pb-3 pt-1 bg-gradient-to-br from-[#0a1520] via-[#0d1a25] to-[#081018]"
      style={{
        boxShadow: '0 0 20px rgba(0, 212, 255, 0.15), inset 0 1px 0 rgba(0, 212, 255, 0.1)',
      }}
    >
      <legend className="px-2 text-[10px] font-bold uppercase tracking-wider text-cyan-400/90">
        <span
          className="mr-1.5 inline-block h-1.5 w-1.5 rounded-full"
          style={{
            backgroundColor: isActive ? 'rgb(0, 212, 255)' : 'rgb(60, 80, 90)',
            boxShadow: isActive ? '0 0 6px rgba(0, 212, 255, 0.8)' : 'none',
          }}
        />
        {label}
      </legend>
      <div
        ref={containerRef}
        className="relative overflow-hidden rounded-lg border border-cyan-500/30"
        style={{
          background: 'linear-gradient(180deg, #0a1218 0%, #060d12 50%, #040810 100%)',
          boxShadow: 'inset 0 2px 8px rgba(0,0,0,0.5), inset 0 0 20px rgba(0, 212, 255, 0.05), 0 0 15px rgba(0, 212, 255, 0.1)',
        }}
      >
        <canvas ref={canvasRef} width={canvasWidth} height={80} className="block w-full" />
      </div>
    </fieldset>
  );
}
