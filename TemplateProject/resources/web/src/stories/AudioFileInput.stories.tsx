import type { Meta, StoryObj } from '@storybook/react';
import { MockedAudioFileInput } from '@/stories/mocks/MockedAudioFileInput';

const meta = {
  title: 'Inputs/AudioFileInput',
  component: MockedAudioFileInput,
  parameters: {
    layout: 'centered',
  },
  tags: ['autodocs'],
  argTypes: {
    initialHasFile: {
      control: 'boolean',
      description: 'Whether a file is loaded',
    },
    initialIsPlaying: {
      control: 'boolean',
      description: 'Whether audio is playing',
    },
    initialFileName: {
      control: 'text',
      description: 'Name of the loaded file',
    },
  },
} satisfies Meta<typeof MockedAudioFileInput>;

export default meta;
type Story = StoryObj<typeof meta>;

// No file selected
export const Empty: Story = {
  args: {
    initialHasFile: false,
    initialIsPlaying: false,
    initialFileName: '',
  },
};

// File selected, ready to play
export const FileLoaded: Story = {
  args: {
    initialHasFile: true,
    initialIsPlaying: false,
    initialFileName: 'drum_loop_120bpm.wav',
  },
};

// Playing
export const Playing: Story = {
  args: {
    initialHasFile: true,
    initialIsPlaying: true,
    initialFileName: 'synth_pad_ambient.mp3',
  },
};

// Long filename
export const LongFileName: Story = {
  args: {
    initialHasFile: true,
    initialIsPlaying: false,
    initialFileName: 'very_long_filename_that_should_be_truncated_in_the_display.wav',
  },
};

// In context (audio input section)
export const InAudioSection: Story = {
  render: () => (
    <div className="p-4 bg-black/40 rounded-xl border border-cyan-900/40">
      <h3 className="text-cyan-300 text-xs font-bold uppercase tracking-wider mb-3">Audio Input</h3>
      <MockedAudioFileInput 
        initialHasFile={true}
        initialFileName="sample.wav"
      />
    </div>
  ),
};

// With callbacks
export const WithCallbacks: Story = {
  render: () => (
    <div className="flex flex-col gap-2">
      <MockedAudioFileInput
        onFileSelected={(file) => console.log('File selected:', file.name)}
        onPlay={() => console.log('Play clicked')}
        onStop={() => console.log('Stop clicked')}
      />
      <p className="text-cyan-300/60 text-xs text-center">Open console to see events</p>
    </div>
  ),
};

// Multiple inputs
export const MultipleInputs: Story = {
  render: () => (
    <div className="flex flex-col gap-4 p-4 bg-black/40 rounded-xl border border-cyan-900/40">
      <div>
        <h4 className="text-cyan-200 text-[10px] font-bold uppercase tracking-wider mb-2">Input A</h4>
        <MockedAudioFileInput initialHasFile={true} initialFileName="kick.wav" />
      </div>
      <div>
        <h4 className="text-cyan-200 text-[10px] font-bold uppercase tracking-wider mb-2">Input B</h4>
        <MockedAudioFileInput initialHasFile={true} initialFileName="snare.wav" />
      </div>
      <div>
        <h4 className="text-cyan-200 text-[10px] font-bold uppercase tracking-wider mb-2">Input C</h4>
        <MockedAudioFileInput />
      </div>
    </div>
  ),
};

