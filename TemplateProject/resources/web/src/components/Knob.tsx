/**
 * Brutalist Knob component - Berlin Brutalism Style
 */

import React, { useRef, useState, useCallback } from 'react';
import { EParams } from '../config/constants';
import { normalizedToDisplay } from '../utils/parameter';
import { useParameters } from './ParameterContext';
import { sendParameterValue } from '../communication/iplug-bridge';
import { isUpdatingFromProcessor } from './ParameterContext';

interface KnobProps {
  paramIdx: EParams;
  label: string;
  min?: number;
  max?: number;
  step?: number;
}

export function Knob({ paramIdx, label, min = 0, max = 1, step = 0.001 }: KnobProps) {
  const { paramValues, setParamValue } = useParameters();
  const value = paramValues.get(paramIdx) ?? 0;
  const knobRef = useRef<HTMLDivElement>(null);
  const [isDragging, setIsDragging] = useState(false);

  const handleMouseDown = useCallback((e: React.MouseEvent) => {
    e.preventDefault();
    setIsDragging(true);
    const startY = e.clientY;
    const startValue = value;

    const handleMouseMove = (moveEvent: MouseEvent) => {
      const deltaY = startY - moveEvent.clientY;
      const sensitivity = 0.005;
      const newValue = Math.max(min, Math.min(max, startValue + deltaY * sensitivity));
      
      setParamValue(paramIdx, newValue);
      
      if (!isUpdatingFromProcessor()) {
        sendParameterValue(paramIdx, newValue);
      }
    };

    const handleMouseUp = () => {
      setIsDragging(false);
      document.removeEventListener('mousemove', handleMouseMove);
      document.removeEventListener('mouseup', handleMouseUp);
    };

    document.addEventListener('mousemove', handleMouseMove);
    document.addEventListener('mouseup', handleMouseUp);
  }, [paramIdx, value, min, max, setParamValue]);

  const normalizedValue = (value - min) / (max - min);
  const rotation = normalizedValue * 270 - 135; // -135° to +135°

  return (
    <div className="flex flex-col items-center gap-1">
      <label className="text-white text-xs font-mono uppercase tracking-wider">
        {label}
      </label>
      <div
        ref={knobRef}
        onMouseDown={handleMouseDown}
        className="relative w-12 h-12 bg-black border-4 border-white cursor-pointer select-none"
        style={{
          transform: isDragging ? 'translate(2px, 2px)' : 'translate(0, 0)',
        }}
      >
        {/* Indicator line */}
        <div
          className="absolute top-0 left-1/2 w-1 bg-white origin-bottom"
          style={{
            height: '40%',
            transform: `translateX(-50%) rotate(${rotation}deg)`,
            transformOrigin: 'bottom center',
          }}
        />
      </div>
      <div className="text-gray-400 text-[10px] font-mono text-center min-w-[50px]">
        {normalizedToDisplay(paramIdx, value)}
      </div>
    </div>
  );
}

