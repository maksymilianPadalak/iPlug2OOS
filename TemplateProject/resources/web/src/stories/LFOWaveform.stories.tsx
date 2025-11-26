import type { Meta, StoryObj } from '@storybook/react';
import { MockedLFOWaveform, MockedWaveformDisplay } from '@/stories/mocks/MockedLFOWaveform';

const meta = {
  title: 'Visualizations/LFOWaveform',
  component: MockedLFOWaveform,
  parameters: {
    layout: 'centered',
  },
  tags: ['autodocs'],
  argTypes: {
    waveform: {
      control: { type: 'select' },
      options: ['sine', 'triangle', 'square', 'sawtooth', 'random'],
      description: 'Waveform type',
    },
    frequency: {
      control: { type: 'range', min: 0.1, max: 10, step: 0.1 },
      description: 'LFO frequency in Hz',
    },
    width: {
      control: { type: 'number' },
      description: 'Canvas width',
    },
    height: {
      control: { type: 'number' },
      description: 'Canvas height',
    },
    color: {
      control: 'color',
      description: 'Waveform line color',
    },
    backgroundColor: {
      control: 'color',
      description: 'Background color',
    },
    animated: {
      control: 'boolean',
      description: 'Enable animation',
    },
  },
} satisfies Meta<typeof MockedLFOWaveform>;

export default meta;
type Story = StoryObj<typeof meta>;

// Sine wave (default)
export const Sine: Story = {
  args: {
    waveform: 'sine',
    frequency: 1,
    animated: true,
  },
};

// Triangle wave
export const Triangle: Story = {
  args: {
    waveform: 'triangle',
    frequency: 1,
    animated: true,
  },
};

// Square wave
export const Square: Story = {
  args: {
    waveform: 'square',
    frequency: 1,
    animated: true,
  },
};

// Sawtooth wave
export const Sawtooth: Story = {
  args: {
    waveform: 'sawtooth',
    frequency: 1,
    animated: true,
  },
};

// Random (S&H style)
export const Random: Story = {
  args: {
    waveform: 'random',
    frequency: 4,
    animated: true,
  },
};

// Static (no animation)
export const Static: Story = {
  args: {
    waveform: 'sine',
    frequency: 1,
    animated: false,
  },
};

// Fast LFO
export const FastLFO: Story = {
  args: {
    waveform: 'sine',
    frequency: 5,
    animated: true,
  },
};

// Slow LFO
export const SlowLFO: Story = {
  args: {
    waveform: 'sine',
    frequency: 0.2,
    animated: true,
  },
};

// Custom colors
export const CustomColors: Story = {
  args: {
    waveform: 'sine',
    frequency: 1,
    color: '#fb923c',
    backgroundColor: '#1c1917',
    animated: true,
  },
};

// Large display
export const Large: Story = {
  args: {
    waveform: 'sine',
    frequency: 1,
    width: 400,
    height: 80,
    animated: true,
  },
};

// All waveforms side by side
export const AllWaveforms: Story = {
  render: () => (
    <div className="flex flex-col gap-4">
      <div className="flex gap-4">
        <MockedWaveformDisplay waveform="sine" label="Sine" size="md" />
        <MockedWaveformDisplay waveform="triangle" label="Triangle" size="md" />
        <MockedWaveformDisplay waveform="square" label="Square" size="md" />
      </div>
      <div className="flex gap-4">
        <MockedWaveformDisplay waveform="sawtooth" label="Saw" size="md" />
        <MockedWaveformDisplay waveform="random" label="S&H" size="md" />
      </div>
    </div>
  ),
};

// Waveform selector display
export const WaveformSelector: Story = {
  render: () => (
    <div className="flex gap-2 p-3 bg-black/40 rounded-lg border border-orange-900/40">
      <MockedWaveformDisplay waveform="sine" size="sm" />
      <MockedWaveformDisplay waveform="triangle" size="sm" />
      <MockedWaveformDisplay waveform="square" size="sm" />
      <MockedWaveformDisplay waveform="sawtooth" size="sm" />
    </div>
  ),
};

// In LFO section
export const InLFOSection: Story = {
  render: () => (
    <div className="p-4 bg-gradient-to-br from-stone-900 to-black border border-orange-900/40 rounded-xl">
      <div className="flex items-center justify-between mb-3">
        <h2 className="text-orange-400 text-xs font-bold uppercase tracking-wider">LFO 1</h2>
        <span className="text-orange-200/60 text-[10px] uppercase">1.0 Hz</span>
      </div>
      <MockedLFOWaveform 
        waveform="sine" 
        frequency={1} 
        width={250} 
        height={50}
        color="#fb923c"
        backgroundColor="#0a0a0a"
      />
    </div>
  ),
};

