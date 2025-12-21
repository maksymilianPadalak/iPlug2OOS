/**
 * ADSR Display Component
 *
 * Visualizes an ADSR envelope curve based on parameter values.
 * Synthwave / Outrun style with perspective grid floor and neon gradients.
 * Takes paramIds and uses useParameter internally.
 */

import { useMemo } from 'react';
import { useParameter } from '../glue/hooks/useParameter';
import type { ADSRDisplayProps } from './componentProps';

export type { ADSRDisplayProps };

export function ADSRDisplay({
  attackParam,
  decayParam,
  sustainParam,
  releaseParam,
  label,
  height: propHeight = 120,
}: ADSRDisplayProps) {
  const { value: attack } = useParameter(attackParam);
  const { value: decay } = useParameter(decayParam);
  const { value: sustain } = useParameter(sustainParam);
  const { value: release } = useParameter(releaseParam);

  // SVG dimensions
  const width = 200;
  const height = propHeight;
  const padding = { top: 14, right: 12, bottom: 14, left: 12 };
  const graphWidth = width - padding.left - padding.right;
  const graphHeight = height - padding.top - padding.bottom;

  // Calculate key points for envelope
  const points = useMemo(() => {
    const attackWidth = graphWidth * 0.25 * attack;
    const decayWidth = graphWidth * 0.25 * decay;
    const sustainWidth = graphWidth * 0.25;
    const releaseWidth = graphWidth * 0.25 * release;

    const minWidth = 4;
    const aW = Math.max(minWidth, attackWidth);
    const dW = Math.max(minWidth, decayWidth);
    const sW = sustainWidth;
    const rW = Math.max(minWidth, releaseWidth);

    const x0 = padding.left;
    const x1 = x0 + aW;
    const x2 = x1 + dW;
    const x3 = x2 + sW;
    const x4 = x3 + rW;

    const yBottom = padding.top + graphHeight;
    const yTop = padding.top;
    const ySustain = padding.top + graphHeight * (1 - sustain);

    return { x0, x1, x2, x3, x4, yBottom, yTop, ySustain, aW, dW, sW, rW };
  }, [attack, decay, sustain, release, graphWidth, graphHeight, padding]);

  // Calculate envelope path - smooth wave (sine-like curves)
  const path = useMemo(() => {
    const { x0, x1, x2, x3, x4, yBottom, yTop, ySustain, aW, dW, rW } = points;

    // Gentle sine-wave style curves
    // Attack: ease out (slow at top)
    // Decay: ease in-out
    // Release: ease in (slow at start)

    return `
      M ${x0} ${yBottom}
      C ${x0 + aW * 0.5} ${yBottom}, ${x1 - aW * 0.5} ${yTop}, ${x1} ${yTop}
      C ${x1 + dW * 0.5} ${yTop}, ${x2 - dW * 0.5} ${ySustain}, ${x2} ${ySustain}
      L ${x3} ${ySustain}
      C ${x3 + rW * 0.5} ${ySustain}, ${x4 - rW * 0.5} ${yBottom}, ${x4} ${yBottom}
    `;
  }, [points]);

  // Create fill path (closed shape for gradient fill)
  const fillPath = useMemo(() => {
    return `${path} L ${padding.left} ${padding.top + graphHeight} Z`;
  }, [path, padding, graphHeight]);

  // Unique IDs for gradients
  const ids = useMemo(() => {
    const base = Math.random().toString(36).slice(2, 9);
    return {
      strokeGradient: `adsr-stroke-${base}`,
      glowFilter: `adsr-glow-${base}`,
    };
  }, []);

  // Generate perspective grid lines (separated for animation)
  const { horizontalLines, verticalLines } = useMemo(() => {
    const horizontal: { x1: number; y1: number; x2: number; y2: number; opacity: number }[] = [];
    const vertical: { x1: number; y1: number; x2: number; y2: number; opacity: number }[] = [];

    const horizonY = padding.top + graphHeight * 0.25; // Vanishing point at 25% down from top
    const floorBottom = height - 2; // Stop just before the bottom edge
    const vanishX = width / 2;

    // Horizontal lines with perspective (skip first line at horizon)
    const hLineCount = 8;
    for (let i = 1; i <= hLineCount; i++) {
      const t = i / hLineCount;
      const y = horizonY + (floorBottom - horizonY) * Math.pow(t, 0.55);
      // Wider spread - starts at 15% at horizon, 100% at front
      const spread = 0.15 + t * 0.85;
      const x1 = vanishX - (vanishX + 10) * spread; // Extend past edges
      const x2 = vanishX + (width - vanishX + 10) * spread;
      horizontal.push({ x1, y1: y, x2, y2: y, opacity: 0.12 });
    }

    // Vertical lines converging to vanishing point
    const vLineCount = 15;
    for (let i = 0; i <= vLineCount; i++) {
      const t = i / vLineCount;
      const xBottom = -10 + (width + 20) * t; // Extend past edges
      vertical.push({
        x1: vanishX,
        y1: horizonY,
        x2: xBottom,
        y2: floorBottom,
        opacity: 0.15,
      });
    }

    return { horizontalLines: horizontal, verticalLines: vertical };
  }, [width, height, padding, graphWidth, graphHeight]);

  return (
    <div className="flex flex-col gap-2 w-full col-span-full">
      {/* Inline keyframe animation - starts dim, pulses brighter */}
      <style>{`
        @keyframes grid-line-pulse {
          0%, 100% { opacity: 0.12; }
          50% { opacity: 0.4; }
        }
      `}</style>

      {label && (
        <label
          className="text-[11px] font-bold uppercase tracking-[0.2em] text-center"
          style={{
            color: '#ff6ec7',
            textShadow: '0 0 10px rgba(255, 110, 199, 0.7), 0 0 20px rgba(255, 110, 199, 0.4)',
          }}
        >
          {label}
        </label>
      )}

      {/* Synthwave container */}
      <div
        className="relative w-full rounded-lg overflow-hidden"
        style={{
          background: 'linear-gradient(180deg, #0a0a1a 0%, #1a0a2e 50%, #2d1b4e 100%)',
          border: '1px solid rgba(255, 110, 199, 0.3)',
        }}
      >
        {/* Main SVG */}
        <svg
          viewBox={`0 0 ${width} ${height}`}
          className="relative w-full"
          style={{ height: `${height}px` }}
          preserveAspectRatio="none"
          shapeRendering="geometricPrecision"
        >
          <defs>
            {/* Animated stroke gradient - pink glow */}
            <linearGradient id={ids.strokeGradient} x1="0%" y1="0%" x2="200%" y2="0%">
              <stop offset="0%" stopColor="#cc4499" />
              <stop offset="25%" stopColor="#ff6ec7" />
              <stop offset="50%" stopColor="#cc4499" />
              <stop offset="75%" stopColor="#ff6ec7" />
              <stop offset="100%" stopColor="#cc4499" />
              <animate
                attributeName="x1"
                values="0%;-100%"
                dur="3s"
                repeatCount="indefinite"
              />
              <animate
                attributeName="x2"
                values="200%;100%"
                dur="3s"
                repeatCount="indefinite"
              />
            </linearGradient>

            {/* Glow filter */}
            <filter id={ids.glowFilter} x="-20%" y="-20%" width="140%" height="140%">
              <feGaussianBlur stdDeviation="2" result="glow" />
              <feMerge>
                <feMergeNode in="glow" />
                <feMergeNode in="SourceGraphic" />
              </feMerge>
            </filter>
          </defs>

          {/* Vertical grid lines (static) */}
          <g>
            {verticalLines.map((line, i) => (
              <line
                key={`v-${i}`}
                x1={line.x1}
                y1={line.y1}
                x2={line.x2}
                y2={line.y2}
                stroke="#ff6ec7"
                strokeWidth="0.5"
                opacity={line.opacity}
              />
            ))}
          </g>

          {/* Horizontal grid lines with staggered CSS animation */}
          <g>
            {horizontalLines.map((line, i) => (
              <line
                key={`h-${i}`}
                x1={line.x1}
                y1={line.y1}
                x2={line.x2}
                y2={line.y2}
                stroke="#ff6ec7"
                strokeWidth="0.5"
                opacity={0.12}
                style={{
                  animation: `grid-line-pulse 1.2s ease-in-out infinite`,
                  animationDelay: `${i * 0.12}s`,
                }}
              />
            ))}
          </g>

          {/* Main curve - animated pink glow */}
          <path
            d={path}
            fill="none"
            stroke={`url(#${ids.strokeGradient})`}
            strokeWidth="2"
            strokeLinecap="round"
            strokeLinejoin="round"
            filter={`url(#${ids.glowFilter})`}
          />

        </svg>

        {/* Corner accents - matching border pink */}
        <div
          className="absolute top-1 left-1 w-3 h-3 border-l border-t"
          style={{ borderColor: 'rgba(255, 110, 199, 0.5)' }}
        />
        <div
          className="absolute top-1 right-1 w-3 h-3 border-r border-t"
          style={{ borderColor: 'rgba(255, 110, 199, 0.5)' }}
        />
        <div
          className="absolute bottom-1 left-1 w-3 h-3 border-l border-b"
          style={{ borderColor: 'rgba(255, 110, 199, 0.5)' }}
        />
        <div
          className="absolute bottom-1 right-1 w-3 h-3 border-r border-b"
          style={{ borderColor: 'rgba(255, 110, 199, 0.5)' }}
        />
      </div>
    </div>
  );
}
