import type { Meta, StoryObj } from '@storybook/react';
import { MockedSection } from './mocks/MockedSection';
import { MockedKnob } from './mocks/MockedKnob';
import { MockedSlider } from './mocks/MockedSlider';
import { MockedToggle } from './mocks/MockedToggle';
import { MockedDropdown } from './mocks/MockedDropdown';
import { EParams } from '../config/constants';

const meta = {
  title: 'Layout/Section',
  component: MockedSection,
  parameters: {
    layout: 'centered',
  },
  tags: ['autodocs'],
  argTypes: {
    title: {
      control: 'text',
      description: 'Section title',
    },
    description: {
      control: 'text',
      description: 'Optional description',
    },
  },
} satisfies Meta<typeof MockedSection>;

export default meta;
type Story = StoryObj<typeof meta>;

// Basic section
export const Basic: Story = {
  args: {
    title: 'Oscillator',
    description: 'Sound source',
    children: (
      <div className="flex gap-4">
        <MockedKnob paramId={EParams.kParamGain} label="Pitch" size="md" initialValue={0.5} />
        <MockedKnob paramId={EParams.kParamGain} label="Fine" size="sm" initialValue={0.5} />
      </div>
    ),
  },
};

// Without description
export const WithoutDescription: Story = {
  args: {
    title: 'Master',
    children: (
      <MockedKnob paramId={EParams.kParamGain} label="Gain" size="lg" initialValue={0.8} />
    ),
  },
};

// Filter section example
export const FilterSection: Story = {
  render: () => (
    <MockedSection title="Filter" description="Low pass filter">
      <MockedDropdown paramId={EParams.kParamGain} label="Type" options={['LP', 'HP', 'BP', 'Notch']} initialIndex={0} />
      <MockedKnob paramId={EParams.kParamGain} label="Cutoff" size="md" initialValue={0.7} />
      <MockedKnob paramId={EParams.kParamGain} label="Res" size="md" initialValue={0.3} />
      <MockedKnob paramId={EParams.kParamGain} label="Drive" size="sm" initialValue={0.2} />
    </MockedSection>
  ),
};

// Envelope section
export const EnvelopeSection: Story = {
  render: () => (
    <MockedSection title="Amp Envelope" description="ADSR">
      <MockedKnob paramId={EParams.kParamGain} label="Attack" size="sm" initialValue={0.1} />
      <MockedKnob paramId={EParams.kParamGain} label="Decay" size="sm" initialValue={0.3} />
      <MockedKnob paramId={EParams.kParamGain} label="Sustain" size="sm" initialValue={0.7} />
      <MockedKnob paramId={EParams.kParamGain} label="Release" size="sm" initialValue={0.4} />
    </MockedSection>
  ),
};

// Effects section with toggles
export const EffectsSection: Story = {
  render: () => (
    <MockedSection title="Effects">
      <div className="flex flex-col gap-4">
        <div className="flex items-center gap-4">
          <MockedToggle paramId={EParams.kParamGain} label="Reverb" initialValue={true} />
          <MockedKnob paramId={EParams.kParamGain} label="Mix" size="sm" initialValue={0.4} />
          <MockedKnob paramId={EParams.kParamGain} label="Size" size="sm" initialValue={0.6} />
        </div>
        <div className="flex items-center gap-4">
          <MockedToggle paramId={EParams.kParamGain} label="Delay" initialValue={false} />
          <MockedKnob paramId={EParams.kParamGain} label="Time" size="sm" initialValue={0.3} />
          <MockedKnob paramId={EParams.kParamGain} label="Feedback" size="sm" initialValue={0.5} />
        </div>
      </div>
    </MockedSection>
  ),
};

// Complete synth layout
export const CompleteSynthLayout: Story = {
  render: () => (
    <div className="flex flex-col gap-4 max-w-4xl">
      <MockedSection title="Oscillators" description="Sound sources">
        <div className="flex gap-6">
          <div className="flex flex-col items-center gap-2">
            <MockedDropdown paramId={EParams.kParamGain} label="Wave 1" options={['Saw', 'Square', 'Tri', 'Sine']} initialIndex={0} />
            <div className="flex gap-2">
              <MockedKnob paramId={EParams.kParamGain} label="Semi" size="sm" initialValue={0.5} />
              <MockedKnob paramId={EParams.kParamGain} label="Fine" size="sm" initialValue={0.5} />
            </div>
          </div>
          <div className="flex flex-col items-center gap-2">
            <MockedDropdown paramId={EParams.kParamGain} label="Wave 2" options={['Saw', 'Square', 'Tri', 'Sine']} initialIndex={1} />
            <div className="flex gap-2">
              <MockedKnob paramId={EParams.kParamGain} label="Semi" size="sm" initialValue={0.5} />
              <MockedKnob paramId={EParams.kParamGain} label="Fine" size="sm" initialValue={0.5} />
            </div>
          </div>
          <MockedKnob paramId={EParams.kParamGain} label="Mix" size="md" initialValue={0.5} />
        </div>
      </MockedSection>
      
      <MockedSection title="Filter" description="Frequency shaping">
        <MockedDropdown paramId={EParams.kParamGain} label="Type" options={['LP', 'HP', 'BP']} initialIndex={0} />
        <MockedKnob paramId={EParams.kParamGain} label="Cutoff" size="lg" initialValue={0.6} />
        <MockedKnob paramId={EParams.kParamGain} label="Resonance" size="md" initialValue={0.3} />
        <MockedKnob paramId={EParams.kParamGain} label="Env Amt" size="sm" initialValue={0.5} />
      </MockedSection>
      
      <MockedSection title="Output">
        <MockedKnob paramId={EParams.kParamGain} label="Master" size="lg" initialValue={0.8} />
        <MockedSlider paramId={EParams.kParamGain} label="Pan" orientation="horizontal" initialValue={0.5} />
      </MockedSection>
    </div>
  ),
};
