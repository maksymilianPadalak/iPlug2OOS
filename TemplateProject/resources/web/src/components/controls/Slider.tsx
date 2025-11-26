/**
 * Slider Control Component
 *
 * Linear control for panning, faders, and continuous parameters.
 * Supports horizontal/vertical orientation.
 */

import React from 'react';
import { normalizedToDisplay } from '@/utils/parameter';
import { useParameter } from '@/glue/hooks/useParameter';

type SliderProps = {
  paramId: number;
  label?: string;
  orientation?: 'horizontal' | 'vertical';
};

export function Slider({ paramId, label, orientation = 'horizontal' }: SliderProps) {
  const { value, setValue, beginChange, endChange } = useParameter(paramId);

  const handlePointerDown = () => {
    beginChange(); // Signal start of parameter change for automation
  };

  const handlePointerUp = () => {
    endChange(); // Signal end of parameter change for automation
  };

  const handleChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    const newValue = Number(e.target.value);
    setValue(newValue);
  };

  const isVertical = orientation === 'vertical';
  const percentage = value * 100;

  return (
    <div className={`flex ${isVertical ? 'flex-col items-center h-32' : 'flex-col'} gap-2`}>
      {label && (
        <label className="text-orange-200 text-[10px] font-bold uppercase tracking-wider">
          {label}
        </label>
      )}
      <input
        type="range"
        min={0}
        max={1}
        step={0.001}
        value={value}
        onChange={handleChange}
        onPointerDown={handlePointerDown}
        onPointerUp={handlePointerUp}
        className={`
          ${isVertical ? 'h-24 w-2' : 'w-full h-2'}
          appearance-none cursor-pointer rounded-full
          bg-stone-800 border border-orange-600/30
          [&::-webkit-slider-thumb]:appearance-none
          [&::-webkit-slider-thumb]:w-4
          [&::-webkit-slider-thumb]:h-4
          [&::-webkit-slider-thumb]:rounded-full
          [&::-webkit-slider-thumb]:bg-orange-400
          [&::-webkit-slider-thumb]:border-2
          [&::-webkit-slider-thumb]:border-orange-600
          [&::-webkit-slider-thumb]:shadow-md
          [&::-webkit-slider-thumb]:cursor-pointer
          [&::-webkit-slider-thumb]:transition-all
          [&::-webkit-slider-thumb]:hover:bg-orange-300
        `}
        style={{
          background: isVertical
            ? `linear-gradient(to top, #fb923c 0%, #fb923c ${percentage}%, #292524 ${percentage}%, #292524 100%)`
            : `linear-gradient(to right, #fb923c 0%, #fb923c ${percentage}%, #292524 ${percentage}%, #292524 100%)`,
          writingMode: isVertical ? 'vertical-lr' : undefined,
          direction: isVertical ? 'rtl' : undefined,
        }}
      />
      <div className="text-orange-300 text-[10px] font-bold text-center">
        {normalizedToDisplay(paramId, value)}
      </div>
    </div>
  );
}
