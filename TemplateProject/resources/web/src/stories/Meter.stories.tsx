import type { Meta, StoryObj } from '@storybook/react';
import { MockedMeter, MockedBarMeter } from '@/stories/mocks/MockedMeter';

const meta = {
  title: 'Visualizations/Meter',
  component: MockedMeter,
  parameters: {
    layout: 'centered',
  },
  tags: ['autodocs'],
  argTypes: {
    peakDb: {
      control: { type: 'range', min: -60, max: 6, step: 0.5 },
      description: 'Peak level in dB',
    },
    channel: {
      control: { type: 'select' },
      options: [0, 1],
      description: 'Audio channel (0=L, 1=R)',
    },
  },
} satisfies Meta<typeof MockedMeter>;

export default meta;
type Story = StoryObj<typeof meta>;

// Normal level
export const Normal: Story = {
  args: {
    peakDb: -12,
    channel: 0,
  },
};

// Hot level (approaching 0dB)
export const Hot: Story = {
  args: {
    peakDb: -3,
    channel: 0,
  },
};

// Clipping
export const Clipping: Story = {
  args: {
    peakDb: 0.5,
    channel: 0,
  },
};

// Very quiet
export const Quiet: Story = {
  args: {
    peakDb: -40,
    channel: 0,
  },
};

// Silence
export const Silence: Story = {
  args: {
    peakDb: -Infinity,
    channel: 0,
  },
};

// Stereo pair
export const StereoPair: Story = {
  render: () => (
    <div className="flex gap-4 items-center">
      <div className="flex flex-col items-center gap-1">
        <span className="text-orange-200 text-[10px] font-bold uppercase tracking-wider">L</span>
        <MockedMeter peakDb={-8} channel={0} />
      </div>
      <div className="flex flex-col items-center gap-1">
        <span className="text-orange-200 text-[10px] font-bold uppercase tracking-wider">R</span>
        <MockedMeter peakDb={-10} channel={1} />
      </div>
    </div>
  ),
};

// Bar meter variants
export const BarMeterVertical: Story = {
  render: () => (
    <div className="flex gap-4">
      <MockedBarMeter value={0.5} label="L" orientation="vertical" />
      <MockedBarMeter value={0.6} label="R" orientation="vertical" />
    </div>
  ),
};

export const BarMeterHorizontal: Story = {
  render: () => (
    <div className="flex flex-col gap-2">
      <MockedBarMeter value={0.7} label="Level" orientation="horizontal" />
      <MockedBarMeter value={0.4} label="Reduction" orientation="horizontal" />
    </div>
  ),
};

// Mixer channel meters
export const MixerMeters: Story = {
  render: () => (
    <div className="flex gap-2 p-4 bg-black/40 rounded-xl border border-orange-900/40">
      <MockedBarMeter value={0.6} label="1" orientation="vertical" />
      <MockedBarMeter value={0.75} label="2" orientation="vertical" />
      <MockedBarMeter value={0.4} label="3" orientation="vertical" />
      <MockedBarMeter value={0.5} label="4" orientation="vertical" />
      <div className="w-px bg-orange-700/30 mx-2" />
      <MockedBarMeter value={0.8} label="M" orientation="vertical" />
    </div>
  ),
};

// All states
export const AllStates: Story = {
  render: () => (
    <div className="flex flex-col gap-4">
      <div className="flex gap-4">
        <MockedMeter peakDb={-40} />
        <span className="text-orange-200 text-xs self-center">Quiet (-40 dB)</span>
      </div>
      <div className="flex gap-4">
        <MockedMeter peakDb={-12} />
        <span className="text-orange-200 text-xs self-center">Normal (-12 dB)</span>
      </div>
      <div className="flex gap-4">
        <MockedMeter peakDb={-3} />
        <span className="text-orange-200 text-xs self-center">Hot (-3 dB)</span>
      </div>
      <div className="flex gap-4">
        <MockedMeter peakDb={0.5} />
        <span className="text-orange-200 text-xs self-center">Clipping (+0.5 dB)</span>
      </div>
    </div>
  ),
};

