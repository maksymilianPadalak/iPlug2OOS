/**
 * Knob Control Component
 *
 * Rotary control for continuous parameters.
 * Drag up/down to change value.
 */

import { useRef, useState, useCallback } from 'react';
import { normalizedToDisplay } from '@/utils/parameter';
import { useParameter } from '@/glue/hooks/useParameter';

type KnobProps = {
  paramId: number;
  label?: string;
};

export function Knob({ paramId, label }: KnobProps) {
  const { value, setValue, beginChange, endChange } = useParameter(paramId);
  const [isDragging, setIsDragging] = useState(false);
  const knobRef = useRef<HTMLDivElement>(null);

  const clamp = (v: number) => Math.max(0, Math.min(1, v));

  const startDrag = useCallback((startY: number, startValue: number) => {
    setIsDragging(true);
    beginChange();

    const onMove = (y: number) => {
      const delta = (startY - y) * 0.005;
      setValue(clamp(startValue + delta));
    };

    const onMouseMove = (e: MouseEvent) => {
      e.preventDefault();
      onMove(e.clientY);
    };

    const onTouchMove = (e: TouchEvent) => {
      e.preventDefault();
      if (e.touches[0]) onMove(e.touches[0].clientY);
    };

    const onEnd = () => {
      endChange();
      setIsDragging(false);
      document.removeEventListener('mousemove', onMouseMove);
      document.removeEventListener('mouseup', onEnd);
      document.removeEventListener('touchmove', onTouchMove);
      document.removeEventListener('touchend', onEnd);
    };

    document.addEventListener('mousemove', onMouseMove, { passive: false });
    document.addEventListener('mouseup', onEnd);
    document.addEventListener('touchmove', onTouchMove, { passive: false });
    document.addEventListener('touchend', onEnd);
  }, [setValue, beginChange, endChange]);

  const handleMouseDown = (e: React.MouseEvent) => {
    e.preventDefault();
    startDrag(e.clientY, value);
  };

  const handleTouchStart = (e: React.TouchEvent) => {
    e.preventDefault();
    if (e.touches[0]) startDrag(e.touches[0].clientY, value);
  };

  // Arc calculation
  const size = 48;
  const stroke = 4;
  const radius = (size - stroke) / 2;
  const circumference = 2 * Math.PI * radius;
  const progress = circumference * value;

  return (
    <div className="flex flex-col items-center gap-1">
      {label && (
        <label className="text-orange-200 text-[10px] font-bold uppercase tracking-wider">
          {label}
        </label>
      )}

      <div
        ref={knobRef}
        onMouseDown={handleMouseDown}
        onTouchStart={handleTouchStart}
        className={`cursor-pointer select-none ${isDragging ? 'opacity-80' : ''}`}
      >
        <svg width={size} height={size}>
          {/* Background track */}
          <circle
            cx={size / 2}
            cy={size / 2}
            r={radius}
            fill="none"
            stroke="#292524"
            strokeWidth={stroke}
          />
          {/* Progress arc */}
          <circle
            cx={size / 2}
            cy={size / 2}
            r={radius}
            fill="none"
            stroke="#fb923c"
            strokeWidth={stroke}
            strokeLinecap="round"
            strokeDasharray={`${progress} ${circumference}`}
            transform={`rotate(-90 ${size / 2} ${size / 2})`}
          />
          {/* Center dot */}
          <circle cx={size / 2} cy={size / 2} r={6} fill="#1c1917" stroke="#fb923c" strokeWidth={1.5} />
        </svg>
      </div>

      <div className="text-orange-300 text-[10px] font-bold">
        {normalizedToDisplay(paramId, value)}
      </div>
    </div>
  );
}
