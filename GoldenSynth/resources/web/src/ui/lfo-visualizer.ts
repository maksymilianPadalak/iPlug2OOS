/**
 * LFO waveform visualizer
 */

const LFO_WAVEFORM_BUFFER_SIZE = 512;
let lfoWaveformBuffer = new Array(LFO_WAVEFORM_BUFFER_SIZE).fill(0.5);
let lfoWaveformIndex = 0;

/**
 * Initialize LFO waveform canvas
 */
export function initLFOWaveform(): void {
  const canvas = document.getElementById('lfoWaveform') as HTMLCanvasElement;
  if (!canvas) return;

  const ctx = canvas.getContext('2d');
  if (!ctx) return;

  const width = canvas.width;
  const height = canvas.height;

  function drawWaveform(): void {
    if (!ctx) return;
    
    ctx.fillStyle = '#000000';
    ctx.fillRect(0, 0, width, height);

    ctx.strokeStyle = '#ffffff';
    ctx.lineWidth = 2;
    ctx.beginPath();

    const centerY = height / 2;
    const scaleY = height / 2;

    for (let i = 0; i < width; i++) {
      const bufferIdx = Math.floor((i / width) * lfoWaveformBuffer.length);
      const value = lfoWaveformBuffer[bufferIdx];
      const y = centerY - (value - 0.5) * scaleY;

      if (i === 0) {
        ctx.moveTo(i, y);
      } else {
        ctx.lineTo(i, y);
      }
    }

    ctx.stroke();
  }

  function animate(): void {
    if (!ctx) return;
    drawWaveform();
    requestAnimationFrame(animate);
  }

  animate();
}

/**
 * Update LFO waveform buffer with new value
 */
export function updateLFOWaveform(value: number): void {
  lfoWaveformBuffer[lfoWaveformIndex] = value;
  lfoWaveformIndex = (lfoWaveformIndex + 1) % lfoWaveformBuffer.length;
}

// Expose to global scope for callbacks
declare global {
  interface Window {
    updateLFOWaveform?: (value: number) => void;
  }
}

window.updateLFOWaveform = updateLFOWaveform;

