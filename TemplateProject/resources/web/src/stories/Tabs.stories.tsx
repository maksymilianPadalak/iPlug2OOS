import type { Meta, StoryObj } from '@storybook/react';
import { MockedTabContainer, MockedTab } from '@/stories/mocks/MockedTabs';
import { MockedKnob } from '@/stories/mocks/MockedKnob';
import { MockedSlider } from '@/stories/mocks/MockedSlider';
import { EParams } from '@/config/runtimeParameters';

const meta = {
  title: 'Layout/Tabs',
  component: MockedTabContainer,
  parameters: {
    layout: 'centered',
  },
  tags: ['autodocs'],
  argTypes: {
    tabs: {
      control: 'object',
      description: 'Array of tab labels',
    },
    initialTab: {
      control: { type: 'number' },
      description: 'Initial active tab index',
    },
  },
} satisfies Meta<typeof MockedTabContainer>;

export default meta;
type Story = StoryObj<typeof meta>;

// Basic tabs
export const Basic: Story = {
  args: {
    tabs: ['Tab 1', 'Tab 2', 'Tab 3'],
    initialTab: 0,
    children: <div className="text-white text-sm">Tab content goes here</div>,
  },
};

// Oscillator tabs
export const OscillatorTabs: Story = {
  args: { tabs: ['OSC 1', 'OSC 2', 'SUB'] },
  render: () => (
    <MockedTabContainer tabs={['OSC 1', 'OSC 2', 'SUB']}>
      {(activeTab) => (
        <div className="flex gap-4">
          <MockedKnob paramId={EParams.kParamGain} label="Pitch" size="sm" initialValue={0.5} />
          <MockedKnob paramId={EParams.kParamGain} label="Fine" size="sm" initialValue={0.5} />
          <MockedKnob paramId={EParams.kParamGain} label="Level" size="sm" initialValue={0.7 - activeTab * 0.1} />
        </div>
      )}
    </MockedTabContainer>
  ),
};

// Effect tabs
export const EffectTabs: Story = {
  args: { tabs: ['REVERB', 'DELAY', 'CHORUS'] },
  render: () => (
    <MockedTabContainer tabs={['REVERB', 'DELAY', 'CHORUS']} initialTab={1}>
      {(activeTab) => (
        <div className="flex gap-4">
          {activeTab === 0 && (
            <>
              <MockedKnob paramId={EParams.kParamGain} label="Size" size="sm" initialValue={0.6} />
              <MockedKnob paramId={EParams.kParamGain} label="Decay" size="sm" initialValue={0.4} />
              <MockedKnob paramId={EParams.kParamGain} label="Mix" size="sm" initialValue={0.3} />
            </>
          )}
          {activeTab === 1 && (
            <>
              <MockedKnob paramId={EParams.kParamGain} label="Time" size="sm" initialValue={0.5} />
              <MockedKnob paramId={EParams.kParamGain} label="Feedback" size="sm" initialValue={0.4} />
              <MockedKnob paramId={EParams.kParamGain} label="Mix" size="sm" initialValue={0.25} />
            </>
          )}
          {activeTab === 2 && (
            <>
              <MockedKnob paramId={EParams.kParamGain} label="Rate" size="sm" initialValue={0.3} />
              <MockedKnob paramId={EParams.kParamGain} label="Depth" size="sm" initialValue={0.5} />
              <MockedKnob paramId={EParams.kParamGain} label="Mix" size="sm" initialValue={0.4} />
            </>
          )}
        </div>
      )}
    </MockedTabContainer>
  ),
};

// Single tab (no tabs)
export const SingleTab: Story = {
  args: {
    tabs: ['MASTER'],
    initialTab: 0,
    children: (
      <div className="flex gap-4 items-center">
        <MockedKnob paramId={EParams.kParamGain} label="Volume" size="lg" initialValue={0.8} />
        <MockedSlider paramId={EParams.kParamGain} label="Pan" orientation="horizontal" initialValue={0.5} />
      </div>
    ),
  },
};

// Many tabs
export const ManyTabs: Story = {
  args: {
    tabs: ['A', 'B', 'C', 'D', 'E', 'F'],
    initialTab: 2,
    children: <div className="text-amber-200 text-xs uppercase tracking-wider">Select a tab</div>,
  },
};

// Individual Tab component
export const IndividualTab: Story = {
  args: { tabs: ['Tab'] },
  render: () => (
    <div className="flex gap-2">
      <MockedTab label="Active" active={true} onClick={() => {}} />
      <MockedTab label="Inactive" active={false} onClick={() => {}} />
      <MockedTab label="Hover me" active={false} onClick={() => {}} />
    </div>
  ),
};

