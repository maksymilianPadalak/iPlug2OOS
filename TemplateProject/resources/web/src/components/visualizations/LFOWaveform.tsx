import React, { useRef, useEffect } from 'react';
import { useParameters } from '../system/ParameterContext';

export function LFOWaveform() {
  const { lfoWaveform } = useParameters();
  const canvasRef = useRef<HTMLCanvasElement>(null);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;

    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    const width = canvas.width;
    const height = canvas.height;

    function drawWaveform() {
      if (!ctx) return;

      ctx.fillStyle = '#ef0000';
      ctx.fillRect(0, 0, width, height);

      ctx.strokeStyle = '#ffffff';
      ctx.lineWidth = 2;
      ctx.beginPath();

      const centerY = height / 2;
      const scaleY = height / 2;

      for (let i = 0; i < width; i++) {
        const bufferIdx = Math.floor((i / width) * lfoWaveform.length);
        const value = lfoWaveform[bufferIdx];
        const y = centerY - (value - 0.5) * scaleY;

        if (i === 0) {
          ctx.moveTo(i, y);
        } else {
          ctx.lineTo(i, y);
        }
      }

      ctx.stroke();
    }

    drawWaveform();

    let animationFrame: number;
    function animate() {
      if (!ctx) return;
      drawWaveform();
      animationFrame = requestAnimationFrame(animate);
    }
    animate();

    return () => {
      if (animationFrame) {
        cancelAnimationFrame(animationFrame);
      }
    };
  }, [lfoWaveform]);

  return (
    <div>
      <label className="block text-white text-[10px] font-mono uppercase tracking-wider mb-1">
        LFO WAVEFORM
      </label>
      <canvas
        ref={canvasRef}
        width={300}
        height={40}
        className="w-full h-10 border-2 border-white bg-black block"
      />
    </div>
  );
}
