import type { Meta, StoryObj } from '@storybook/react';
import { MockedSlider } from '@/stories/mocks/MockedSlider';
import { EParams } from '@/config/runtimeParameters';

const meta = {
  title: 'Controls/Slider',
  component: MockedSlider,
  parameters: {
    layout: 'centered',
  },
  tags: ['autodocs'],
  argTypes: {
    paramId: {
      control: { type: 'select' },
      options: [0, 1],
      description: 'Parameter index to control',
    },
    label: {
      control: 'text',
      description: 'Label displayed above the slider',
    },
    orientation: {
      control: { type: 'select' },
      options: ['horizontal', 'vertical'],
      description: 'Slider orientation',
    },
    initialValue: {
      control: { type: 'range', min: 0, max: 1, step: 0.01 },
      description: 'Initial value (0-1)',
    },
  },
} satisfies Meta<typeof MockedSlider>;

export default meta;
type Story = StoryObj<typeof meta>;

// Orientations
export const Horizontal: Story = {
  args: {
    paramId: EParams.kParamGain,
    label: 'Pan',
    orientation: 'horizontal',
    initialValue: 0.5,
  },
};

export const Vertical: Story = {
  args: {
    paramId: EParams.kParamGain,
    label: 'Fader',
    orientation: 'vertical',
    initialValue: 0.7,
  },
};

// Without label
export const NoLabel: Story = {
  args: {
    paramId: EParams.kParamGain,
    orientation: 'horizontal',
    initialValue: 0.5,
  },
};

// Both orientations
export const BothOrientations: Story = {
  args: { paramId: 0 },
  render: () => (
    <div className="flex items-start gap-8">
      <div className="w-48">
        <MockedSlider paramId={EParams.kParamGain} label="Pan" orientation="horizontal" initialValue={0.5} />
      </div>
      <MockedSlider paramId={EParams.kParamGain} label="Volume" orientation="vertical" initialValue={0.7} />
    </div>
  ),
};

// Mixer-style faders
export const MixerFaders: Story = {
  args: { paramId: 0 },
  render: () => (
    <div className="flex gap-4 p-4 bg-black/40 rounded-xl border border-orange-900/40">
      <MockedSlider paramId={EParams.kParamGain} label="Ch 1" orientation="vertical" initialValue={0.6} />
      <MockedSlider paramId={EParams.kParamGain} label="Ch 2" orientation="vertical" initialValue={0.8} />
      <MockedSlider paramId={EParams.kParamGain} label="Ch 3" orientation="vertical" initialValue={0.4} />
      <MockedSlider paramId={EParams.kParamGain} label="Master" orientation="vertical" initialValue={0.9} />
    </div>
  ),
};
