import type { Meta, StoryObj } from '@storybook/react';
import { MockedWebControls, MockedAudioStatus } from '@/stories/mocks/MockedWebControls';

const meta = {
  title: 'Sections/WebControls',
  component: MockedWebControls,
  parameters: {
    layout: 'centered',
  },
  tags: ['autodocs'],
  argTypes: {
    audioStatus: {
      control: { type: 'select' },
      options: ['working', 'not-working', null],
      description: 'Audio engine status',
    },
    showMidiControls: {
      control: 'boolean',
      description: 'Show MIDI input/output selectors',
    },
    midiInputOptions: {
      control: 'object',
      description: 'Available MIDI input devices',
    },
    midiOutputOptions: {
      control: 'object',
      description: 'Available MIDI output devices',
    },
  },
} satisfies Meta<typeof MockedWebControls>;

export default meta;
type Story = StoryObj<typeof meta>;

// Default with no audio status
export const Default: Story = {
  args: {
    audioStatus: null,
    showMidiControls: true,
    midiInputOptions: ['MIDI INPUT'],
    midiOutputOptions: ['MIDI OUTPUT'],
  },
};

// Audio working
export const AudioWorking: Story = {
  args: {
    audioStatus: 'working',
    showMidiControls: true,
    midiInputOptions: ['MIDI INPUT'],
    midiOutputOptions: ['MIDI OUTPUT'],
  },
};

// Audio not working
export const AudioNotWorking: Story = {
  args: {
    audioStatus: 'not-working',
    showMidiControls: true,
    midiInputOptions: ['MIDI INPUT'],
    midiOutputOptions: ['MIDI OUTPUT'],
  },
};

// With MIDI devices
export const WithMidiDevices: Story = {
  args: {
    audioStatus: 'working',
    showMidiControls: true,
    midiInputOptions: ['No Device', 'MIDI Keyboard', 'USB MIDI Controller', 'Virtual MIDI'],
    midiOutputOptions: ['No Device', 'Software Synth', 'Hardware Synth'],
  },
};

// MIDI only (no audio status)
export const MidiOnly: Story = {
  args: {
    audioStatus: null,
    showMidiControls: true,
    midiInputOptions: ['USB MIDI', 'Bluetooth MIDI'],
    midiOutputOptions: ['Internal Synth'],
  },
};

// Audio status only (no MIDI)
export const AudioStatusOnly: Story = {
  args: {
    audioStatus: 'working',
    showMidiControls: false,
  },
};

// Compact audio status indicator
export const AudioStatusIndicator: Story = {
  render: () => (
    <div className="flex gap-4">
      <MockedAudioStatus status="working" />
      <MockedAudioStatus status="not-working" />
    </div>
  ),
};

// In plugin context
export const InPluginContext: Story = {
  render: () => (
    <div className="w-[600px] p-4 bg-gradient-to-br from-stone-950 to-black border border-orange-900/40 rounded-xl">
      <div className="text-orange-400 text-lg font-black uppercase tracking-tight mb-4">
        Web Audio Plugin
      </div>
      <MockedWebControls 
        audioStatus="working"
        showMidiControls={true}
        midiInputOptions={['No Device', 'USB MIDI Keyboard']}
        midiOutputOptions={['No Device']}
      />
      <div className="text-orange-200/40 text-xs mt-2">
        Plugin controls would go here...
      </div>
    </div>
  ),
};

// All states
export const AllStates: Story = {
  render: () => (
    <div className="flex flex-col gap-6 w-[500px]">
      <div className="p-4 bg-black/40 rounded-lg border border-orange-900/40">
        <h3 className="text-orange-300 text-xs font-bold uppercase tracking-wider mb-2">Audio Working</h3>
        <MockedWebControls audioStatus="working" />
      </div>
      <div className="p-4 bg-black/40 rounded-lg border border-orange-900/40">
        <h3 className="text-orange-300 text-xs font-bold uppercase tracking-wider mb-2">Audio Error</h3>
        <MockedWebControls audioStatus="not-working" />
      </div>
      <div className="p-4 bg-black/40 rounded-lg border border-orange-900/40">
        <h3 className="text-orange-300 text-xs font-bold uppercase tracking-wider mb-2">No Status</h3>
        <MockedWebControls audioStatus={null} />
      </div>
    </div>
  ),
};

// Status badges collection
export const StatusBadges: Story = {
  render: () => (
    <div className="flex flex-col gap-4">
      <div className="flex gap-4 items-center">
        <MockedAudioStatus status="working" />
        <span className="text-orange-200/60 text-xs">When audio context is active</span>
      </div>
      <div className="flex gap-4 items-center">
        <MockedAudioStatus status="not-working" />
        <span className="text-orange-200/60 text-xs">When audio context fails</span>
      </div>
    </div>
  ),
};

