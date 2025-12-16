/**
 * WaveformDisplay - Oscilloscope-style waveform visualization
 *
 * Displays real-time waveform data from DSP via control tags.
 * Auto-fades when no data is received.
 */

import { useRef, useEffect, useState } from 'react';
import { useWaveform } from '@/glue/hooks/useWaveform';
import type { WaveformDisplayProps } from '@/components/uiManifest/componentProps';

const FADE_TIMEOUT_MS = 300;

export function WaveformDisplay({ ctrlTag, label }: WaveformDisplayProps) {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const containerRef = useRef<HTMLDivElement>(null);
  const [canvasWidth, setCanvasWidth] = useState(200);

  const { samples, timestamp } = useWaveform(ctrlTag);

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

  // Draw waveform
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

    // Check if data is stale (fade out)
    const isStale = Date.now() - timestamp > FADE_TIMEOUT_MS;
    const alpha = isStale ? 0.2 : 1;

    // Draw subtle grid - brass color
    ctx.strokeStyle = `rgba(184, 134, 11, ${0.15 * alpha})`;
    ctx.lineWidth = 1;
    ctx.beginPath();
    ctx.moveTo(0, centerY);
    ctx.lineTo(width, centerY);
    ctx.stroke();

    // Draw waveform if we have data
    if (samples.length > 0) {
      // Glow layers - warm brass tones
      const glowLayers = [
        { blur: 10, alpha: 0.25 * alpha },
        { blur: 5, alpha: 0.4 * alpha },
        { blur: 2, alpha: 0.7 * alpha },
      ];

      for (const { blur, alpha: layerAlpha } of glowLayers) {
        ctx.save();
        ctx.shadowColor = 'rgba(184, 134, 11, 1)';
        ctx.shadowBlur = blur;
        ctx.strokeStyle = `rgba(184, 134, 11, ${layerAlpha})`;
        ctx.lineWidth = 2;
        ctx.beginPath();

        for (let i = 0; i < samples.length; i++) {
          const x = (i / samples.length) * width;
          const y = centerY - samples[i] * centerY * 0.8;
          if (i === 0) ctx.moveTo(x, y);
          else ctx.lineTo(x, y);
        }

        ctx.stroke();
        ctx.restore();
      }

      // Main stroke - darker brass for contrast
      ctx.strokeStyle = `rgba(139, 105, 20, ${alpha})`;
      ctx.lineWidth = 1.5;
      ctx.beginPath();

      for (let i = 0; i < samples.length; i++) {
        const x = (i / samples.length) * width;
        const y = centerY - samples[i] * centerY * 0.8;
        if (i === 0) ctx.moveTo(x, y);
        else ctx.lineTo(x, y);
      }

      ctx.stroke();
    }

    // Soft vignette for depth
    const vignette = ctx.createRadialGradient(
      width / 2, height / 2, 0,
      width / 2, height / 2, width / 1.5
    );
    vignette.addColorStop(0, 'rgba(0,0,0,0)');
    vignette.addColorStop(1, 'rgba(0,0,0,0.15)');
    ctx.fillStyle = vignette;
    ctx.fillRect(0, 0, width, height);
  }, [samples, timestamp, canvasWidth]);

  return (
    <fieldset className="w-full col-span-full rounded-xl border border-[#B8860B]/30 px-3 pb-3 pt-1 bg-gradient-to-br from-white/50 to-white/20">
      <legend className="px-2 text-[10px] font-bold uppercase tracking-wider text-[#1a1a1a]">
        <span
          className="mr-1.5 inline-block h-1.5 w-1.5 rounded-full"
          style={{
            backgroundColor:
              Date.now() - timestamp < FADE_TIMEOUT_MS
                ? 'rgb(184, 134, 11)'
                : 'rgb(180, 170, 155)',
          }}
        />
        {label}
      </legend>
      <div
        ref={containerRef}
        className="relative overflow-hidden rounded-lg border border-[#B8860B]/20 bg-gradient-to-b from-[#1a1816] via-[#0f0e0d] to-[#0a0908]"
        style={{
          boxShadow: 'inset 0 2px 4px rgba(0,0,0,0.3)',
        }}
      >
        <canvas
          ref={canvasRef}
          width={canvasWidth}
          height={80}
          className="block w-full"
        />
      </div>
    </fieldset>
  );
}
