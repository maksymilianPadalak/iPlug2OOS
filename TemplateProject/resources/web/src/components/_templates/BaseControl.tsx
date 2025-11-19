/**
 * BASE TEMPLATE: Parameter Control Component
 *
 * This is a template for creating new parameter control components.
 * Use this as a reference when AI needs to create new control types.
 *
 * Key patterns demonstrated:
 * - Parameter binding with useParameters hook
 * - MIDI learn functionality
 * - Value formatting and display
 * - Berlin Brutalism styling (black/white, uppercase, monospace)
 * - Mouse/touch interaction patterns
 * - Accessibility features
 */

import React from 'react';
import { useParameters } from '../system/ParameterContext';
import { EParams } from '../../config/constants';

interface BaseControlProps {
  /** Parameter ID from EParams enum */
  paramId: EParams;
  /** Display label */
  label?: string;
  /** Custom size (optional) */
  size?: 'small' | 'medium' | 'large';
}

export function BaseControl({ paramId, label, size = 'medium' }: BaseControlProps) {
  const { paramValues, setParamValue } = useParameters();
  const value = paramValues.get(paramId) ?? 0;

  // Interaction state
  const [isInteracting, setIsInteracting] = React.useState(false);
  const interactionRef = React.useRef<HTMLDivElement>(null);

  // Format parameter value for display (normalized 0-1)
  const formattedValue = React.useMemo(() => {
    return value.toFixed(2);
  }, [value]);

  // Handle mouse/touch interaction start
  const handleInteractionStart = (e: React.MouseEvent | React.TouchEvent) => {
    e.preventDefault();
    setIsInteracting(true);

    // Add global move and end listeners
    const handleMove = (moveEvent: MouseEvent | TouchEvent) => {
      // Calculate new value based on mouse/touch movement
      // (Implementation depends on control type - vertical, horizontal, rotary, etc.)
      const clientY = 'touches' in moveEvent ? moveEvent.touches[0].clientY : moveEvent.clientY;

      // Example: vertical drag (invert Y axis)
      const rect = interactionRef.current?.getBoundingClientRect();
      if (rect) {
        const relativeY = (rect.bottom - clientY) / rect.height;
        const newValue = Math.max(0, Math.min(1, relativeY)); // Normalized 0-1
        setParamValue(paramId, newValue);
      }
    };

    const handleEnd = () => {
      setIsInteracting(false);
      document.removeEventListener('mousemove', handleMove);
      document.removeEventListener('mouseup', handleEnd);
      document.removeEventListener('touchmove', handleMove);
      document.removeEventListener('touchend', handleEnd);
    };

    document.addEventListener('mousemove', handleMove);
    document.addEventListener('mouseup', handleEnd);
    document.addEventListener('touchmove', handleMove);
    document.addEventListener('touchend', handleEnd);
  };

  // Handle double-click to reset to default (0.5 as example)
  const handleDoubleClick = () => {
    setParamValue(paramId, 0.5);
  };

  return (
    <div className="flex flex-col items-center gap-2">
      {/* Label */}
      {label && (
        <label className="text-white text-[10px] font-bold uppercase tracking-widest font-mono">
          {label}
        </label>
      )}

      {/* Control Visual */}
      <div
        ref={interactionRef}
        className={`
          relative bg-black border-2 border-white cursor-pointer select-none
          transition-all
          ${isInteracting ? 'border-cyan-400' : 'border-white'}
          ${size === 'small' ? 'w-12 h-12' : size === 'large' ? 'w-24 h-24' : 'w-16 h-16'}
        `}
        onMouseDown={handleInteractionStart}
        onTouchStart={handleInteractionStart}
        onDoubleClick={handleDoubleClick}
        role="slider"
        aria-label={label || `Parameter ${paramId}`}
        aria-valuemin={0}
        aria-valuemax={1}
        aria-valuenow={value}
        tabIndex={0}
      >
        {/* Visual indicator (progress bar example) */}
        <div
          className="absolute bottom-0 left-0 right-0 bg-white transition-all"
          style={{ height: `${value * 100}%` }}
        />

        {/* Value display */}
        <div className="absolute inset-0 flex items-center justify-center">
          <span className="text-xs font-bold font-mono mix-blend-difference text-white">
            {formattedValue}
          </span>
        </div>
      </div>

      {/* Parameter info */}
      {label && (
        <span className="text-white/60 text-[9px] uppercase tracking-wider font-mono">
          {label}
        </span>
      )}
    </div>
  );
}
