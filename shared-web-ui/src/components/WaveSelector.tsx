/**
 * WaveSelector Component
 *
 * Animated waveform visualization with dropdown selection.
 * Displays sine, saw, square, or triangle waves with smooth animation.
 * Neo-futuristic dark theme with cyan accents.
 */

import { useRef, useEffect, useState, useMemo, useCallback } from 'react';
import { useParameter } from '../glue/hooks/useParameter';
import { useRuntimeParameters } from '../glue/RuntimeParametersProvider';
import type { WaveSelectorProps } from './componentProps';

export type { WaveSelectorProps };

const colorConfig = {
  cyan: {
    primary: '#00ffff',
    secondary: '#0088ff',
    glow: 'rgba(0, 255, 255, 0.8)',
    dim: 'rgba(0, 255, 255, 0.3)',
  },
  magenta: {
    primary: '#ff00ff',
    secondary: '#ff0088',
    glow: 'rgba(255, 0, 255, 0.8)',
    dim: 'rgba(255, 0, 255, 0.3)',
  },
  green: {
    primary: '#00ff88',
    secondary: '#88ff00',
    glow: 'rgba(0, 255, 136, 0.8)',
    dim: 'rgba(0, 255, 136, 0.3)',
  },
  orange: {
    primary: '#ff8800',
    secondary: '#ff0044',
    glow: 'rgba(255, 136, 0, 0.8)',
    dim: 'rgba(255, 136, 0, 0.3)',
  },
};

type WaveType = 'sine' | 'saw' | 'square' | 'triangle';

const WAVE_FUNCTIONS: Record<WaveType, (phase: number) => number> = {
  sine: (phase) => Math.sin(phase * Math.PI * 2),
  saw: (phase) => 2 * (phase - Math.floor(phase + 0.5)),
  square: (phase) => (phase % 1 < 0.5 ? 1 : -1),
  triangle: (phase) => 4 * Math.abs(phase - Math.floor(phase + 0.5)) - 1,
};

const DEFAULT_WAVE_OPTIONS: WaveType[] = ['sine', 'saw', 'square', 'triangle'];

// TODO: In the future, allow custom color mapping via props or context
// For now, each wave type gets a unique color from the palette
const WAVE_COLOR_ORDER: Array<keyof typeof colorConfig> = ['cyan', 'magenta', 'green', 'orange'];

export function WaveSelector({
  paramId,
  label,
  height = 120,
}: WaveSelectorProps) {
  const { value, setValue, beginChange, endChange } = useParameter(paramId);
  const runtimeParameters = useRuntimeParameters();
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const containerRef = useRef<HTMLDivElement>(null);
  const dropdownRef = useRef<HTMLDivElement>(null);
  const animationRef = useRef<number>();
  const phaseRef = useRef(0);
  const ghostPhaseRef = useRef(0);
  const [isOpen, setIsOpen] = useState(false);
  const [canvasWidth, setCanvasWidth] = useState(200);

  // Get options from runtimeParameters or use defaults
  const param = useMemo(
    () => runtimeParameters.find((p) => p.id === paramId),
    [runtimeParameters, paramId]
  );

  const options = useMemo(() => {
    if (param?.enumValues && param.enumValues.length > 0) {
      return param.enumValues.map((v) => v.toLowerCase() as WaveType);
    }
    return DEFAULT_WAVE_OPTIONS;
  }, [param]);

  const normalizedToIndex = useCallback(
    (norm: number): number => {
      return Math.round(norm * (options.length - 1));
    },
    [options.length]
  );

  const indexToNormalized = useCallback(
    (idx: number): number => {
      return options.length > 1 ? idx / (options.length - 1) : 0;
    },
    [options.length]
  );

  const selectedIndex = normalizedToIndex(value);
  const selectedWave = options[selectedIndex] || 'sine';

  // TODO: Make color mapping configurable via props
  // Each wave gets a different color based on its index
  const currentColor = WAVE_COLOR_ORDER[selectedIndex % WAVE_COLOR_ORDER.length];
  const colors = colorConfig[currentColor];

  const handleSelect = useCallback(
    (index: number) => {
      beginChange();
      setValue(indexToNormalized(index));
      endChange();
      setIsOpen(false);
    },
    [beginChange, setValue, endChange, indexToNormalized]
  );

  // Close dropdown on click outside
  useEffect(() => {
    if (!isOpen) return;

    const handleClickOutside = (e: MouseEvent) => {
      if (dropdownRef.current && !dropdownRef.current.contains(e.target as Node)) {
        setIsOpen(false);
      }
    };

    document.addEventListener('mousedown', handleClickOutside);
    return () => document.removeEventListener('mousedown', handleClickOutside);
  }, [isOpen]);

  // Track container width for responsive canvas
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

  // Store selectedWave in a ref so animation loop can access latest value
  const selectedWaveRef = useRef(selectedWave);
  useEffect(() => {
    selectedWaveRef.current = selectedWave;
  }, [selectedWave]);

  // Store colors in ref for animation loop
  const colorsRef = useRef(colors);
  useEffect(() => {
    colorsRef.current = colors;
  }, [colors]);

  // Animation loop - only restart on canvas size change
  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;

    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    const draw = () => {
      const w = canvas.width;
      const h = canvas.height;
      const centerY = h / 2;
      const currentWave = selectedWaveRef.current;
      const currentColors = colorsRef.current;

      // Clear completely each frame (no blur trail)
      ctx.fillStyle = 'rgba(10, 10, 20, 1)';
      ctx.fillRect(0, 0, w, h);

      // Get wave function
      const waveFunc = WAVE_FUNCTIONS[currentWave] || WAVE_FUNCTIONS.sine;

      // Draw grid lines
      ctx.strokeStyle = currentColors.dim;
      ctx.lineWidth = 0.5;
      ctx.setLineDash([2, 4]);

      // Horizontal center line
      ctx.beginPath();
      ctx.moveTo(0, centerY);
      ctx.lineTo(w, centerY);
      ctx.stroke();

      // Vertical lines at 25%, 50%, 75%
      for (const pos of [0.25, 0.5, 0.75]) {
        ctx.beginPath();
        ctx.moveTo(w * pos, 0);
        ctx.lineTo(w * pos, h);
        ctx.stroke();
      }

      ctx.setLineDash([]);

      // Draw ghost wave (trailing behind main wave)
      ctx.strokeStyle = currentColors.dim;
      ctx.lineWidth = 1.5;
      ctx.globalAlpha = 0.4;
      ctx.beginPath();
      for (let x = 0; x < w; x++) {
        const phase = ghostPhaseRef.current + (x / w) * 2;
        const y = centerY - waveFunc(phase) * (h * 0.32);
        if (x === 0) ctx.moveTo(x, y);
        else ctx.lineTo(x, y);
      }
      ctx.stroke();
      ctx.globalAlpha = 1;

      // Create gradient for main wave
      const gradient = ctx.createLinearGradient(0, 0, w, 0);
      gradient.addColorStop(0, currentColors.secondary);
      gradient.addColorStop(0.5, currentColors.primary);
      gradient.addColorStop(1, currentColors.secondary);

      // Draw main waveform line with gradient
      ctx.strokeStyle = gradient;
      ctx.lineWidth = 2;
      ctx.lineCap = 'round';
      ctx.lineJoin = 'round';

      // Add glow effect
      ctx.shadowColor = currentColors.primary;
      ctx.shadowBlur = 8;

      ctx.beginPath();
      for (let x = 0; x < w; x++) {
        const phase = phaseRef.current + (x / w) * 2;
        const y = centerY - waveFunc(phase) * (h * 0.35);
        if (x === 0) ctx.moveTo(x, y);
        else ctx.lineTo(x, y);
      }
      ctx.stroke();

      // Reset shadow
      ctx.shadowBlur = 0;

      // Update phases for animation
      phaseRef.current += 0.015;
      // Ghost wave moves slightly slower for trailing effect
      ghostPhaseRef.current += 0.012;

      animationRef.current = requestAnimationFrame(draw);
    };

    // Initial clear
    ctx.fillStyle = 'rgba(10, 10, 20, 1)';
    ctx.fillRect(0, canvas.width, 0, canvas.height);

    draw();

    return () => {
      if (animationRef.current) {
        cancelAnimationFrame(animationRef.current);
      }
    };
  }, [canvasWidth, height]);

  return (
    <div className="flex flex-col items-center gap-2 w-full">
      {label && (
        <label
          className="text-[10px] font-bold uppercase tracking-[0.15em]"
          style={{ color: colors.primary, textShadow: `0 0 8px ${colors.glow}` }}
        >
          {label}
        </label>
      )}

      {/* Waveform display */}
      <div
        ref={containerRef}
        className="relative rounded-lg overflow-hidden w-full"
        style={{
          height: `${height}px`,
          backgroundColor: 'rgba(10, 10, 20, 1)',
          border: `1px solid ${colors.dim}`,
          boxShadow: `inset 0 0 20px rgba(0,0,0,0.5), 0 0 15px ${colors.dim}`,
        }}
      >
        <canvas ref={canvasRef} width={canvasWidth} height={height} className="block w-full" />

        {/* Corner accents */}
        <div
          className="absolute top-1 left-1 w-3 h-3 border-l border-t"
          style={{ borderColor: colors.dim }}
        />
        <div
          className="absolute top-1 right-1 w-3 h-3 border-r border-t"
          style={{ borderColor: colors.dim }}
        />
        <div
          className="absolute bottom-1 left-1 w-3 h-3 border-l border-b"
          style={{ borderColor: colors.dim }}
        />
        <div
          className="absolute bottom-1 right-1 w-3 h-3 border-r border-b"
          style={{ borderColor: colors.dim }}
        />
      </div>

      {/* Custom dropdown */}
      <div ref={dropdownRef} className="relative">
        <button
          onClick={() => setIsOpen(!isOpen)}
          className="flex items-center gap-2 px-3 py-1.5 rounded-md transition-all"
          style={{
            backgroundColor: 'rgba(10, 10, 20, 0.8)',
            border: `1px solid ${colors.dim}`,
            color: colors.primary,
            boxShadow: isOpen ? `0 0 10px ${colors.glow}` : 'none',
          }}
        >
          <span className="text-[11px] font-bold uppercase tracking-wider min-w-[60px]">
            {selectedWave}
          </span>
          <svg
            width="10"
            height="6"
            viewBox="0 0 10 6"
            fill="none"
            style={{
              transform: isOpen ? 'rotate(180deg)' : 'rotate(0deg)',
              transition: 'transform 0.2s ease',
            }}
          >
            <path d="M1 1L5 5L9 1" stroke={colors.primary} strokeWidth="1.5" strokeLinecap="round" />
          </svg>
        </button>

        {/* Dropdown menu */}
        {isOpen && (
          <div
            className="absolute top-full left-0 right-0 mt-1 rounded-md overflow-hidden z-50"
            style={{
              backgroundColor: 'rgba(5, 5, 10, 0.98)',
              border: `1px solid ${colors.dim}`,
              boxShadow: `0 4px 20px rgba(0,0,0,0.7)`,
            }}
          >
            {options.map((wave, index) => {
              // Each option gets its own color - always full brightness
              const optionColor = colorConfig[WAVE_COLOR_ORDER[index % WAVE_COLOR_ORDER.length]];
              const isSelected = index === selectedIndex;
              return (
                <button
                  key={wave}
                  onClick={() => handleSelect(index)}
                  className="w-full px-3 py-2 text-left transition-all"
                  style={{
                    backgroundColor: isSelected ? `${optionColor.primary}30` : 'transparent',
                    color: optionColor.primary,
                  }}
                  onMouseEnter={(e) => {
                    e.currentTarget.style.backgroundColor = `${optionColor.primary}25`;
                  }}
                  onMouseLeave={(e) => {
                    e.currentTarget.style.backgroundColor = isSelected ? `${optionColor.primary}30` : 'transparent';
                  }}
                >
                  <span className="text-[12px] font-bold uppercase tracking-wider">{wave}</span>
                </button>
              );
            })}
          </div>
        )}
      </div>
    </div>
  );
}
