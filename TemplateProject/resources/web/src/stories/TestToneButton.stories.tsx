import type { Meta, StoryObj } from '@storybook/react';
import { MockedTestToneButton, MockedCompactMicButton } from './mocks/MockedTestToneButton';

const meta = {
  title: 'Inputs/TestToneButton',
  component: MockedTestToneButton,
  parameters: {
    layout: 'centered',
  },
  tags: ['autodocs'],
  argTypes: {
    initialIsPlaying: {
      control: 'boolean',
      description: 'Initial microphone state',
    },
    label: {
      control: 'text',
      description: 'Button label',
    },
  },
} satisfies Meta<typeof MockedTestToneButton>;

export default meta;
type Story = StoryObj<typeof meta>;

// Microphone off
export const Off: Story = {
  args: {
    initialIsPlaying: false,
    label: 'Microphone Input',
  },
};

// Microphone on
export const On: Story = {
  args: {
    initialIsPlaying: true,
    label: 'Microphone Input',
  },
};

// Custom label
export const CustomLabel: Story = {
  args: {
    initialIsPlaying: false,
    label: 'Test Tone',
  },
};

// Compact variant off
export const CompactOff: Story = {
  render: () => <MockedCompactMicButton initialIsPlaying={false} />,
};

// Compact variant on
export const CompactOn: Story = {
  render: () => <MockedCompactMicButton initialIsPlaying={true} />,
};

// With callback
export const WithCallback: Story = {
  render: () => (
    <div className="flex flex-col gap-2">
      <MockedTestToneButton 
        onToggle={(isPlaying) => console.log('Mic state:', isPlaying)} 
      />
      <p className="text-cyan-300/60 text-xs text-center">Open console to see events</p>
    </div>
  ),
};

// In input section
export const InInputSection: Story = {
  render: () => (
    <div className="p-6 bg-gradient-to-br from-stone-900 to-black border border-cyan-900/40 rounded-xl">
      <MockedTestToneButton label="Live Input" />
    </div>
  ),
};

// Multiple input sources
export const MultipleInputSources: Story = {
  render: () => (
    <div className="flex gap-4 p-4 bg-black/40 rounded-xl border border-cyan-900/40">
      <div className="flex flex-col items-center gap-2">
        <MockedCompactMicButton initialIsPlaying={true} />
        <span className="text-cyan-300/60 text-[10px] uppercase">Mic 1</span>
      </div>
      <div className="flex flex-col items-center gap-2">
        <MockedCompactMicButton initialIsPlaying={false} />
        <span className="text-cyan-300/60 text-[10px] uppercase">Mic 2</span>
      </div>
      <div className="flex flex-col items-center gap-2">
        <MockedCompactMicButton initialIsPlaying={false} />
        <span className="text-cyan-300/60 text-[10px] uppercase">Line In</span>
      </div>
    </div>
  ),
};

// Side by side comparison
export const Comparison: Story = {
  render: () => (
    <div className="flex gap-8">
      <div className="flex flex-col items-center gap-2">
        <span className="text-orange-200 text-xs uppercase tracking-wider">Off State</span>
        <MockedTestToneButton initialIsPlaying={false} />
      </div>
      <div className="flex flex-col items-center gap-2">
        <span className="text-orange-200 text-xs uppercase tracking-wider">On State</span>
        <MockedTestToneButton initialIsPlaying={true} />
      </div>
    </div>
  ),
};

