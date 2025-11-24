import type { Meta, StoryObj } from '@storybook/react';
import { MockedToggle } from './mocks/MockedToggle';
import { EParams } from '../config/constants';

const meta = {
  title: 'Controls/Toggle',
  component: MockedToggle,
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
      description: 'Label displayed above the toggle',
    },
    initialValue: {
      control: 'boolean',
      description: 'Initial on/off state',
    },
  },
} satisfies Meta<typeof MockedToggle>;

export default meta;
type Story = StoryObj<typeof meta>;

// Basic toggle
export const Default: Story = {
  args: {
    paramId: EParams.kParamGain,
    label: 'Bypass',
    initialValue: false,
  },
};

// Enabled state
export const Enabled: Story = {
  args: {
    paramId: EParams.kParamGain,
    label: 'Active',
    initialValue: true,
  },
};

// Without label
export const NoLabel: Story = {
  args: {
    paramId: EParams.kParamGain,
    initialValue: false,
  },
};

// Common use cases
export const EffectToggles: Story = {
  render: () => (
    <div className="flex flex-col gap-4 p-4 bg-black/40 rounded-xl border border-orange-900/40">
      <MockedToggle paramId={EParams.kParamGain} label="Reverb" initialValue={true} />
      <MockedToggle paramId={EParams.kParamGain} label="Delay" initialValue={false} />
      <MockedToggle paramId={EParams.kParamGain} label="Chorus" initialValue={true} />
      <MockedToggle paramId={EParams.kParamGain} label="Bypass" initialValue={false} />
    </div>
  ),
};

// Inline toggles
export const InlineToggles: Story = {
  render: () => (
    <div className="flex gap-6 p-4 bg-black/40 rounded-xl border border-orange-900/40">
      <MockedToggle paramId={EParams.kParamGain} label="Mono" initialValue={false} />
      <MockedToggle paramId={EParams.kParamGain} label="Phase" initialValue={true} />
      <MockedToggle paramId={EParams.kParamGain} label="HP" initialValue={false} />
    </div>
  ),
};
