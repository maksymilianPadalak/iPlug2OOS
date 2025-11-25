import type { Meta, StoryObj } from '@storybook/react';
import { MockedDropdown } from './mocks/MockedDropdown';
import { EParams } from '../config/runtimeParameters';

const meta = {
  title: 'Controls/Dropdown',
  component: MockedDropdown,
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
      description: 'Label displayed above the dropdown',
    },
    options: {
      control: 'object',
      description: 'Array of option strings',
    },
    initialIndex: {
      control: { type: 'number' },
      description: 'Initial selected index',
    },
  },
} satisfies Meta<typeof MockedDropdown>;

export default meta;
type Story = StoryObj<typeof meta>;

// Waveform selector
export const Waveform: Story = {
  args: {
    paramId: EParams.kParamGain,
    label: 'Waveform',
    options: ['Sine', 'Triangle', 'Saw', 'Square', 'Noise'],
    initialIndex: 2,
  },
};

// Filter type
export const FilterType: Story = {
  args: {
    paramId: EParams.kParamGain,
    label: 'Filter',
    options: ['Low Pass', 'High Pass', 'Band Pass', 'Notch'],
    initialIndex: 0,
  },
};

// Algorithm selector
export const Algorithm: Story = {
  args: {
    paramId: EParams.kParamGain,
    label: 'Algorithm',
    options: ['Algorithm 1', 'Algorithm 2', 'Algorithm 3', 'Algorithm 4'],
    initialIndex: 0,
  },
};

// Without label
export const NoLabel: Story = {
  args: {
    paramId: EParams.kParamGain,
    options: ['Option A', 'Option B', 'Option C'],
    initialIndex: 1,
  },
};

// Multiple dropdowns
export const OscillatorSection: Story = {
  args: { paramId: 0, options: [] },
  render: () => (
    <div className="flex gap-4 p-4 bg-black/40 rounded-xl border border-orange-900/40">
      <MockedDropdown 
        paramId={EParams.kParamGain} 
        label="Osc 1" 
        options={['Sine', 'Saw', 'Square', 'Tri']} 
        initialIndex={1}
      />
      <MockedDropdown 
        paramId={EParams.kParamGain} 
        label="Osc 2" 
        options={['Sine', 'Saw', 'Square', 'Tri']} 
        initialIndex={0}
      />
      <MockedDropdown 
        paramId={EParams.kParamGain} 
        label="Sub" 
        options={['Off', 'Square', 'Sine']} 
        initialIndex={0}
      />
    </div>
  ),
};
