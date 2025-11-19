/**
 * WaveformDisplay Component
 * SVG-based waveform visualization with animation
 */

import React from 'react';

interface WaveformDisplayProps {
  waveIndex: number;
  color: string;
}

export function WaveformDisplay({ waveIndex, color }: WaveformDisplayProps) {
  const [offset, setOffset] = React.useState(0);
  const width = 180;
  const height = 40;
  const padding = 8;
  const centerY = height / 2;

  // unique filter id to avoid collisions
  const filterIdRef = React.useRef(`glow-${Math.random().toString(36).slice(2, 9)}`);
  const filterId = filterIdRef.current;

  // Animate the waveform scrolling
  React.useEffect(() => {
    let animationFrame: number;
    let lastTime = Date.now();

    const animate = () => {
      const now = Date.now();
      const delta = (now - lastTime) / 1000; // Convert to seconds
      lastTime = now;

      setOffset((prev) => (prev + delta * 0.5) % 1); // Scroll speed
      animationFrame = requestAnimationFrame(animate);
    };

    animationFrame = requestAnimationFrame(animate);
    return () => cancelAnimationFrame(animationFrame);
  }, []);

  // Generate path data for different waveforms
  const generatePath = (phaseOffset: number) => {
    const points: string[] = [];
    const cycles = 2;
    const resolution = 100;

    for (let i = 0; i <= resolution; i++) {
      const x = padding + (i / resolution) * (width - padding * 2);
      const t = ((i / resolution) + phaseOffset) * cycles * Math.PI * 2;
      let y = 0;

      switch (waveIndex) {
        case 0: // Sine
          y = Math.sin(t);
          break;
        case 1: // Saw
          y = 2 * ((t / (Math.PI * 2)) % 1) - 1;
          break;
        case 2: // Square
          y = Math.sin(t) >= 0 ? 1 : -1;
          break;
        case 3: // Triangle
          const normalized = (t / (Math.PI * 2)) % 1;
          y = normalized < 0.5 ? 4 * normalized - 1 : -4 * normalized + 3;
          break;
      }

      const scaledY = centerY - y * (height / 2 - padding);
      points.push(i === 0 ? `M ${x} ${scaledY}` : `L ${x} ${scaledY}`);
    }

    return points.join(' ');
  };

  const pathD = React.useMemo(() => generatePath(offset), [offset, waveIndex]);

  return (
    <div className="w-full bg-black/40 border border-orange-700/30 rounded p-1 overflow-hidden">
      <svg width={width} height={height} viewBox={`0 0 ${width} ${height}`} className="w-full">
        {/* Grid lines */}
        <line x1={padding} y1={centerY} x2={width - padding} y2={centerY} stroke="#ffffff10" strokeWidth={1} />

        {/* Waveform with glow effect */}
        <defs>
          <filter id={filterId} x="-50%" y="-50%" width="200%" height="200%">
            <feGaussianBlur stdDeviation="3" result="coloredBlur" />
            <feMerge>
              <feMergeNode in="coloredBlur" />
              <feMergeNode in="SourceGraphic" />
            </feMerge>
          </filter>
        </defs>

        {/* blurred thicker stroke for glow */}
        <path
          d={pathD}
          fill="none"
          stroke={color}
          strokeWidth={4.5}
          strokeLinecap="round"
          strokeLinejoin="round"
          opacity={0.35}
          filter={`url(#${filterId})`}
        />

        <path
          d={pathD}
          fill="none"
          stroke={color}
          strokeWidth={2.5}
          strokeLinecap="round"
          strokeLinejoin="round"
        />
      </svg>
    </div>
  );
}
