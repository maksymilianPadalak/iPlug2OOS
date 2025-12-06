/**
 * ADSR Display Component
 *
 * Visualizes an ADSR envelope curve based on parameter values.
 * Streamline Moderne style with smooth curves and gradient fills.
 */

import { useMemo } from 'react';
import { useParameter } from '@/glue/hooks/useParameter';

type ADSRDisplayProps = {
  attackParam: number;
  decayParam: number;
  sustainParam: number;
  releaseParam: number;
  label?: string;
};

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
    <div className="flex flex-col items-center gap-1">
      {label && (
        <label className="text-orange-200 text-[10px] font-bold uppercase tracking-wider">
          {label}
        </label>
      )}

      <div className="relative bg-gradient-to-br from-stone-900/80 to-black/80 border border-orange-700/30 rounded-xl overflow-hidden shadow-lg">
        {/* Subtle grid lines */}
        <svg
          width={width}
          height={height}
          className="absolute inset-0 opacity-20"
        >
          {/* Horizontal grid lines */}
          {[0.25, 0.5, 0.75].map((y) => (
            <line
              key={`h-${y}`}
              x1={padding.left}
              y1={padding.top + graphHeight * y}
              x2={width - padding.right}
              y2={padding.top + graphHeight * y}
              stroke="#fb923c"
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
              stroke="#fb923c"
              strokeWidth="0.5"
              strokeDasharray="2,4"
            />
          ))}
        </svg>

        {/* Main envelope SVG */}
        <svg width={width} height={height}>
          <defs>
            {/* Gradient fill */}
            <linearGradient id={gradientId} x1="0%" y1="0%" x2="0%" y2="100%">
              <stop offset="0%" stopColor="#fb923c" stopOpacity="0.4" />
              <stop offset="50%" stopColor="#ea580c" stopOpacity="0.2" />
              <stop offset="100%" stopColor="#9a3412" stopOpacity="0.05" />
            </linearGradient>
            {/* Glow filter */}
            <filter id="glow" x="-50%" y="-50%" width="200%" height="200%">
              <feGaussianBlur stdDeviation="2" result="blur" />
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
            stroke="#fb923c"
            strokeWidth="2.5"
            strokeLinecap="round"
            strokeLinejoin="round"
            filter="url(#glow)"
          />

          {/* Crisp envelope curve on top */}
          <path
            d={path}
            fill="none"
            stroke="#fdba74"
            strokeWidth="1.5"
            strokeLinecap="round"
            strokeLinejoin="round"
          />

          {/* Corner accents - Streamline Moderne style */}
          <path
            d={`M ${padding.left} ${padding.top + 12} L ${padding.left} ${padding.top} L ${padding.left + 12} ${padding.top}`}
            fill="none"
            stroke="#fb923c"
            strokeWidth="1"
            strokeOpacity="0.5"
          />
          <path
            d={`M ${width - padding.right - 12} ${padding.top} L ${width - padding.right} ${padding.top} L ${width - padding.right} ${padding.top + 12}`}
            fill="none"
            stroke="#fb923c"
            strokeWidth="1"
            strokeOpacity="0.5"
          />
        </svg>

        {/* ADSR labels at bottom */}
        <div className="absolute bottom-1 left-0 right-0 flex justify-around px-2">
          <span className="text-[8px] font-bold text-orange-500/60 uppercase">A</span>
          <span className="text-[8px] font-bold text-orange-500/60 uppercase">D</span>
          <span className="text-[8px] font-bold text-orange-500/60 uppercase">S</span>
          <span className="text-[8px] font-bold text-orange-500/60 uppercase">R</span>
        </div>
      </div>
    </div>
  );
}
