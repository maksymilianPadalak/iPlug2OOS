/**
 * WaveformDisplay - Oscilloscope-style waveform visualization
 *
 * Displays real-time waveform data from DSP via control tags.
 * Auto-fades when no data is received.
 */

import { useRef, useEffect, useState } from 'react';
import { useWaveform } from '@/glue/hooks/useWaveform';
import { EControlTags } from '@/config/runtimeParameters';

type WaveformDisplayProps = {
  ctrlTag: keyof typeof EControlTags;
  label?: string;
};

const FADE_TIMEOUT_MS = 300;

export function WaveformDisplay({ ctrlTag, label }: WaveformDisplayProps) {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const containerRef = useRef<HTMLDivElement>(null);
  const [canvasWidth, setCanvasWidth] = useState(200);

  // Get numeric control tag from string key
  const ctrlTagNum = EControlTags[ctrlTag];
  const { samples, timestamp } = useWaveform(ctrlTagNum);

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

    // Draw subtle grid
    ctx.strokeStyle = `rgba(251, 146, 60, ${0.1 * alpha})`;
    ctx.lineWidth = 1;
    ctx.beginPath();
    ctx.moveTo(0, centerY);
    ctx.lineTo(width, centerY);
    ctx.stroke();

    // Draw waveform if we have data
    if (samples.length > 0) {
      // Glow layers
      const glowLayers = [
        { blur: 12, alpha: 0.3 * alpha },
        { blur: 6, alpha: 0.5 * alpha },
        { blur: 2, alpha: 0.8 * alpha },
      ];

      for (const { blur, alpha: layerAlpha } of glowLayers) {
        ctx.save();
        ctx.shadowColor = 'rgba(251, 146, 60, 1)';
        ctx.shadowBlur = blur;
        ctx.strokeStyle = `rgba(251, 146, 60, ${layerAlpha})`;
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

      // Main stroke
      ctx.strokeStyle = `rgba(255, 200, 150, ${alpha})`;
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

    // Vignette overlay
    const vignette = ctx.createRadialGradient(
      width / 2, height / 2, 0,
      width / 2, height / 2, width / 1.5
    );
    vignette.addColorStop(0, 'rgba(0,0,0,0)');
    vignette.addColorStop(1, 'rgba(0,0,0,0.4)');
    ctx.fillStyle = vignette;
    ctx.fillRect(0, 0, width, height);
  }, [samples, timestamp, canvasWidth]);

  return (
    <fieldset className="rounded-lg border border-orange-500/30 px-3 pb-3 pt-1">
      <legend className="px-2 text-[10px] font-semibold uppercase tracking-wider text-orange-300/70">
        <span
          className="mr-1.5 inline-block h-1.5 w-1.5 rounded-full"
          style={{
            backgroundColor:
              Date.now() - timestamp < FADE_TIMEOUT_MS
                ? 'rgb(251, 146, 60)'
                : 'rgb(120, 113, 108)',
          }}
        />
        {label}
      </legend>
      <div
        ref={containerRef}
        className="relative overflow-hidden rounded-md border border-orange-900/50 bg-gradient-to-b from-stone-950 via-neutral-950 to-black"
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
