/**
 * BASE TEMPLATE: Visualization Component
 *
 * This is a template for creating new visualization components.
 * Use this as a reference when AI needs to create visual displays.
 *
 * Key patterns demonstrated:
 * - Canvas-based rendering for performance
 * - Real-time animation with requestAnimationFrame
 * - Parameter monitoring and visualization
 * - Responsive sizing
 * - Berlin Brutalism styling
 * - Cleanup and memory management
 */

import React from 'react';
import { useParameters } from '../system/ParameterContext';
import { EParams } from '../../config/constants';

interface BaseVisualProps {
  /** Parameter ID to visualize (optional) */
  paramId?: EParams;
  /** Width in pixels */
  width?: number;
  /** Height in pixels */
  height?: number;
  /** Display label */
  label?: string;
}

export function BaseVisual({
  paramId,
  width = 200,
  height = 100,
  label,
}: BaseVisualProps) {
  const { paramValues } = useParameters();
  const canvasRef = React.useRef<HTMLCanvasElement>(null);
  const animationFrameRef = React.useRef<number>();

  // Get parameter value if specified (normalized 0-1)
  const value = paramId !== undefined ? (paramValues.get(paramId) ?? 0.5) : 0.5;

  // Animation loop
  React.useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;

    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    // Set canvas resolution for crisp rendering
    const dpr = window.devicePixelRatio || 1;
    canvas.width = width * dpr;
    canvas.height = height * dpr;
    canvas.style.width = `${width}px`;
    canvas.style.height = `${height}px`;
    ctx.scale(dpr, dpr);

    // Animation state
    let time = 0;

    const render = () => {
      // Clear canvas
      ctx.fillStyle = '#000000';
      ctx.fillRect(0, 0, width, height);

      // Example visualization: Simple waveform
      ctx.strokeStyle = '#ffffff';
      ctx.lineWidth = 2;
      ctx.beginPath();

      const points = 50;
      for (let i = 0; i < points; i++) {
        const x = (i / (points - 1)) * width;

        // Create wave based on parameter value and time
        const frequency = value * 5 + 1;
        const amplitude = value * (height * 0.3);
        const y =
          height / 2 +
          Math.sin((i / points) * Math.PI * 2 * frequency + time * 0.1) * amplitude;

        if (i === 0) {
          ctx.moveTo(x, y);
        } else {
          ctx.lineTo(x, y);
        }
      }

      ctx.stroke();

      // Draw value indicator
      ctx.fillStyle = '#ffffff';
      ctx.font = '10px monospace';
      ctx.textAlign = 'right';
      ctx.textBaseline = 'top';
      ctx.fillText(value.toFixed(2), width - 5, 5);

      // Draw center line
      ctx.strokeStyle = 'rgba(255, 255, 255, 0.2)';
      ctx.lineWidth = 1;
      ctx.beginPath();
      ctx.moveTo(0, height / 2);
      ctx.lineTo(width, height / 2);
      ctx.stroke();

      time++;
      animationFrameRef.current = requestAnimationFrame(render);
    };

    // Start animation
    animationFrameRef.current = requestAnimationFrame(render);

    // Cleanup
    return () => {
      if (animationFrameRef.current) {
        cancelAnimationFrame(animationFrameRef.current);
      }
    };
  }, [value, width, height]);

  return (
    <div className="flex flex-col items-center gap-2">
      {/* Label */}
      {label && (
        <label className="text-white text-[10px] font-bold uppercase tracking-widest font-mono">
          {label}
        </label>
      )}

      {/* Canvas */}
      <div className="border-2 border-white bg-black">
        <canvas
          ref={canvasRef}
          className="block"
          style={{ width, height }}
        />
      </div>

      {/* Parameter info */}
      {label && (
        <span className="text-white/60 text-[9px] uppercase tracking-wider font-mono">
          Param {paramId}
        </span>
      )}
    </div>
  );
}
