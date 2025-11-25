import type { Meta, StoryObj } from '@storybook/react';
import { MockedKnob } from './mocks/MockedKnob';
import { EParams } from '../config/runtimeParameters';

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
    size: {
      control: { type: 'select' },
      options: ['sm', 'md', 'lg'],
      description: 'Size variant',
    },
    initialValue: {
      control: { type: 'range', min: 0, max: 1, step: 0.01 },
      description: 'Initial value (0-1)',
    },
  },
} satisfies Meta<typeof MockedKnob>;

export default meta;
type Story = StoryObj<typeof meta>;

// Size variants
export const Small: Story = {
  args: {
    paramId: EParams.kParamGain,
    label: 'Volume',
    size: 'sm',
    initialValue: 0.75,
  },
};

export const Medium: Story = {
  args: {
    paramId: EParams.kParamGain,
    label: 'Gain',
    size: 'md',
    initialValue: 0.5,
  },
};

export const Large: Story = {
  args: {
    paramId: EParams.kParamGain,
    label: 'Master',
    size: 'lg',
    initialValue: 0.8,
  },
};

// Without label
export const NoLabel: Story = {
  args: {
    paramId: EParams.kParamGain,
    size: 'md',
    initialValue: 0.5,
  },
};

// All sizes side by side
export const AllSizes: Story = {
  args: { paramId: 0 },
  render: () => (
    <div className="flex items-end gap-8">
      <MockedKnob paramId={EParams.kParamGain} label="Small" size="sm" initialValue={0.3} />
      <MockedKnob paramId={EParams.kParamGain} label="Medium" size="md" initialValue={0.5} />
      <MockedKnob paramId={EParams.kParamGain} label="Large" size="lg" initialValue={0.7} />
    </div>
  ),
};

// Example use cases
export const MasterSection: Story = {
  args: { paramId: 0 },
  render: () => (
    <div className="flex items-end gap-6 p-4 bg-black/40 rounded-xl border border-orange-900/40">
      <MockedKnob paramId={EParams.kParamGain} label="Volume" size="lg" initialValue={0.8} />
      <MockedKnob paramId={EParams.kParamGain} label="Pan" size="md" initialValue={0.5} />
      <MockedKnob paramId={EParams.kParamGain} label="Width" size="sm" initialValue={0.6} />
    </div>
  ),
};
