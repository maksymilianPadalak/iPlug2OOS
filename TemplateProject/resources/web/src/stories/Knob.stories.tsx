import type { Meta, StoryObj } from '@storybook/react';
import { MockedKnob } from '@/stories/mocks/MockedKnob';
import { EParams } from '@/config/runtimeParameters';

const meta = {
  title: 'Controls/Knob',
  component: MockedKnob,
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
      description: 'Label displayed above the knob',
    },
    initialValue: {
      control: { type: 'range', min: 0, max: 1, step: 0.01 },
      description: 'Initial value (0-1)',
    },
  },
} satisfies Meta<typeof MockedKnob>;

export default meta;
type Story = StoryObj<typeof meta>;

export const Default: Story = {
  args: {
    paramId: EParams.kParamGain,
    label: 'Gain',
    initialValue: 0.75,
  },
};

export const NoLabel: Story = {
  args: {
    paramId: EParams.kParamGain,
    initialValue: 0.5,
  },
};

export const MultipleKnobs: Story = {
  args: { paramId: 0 },
  render: () => (
    <div className="flex items-end gap-6 p-4 bg-black/40 rounded-xl border border-orange-900/40">
      <MockedKnob paramId={EParams.kParamGain} label="Volume" initialValue={0.8} />
      <MockedKnob paramId={EParams.kParamGain} label="Pan" initialValue={0.5} />
      <MockedKnob paramId={EParams.kParamGain} label="Width" initialValue={0.6} />
    </div>
  ),
};
