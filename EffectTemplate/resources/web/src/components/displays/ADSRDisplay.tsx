/**
 * ADSR Display Component
 *
 * Visualizes an ADSR envelope curve based on parameter values.
 * Streamline Moderne style with smooth curves and gradient fills.
 */

import { useMemo } from 'react';
import { useParameter } from '@/glue/hooks/useParameter';
import type { ADSRDisplayProps } from '@/components/uiManifest/componentProps';

export function ADSRDisplay({
  attackParam,
  decayParam,
  sustainParam,
  releaseParam,
  label,
}: ADSRDisplayProps) {
  const { value: attack } = useParameter(attackParam);
  const { value: decay } = useParameter(decayParam);
  const { value: sustain } = useParameter(sustainParam);
  const { value: release } = useParameter(releaseParam);

  // SVG dimensions
  const width = 200;
  const height = 80;
  const padding = { top: 8, right: 8, bottom: 8, left: 8 };
  const graphWidth = width - padding.left - padding.right;
  const graphHeight = height - padding.top - padding.bottom;

  // Calculate envelope path
  const path = useMemo(() => {
    // Normalize time values (attack, decay, release are 0-1 normalized)
    // Allocate width: Attack 25%, Decay 25%, Sustain 25%, Release 25%
    const attackWidth = graphWidth * 0.25 * attack;
    const decayWidth = graphWidth * 0.25 * decay;
    const sustainWidth = graphWidth * 0.25;
    const releaseWidth = graphWidth * 0.25 * release;

    // Ensure minimum visibility
    const minWidth = 4;
    const aW = Math.max(minWidth, attackWidth);
    const dW = Math.max(minWidth, decayWidth);
    const sW = sustainWidth;
    const rW = Math.max(minWidth, releaseWidth);

    // Calculate x positions
    const x0 = padding.left;
    const x1 = x0 + aW; // End of attack (peak)
    const x2 = x1 + dW; // End of decay (sustain level)
    const x3 = x2 + sW; // End of sustain
    const x4 = x3 + rW; // End of release

    // Calculate y positions (inverted because SVG y goes down)
    const yBottom = padding.top + graphHeight;
    const yTop = padding.top;
    const ySustain = padding.top + graphHeight * (1 - sustain);

    // Create smooth bezier curve path
    // Attack: exponential rise
    const attackCtrl1X = x0 + aW * 0.4;
    const attackCtrl1Y = yBottom;
    const attackCtrl2X = x0 + aW * 0.6;
    const attackCtrl2Y = yTop;

    // Decay: exponential fall to sustain
    const decayCtrl1X = x1 + dW * 0.3;
    const decayCtrl1Y = yTop;
    const decayCtrl2X = x1 + dW * 0.7;
    const decayCtrl2Y = ySustain;

    // Release: exponential fall to zero
    const releaseCtrl1X = x3 + rW * 0.3;
    const releaseCtrl1Y = ySustain;
    const releaseCtrl2X = x3 + rW * 0.7;
    const releaseCtrl2Y = yBottom;

    return `
      M ${x0} ${yBottom}
      C ${attackCtrl1X} ${attackCtrl1Y}, ${attackCtrl2X} ${attackCtrl2Y}, ${x1} ${yTop}
      C ${decayCtrl1X} ${decayCtrl1Y}, ${decayCtrl2X} ${decayCtrl2Y}, ${x2} ${ySustain}
      L ${x3} ${ySustain}
      C ${releaseCtrl1X} ${releaseCtrl1Y}, ${releaseCtrl2X} ${releaseCtrl2Y}, ${x4} ${yBottom}
    `;
  }, [attack, decay, sustain, release, graphWidth, graphHeight, padding]);

  // Create fill path (closed shape for gradient fill)
  const fillPath = useMemo(() => {
    return `${path} L ${padding.left} ${padding.top + graphHeight} Z`;
  }, [path, padding, graphHeight]);

  // Unique ID for gradient
  const gradientId = useMemo(
    () => `adsr-gradient-${Math.random().toString(36).slice(2, 9)}`,
    []
  );

  return (
    <div className="flex flex-col gap-1.5 w-full col-span-full">
      {label && (
        <label className="text-[#1a1a1a] text-[11px] font-black uppercase tracking-[0.1em] text-center">
          {label}
        </label>
      )}

      <div
        className="relative w-full bg-gradient-to-br from-white/70 to-white/40 border border-[#B8860B]/25 rounded-xl overflow-hidden"
        style={{
          boxShadow: 'inset 0 1px 3px rgba(255,255,255,0.9), 0 4px 12px rgba(0,0,0,0.08)',
        }}
      >
        {/* Subtle grid lines */}
        <svg
          viewBox={`0 0 ${width} ${height}`}
          className="absolute inset-0 w-full h-full opacity-30"
          preserveAspectRatio="none"
          style={{ height: '80px' }}
        >
          {/* Horizontal grid lines */}
          {[0.25, 0.5, 0.75].map((y) => (
            <line
              key={`h-${y}`}
              x1={padding.left}
              y1={padding.top + graphHeight * y}
              x2={width - padding.right}
              y2={padding.top + graphHeight * y}
              stroke="#B8860B"
              strokeWidth="0.5"
              strokeDasharray="2,4"
            />
          ))}
          {/* Vertical grid lines for ADSR segments */}
          {[0.25, 0.5, 0.75].map((x) => (
            <line
              key={`v-${x}`}
              x1={padding.left + graphWidth * x}
              y1={padding.top}
              x2={padding.left + graphWidth * x}
              y2={height - padding.bottom}
              stroke="#B8860B"
              strokeWidth="0.5"
              strokeDasharray="2,4"
            />
          ))}
        </svg>

        {/* Main envelope SVG */}
        <svg viewBox={`0 0 ${width} ${height}`} className="w-full" style={{ height: '80px' }} preserveAspectRatio="none">
          <defs>
            {/* Gradient fill - warm brass tones */}
            <linearGradient id={gradientId} x1="0%" y1="0%" x2="0%" y2="100%">
              <stop offset="0%" stopColor="#B8860B" stopOpacity="0.35" />
              <stop offset="50%" stopColor="#DAA520" stopOpacity="0.15" />
              <stop offset="100%" stopColor="#F5E6D3" stopOpacity="0.05" />
            </linearGradient>
            {/* Subtle glow filter */}
            <filter id="adsrGlow" x="-50%" y="-50%" width="200%" height="200%">
              <feGaussianBlur stdDeviation="1.5" result="blur" />
              <feMerge>
                <feMergeNode in="blur" />
                <feMergeNode in="SourceGraphic" />
              </feMerge>
            </filter>
          </defs>

          {/* Filled area under curve */}
          <path d={fillPath} fill={`url(#${gradientId})`} />

          {/* Main envelope curve with glow */}
          <path
            d={path}
            fill="none"
            stroke="#B8860B"
            strokeWidth="2"
            strokeLinecap="round"
            strokeLinejoin="round"
            filter="url(#adsrGlow)"
          />

          {/* Crisp envelope curve on top */}
          <path
            d={path}
            fill="none"
            stroke="#8B6914"
            strokeWidth="1.5"
            strokeLinecap="round"
            strokeLinejoin="round"
          />

          {/* Corner accents - Streamline Moderne style */}
          <path
            d={`M ${padding.left} ${padding.top + 10} L ${padding.left} ${padding.top} L ${padding.left + 10} ${padding.top}`}
            fill="none"
            stroke="#B8860B"
            strokeWidth="1"
            strokeOpacity="0.4"
          />
          <path
            d={`M ${width - padding.right - 10} ${padding.top} L ${width - padding.right} ${padding.top} L ${width - padding.right} ${padding.top + 10}`}
            fill="none"
            stroke="#B8860B"
            strokeWidth="1"
            strokeOpacity="0.4"
          />
        </svg>

        {/* ADSR labels at bottom */}
        <div className="absolute bottom-1 left-0 right-0 flex justify-around px-2">
          <span className="text-[9px] font-black text-[#2a2a2a] uppercase">A</span>
          <span className="text-[9px] font-black text-[#2a2a2a] uppercase">D</span>
          <span className="text-[9px] font-black text-[#2a2a2a] uppercase">S</span>
          <span className="text-[9px] font-black text-[#2a2a2a] uppercase">R</span>
        </div>
      </div>
    </div>
  );
}
