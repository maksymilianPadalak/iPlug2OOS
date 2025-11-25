import type { Meta, StoryObj } from '@storybook/react';
import { MockedPianoKeyboard, MockedMiniKeyboard } from './mocks/MockedPianoKeyboard';

const meta = {
  title: 'Visualizations/PianoKeyboard',
  component: MockedPianoKeyboard,
  parameters: {
    layout: 'centered',
  },
  tags: ['autodocs'],
  argTypes: {
    octaves: {
      control: 'object',
      description: 'Array of octave numbers to display',
    },
    showOctaveLabels: {
      control: 'boolean',
      description: 'Show octave range labels',
    },
  },
} satisfies Meta<typeof MockedPianoKeyboard>;

export default meta;
type Story = StoryObj<typeof meta>;

// Default 3-octave keyboard
export const Default: Story = {
  args: {
    octaves: [3, 4, 5],
    showOctaveLabels: true,
  },
};

// Single octave
export const SingleOctave: Story = {
  args: {
    octaves: [4],
    showOctaveLabels: true,
  },
};

// Two octaves
export const TwoOctaves: Story = {
  args: {
    octaves: [4, 5],
    showOctaveLabels: true,
  },
};

// Without labels
export const WithoutLabels: Story = {
  args: {
    octaves: [4, 5],
    showOctaveLabels: false,
  },
};

// High octaves
export const HighOctaves: Story = {
  args: {
    octaves: [5, 6, 7],
    showOctaveLabels: true,
  },
};

// Low octaves
export const LowOctaves: Story = {
  args: {
    octaves: [1, 2, 3],
    showOctaveLabels: true,
  },
};

// Mini keyboard
export const MiniKeyboard: Story = {
  render: () => (
    <div className="flex flex-col gap-4">
      <div className="text-orange-200 text-xs uppercase tracking-wider">One Octave (C4-C5)</div>
      <MockedMiniKeyboard startNote={60} numKeys={13} />
    </div>
  ),
};

// In section context
export const InSection: Story = {
  render: () => (
    <div className="bg-gradient-to-b from-stone-800 to-stone-900 border border-orange-700/30 rounded-lg p-2 shadow-lg">
      <MockedPianoKeyboard octaves={[3, 4, 5]} showOctaveLabels={true} />
    </div>
  ),
};

// Interactive with callback logging
export const WithCallbacks: Story = {
  render: () => (
    <div className="flex flex-col gap-2">
      <MockedPianoKeyboard 
        octaves={[4]} 
        showOctaveLabels={true}
        onNoteOn={(note) => console.log(`Note ON: ${note}`)}
        onNoteOff={(note) => console.log(`Note OFF: ${note}`)}
      />
      <p className="text-orange-300/60 text-xs">Open console to see note events</p>
    </div>
  ),
};

// Full range (extreme)
export const FullRange: Story = {
  render: () => (
    <div className="overflow-x-auto">
      <MockedPianoKeyboard octaves={[2, 3, 4, 5, 6]} showOctaveLabels={true} />
    </div>
  ),
};

