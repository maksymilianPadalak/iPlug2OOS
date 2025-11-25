/**
 * Mocked LFO Waveform for Storybook
 * Displays animated waveform without ParameterContext
 */

import React, { useRef, useEffect, useState } from 'react';

type WaveformType = 'sine' | 'triangle' | 'square' | 'sawtooth' | 'random';

type MockedLFOWaveformProps = {
  waveform?: WaveformType;
  frequency?: number; // Hz
  width?: number;
  height?: number;
  color?: string;
  backgroundColor?: string;
  animated?: boolean;
};

function generateWaveform(type: WaveformType, length: number, phase: number = 0): Float32Array {
  const data = new Float32Array(length);
  
  for (let i = 0; i < length; i++) {
    const t = (i / length + phase) % 1;
    
    switch (type) {
      case 'sine':
        data[i] = 0.5 + 0.5 * Math.sin(t * 2 * Math.PI);
        break;
      case 'triangle':
        data[i] = t < 0.5 ? t * 2 : 2 - t * 2;
        break;
      case 'square':
        data[i] = t < 0.5 ? 1 : 0;
        break;
      case 'sawtooth':
        data[i] = t;
        break;
      case 'random':
        data[i] = Math.random();
        break;
    }
  }
  
  return data;
}

export function MockedLFOWaveform({ 
  waveform = 'sine',
  frequency = 1,
  width = 300,
  height = 40,
  color = '#ffffff',
  backgroundColor = '#ef0000',
  animated = true
}: MockedLFOWaveformProps) {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const [phase, setPhase] = useState(0);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;

    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    const waveformData = generateWaveform(waveform, 512, phase);

    // Clear canvas
    ctx.fillStyle = backgroundColor;
    ctx.fillRect(0, 0, width, height);

    // Draw waveform
    ctx.strokeStyle = color;
    ctx.lineWidth = 2;
    ctx.beginPath();

    const centerY = height / 2;
    const scaleY = height / 2;

    for (let i = 0; i < width; i++) {
      const bufferIdx = Math.floor((i / width) * waveformData.length);
      const value = waveformData[bufferIdx];
      const y = centerY - (value - 0.5) * scaleY;

      if (i === 0) {
        ctx.moveTo(i, y);
      } else {
        ctx.lineTo(i, y);
      }
    }

    ctx.stroke();
  }, [waveform, width, height, color, backgroundColor, phase]);

  // Animation loop
  useEffect(() => {
    if (!animated) return;

    let animationFrame: number;
    let lastTime = performance.now();

    const animate = (currentTime: number) => {
      const deltaTime = (currentTime - lastTime) / 1000;
      lastTime = currentTime;
      
      setPhase(prev => (prev + frequency * deltaTime) % 1);
      animationFrame = requestAnimationFrame(animate);
    };

    animationFrame = requestAnimationFrame(animate);

    return () => {
      cancelAnimationFrame(animationFrame);
    };
  }, [animated, frequency]);

  return (
    <div>
      <label className="block text-white text-[10px] font-mono uppercase tracking-wider mb-1">
        LFO WAVEFORM
      </label>
      <canvas
        ref={canvasRef}
        width={width}
        height={height}
        className="w-full border-2 border-white bg-black block"
        style={{ height: `${height}px` }}
      />
    </div>
  );
}

// Compact waveform display (static)
type MockedWaveformDisplayProps = {
  waveform?: WaveformType;
  label?: string;
  size?: 'sm' | 'md' | 'lg';
};

export function MockedWaveformDisplay({ 
  waveform = 'sine', 
  label,
  size = 'md' 
}: MockedWaveformDisplayProps) {
  const sizes = {
    sm: { width: 60, height: 24 },
    md: { width: 100, height: 32 },
    lg: { width: 150, height: 48 },
  };

  const { width, height } = sizes[size];
  const canvasRef = useRef<HTMLCanvasElement>(null);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;

    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    const waveformData = generateWaveform(waveform, 128);

    // Clear canvas
    ctx.fillStyle = '#1c1917';
    ctx.fillRect(0, 0, width, height);

    // Draw waveform
    ctx.strokeStyle = '#fb923c';
    ctx.lineWidth = 1.5;
    ctx.beginPath();

    for (let i = 0; i < width; i++) {
      const bufferIdx = Math.floor((i / width) * waveformData.length);
      const value = waveformData[bufferIdx];
      const y = height - value * height;

      if (i === 0) {
        ctx.moveTo(i, y);
      } else {
        ctx.lineTo(i, y);
      }
    }

    ctx.stroke();
  }, [waveform, width, height]);

  return (
    <div className="flex flex-col items-center gap-1">
      {label && (
        <label className="text-orange-200 text-[10px] font-bold uppercase tracking-wider">
          {label}
        </label>
      )}
      <canvas
        ref={canvasRef}
        width={width}
        height={height}
        className="border border-orange-700/40 rounded"
      />
    </div>
  );
}

