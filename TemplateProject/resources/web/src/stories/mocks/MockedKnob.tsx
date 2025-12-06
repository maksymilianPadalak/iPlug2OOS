/**
 * Mocked Knob for Storybook
 */

import { useState } from 'react';

type MockedKnobProps = {
  paramId: number;
  label?: string;
  initialValue?: number;
};

export function MockedKnob({ paramId, label, initialValue = 0.75 }: MockedKnobProps) {
  const [value, setValue] = useState(initialValue);
  const [isDragging, setIsDragging] = useState(false);

  const size = 48;
  const stroke = 4;
  const radius = (size - stroke) / 2;
  const circumference = 2 * Math.PI * radius;
  const progress = circumference * value;

  const handleMouseDown = (e: React.MouseEvent) => {
    e.preventDefault();
    const startY = e.clientY;
    const startValue = value;
    setIsDragging(true);

    const handleMove = (ev: MouseEvent) => {
      const deltaY = startY - ev.clientY;
      const newValue = Math.max(0, Math.min(1, startValue + deltaY * 0.005));
      setValue(newValue);
    };

    const handleUp = () => {
      setIsDragging(false);
      document.removeEventListener('mousemove', handleMove);
      document.removeEventListener('mouseup', handleUp);
    };

    document.addEventListener('mousemove', handleMove);
    document.addEventListener('mouseup', handleUp);
  };

  return (
    <div className="flex flex-col items-center gap-1">
      {label && (
        <label className="text-orange-200 text-[10px] font-bold uppercase tracking-wider">
          {label}
        </label>
      )}

      <div
        onMouseDown={handleMouseDown}
        className={`cursor-pointer select-none ${isDragging ? 'opacity-80' : ''}`}
      >
        <svg width={size} height={size}>
          <circle
            cx={size / 2}
            cy={size / 2}
            r={radius}
            fill="none"
            stroke="#292524"
            strokeWidth={stroke}
          />
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
          <circle cx={size / 2} cy={size / 2} r={6} fill="#1c1917" stroke="#fb923c" strokeWidth={1.5} />
        </svg>
      </div>

      <div className="text-orange-300 text-[10px] font-bold">
        {(value * 100).toFixed(0)}%
      </div>
    </div>
  );
}
