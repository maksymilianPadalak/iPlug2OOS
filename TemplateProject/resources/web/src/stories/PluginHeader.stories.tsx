import type { Meta, StoryObj } from '@storybook/react';
import { MockedPluginHeader, MockedCompactHeader } from '@/stories/mocks/MockedPluginHeader';

const meta = {
  title: 'Sections/PluginHeader',
  component: MockedPluginHeader,
  parameters: {
    layout: 'centered',
  },
  tags: ['autodocs'],
  argTypes: {
    title: {
      control: 'text',
      description: 'Plugin name',
    },
    version: {
      control: 'text',
      description: 'Plugin version',
    },
    showVersion: {
      control: 'boolean',
      description: 'Show version badge',
    },
  },
} satisfies Meta<typeof MockedPluginHeader>;

export default meta;
type Story = StoryObj<typeof meta>;

// Default header
export const Default: Story = {
  args: {
    title: 'TemplateProject',
    version: '1.0',
    showVersion: true,
  },
};

// Synth plugin
export const SynthPlugin: Story = {
  args: {
    title: 'SYNTHWAVE',
    version: '2.1',
    showVersion: true,
  },
};

// Effect plugin
export const EffectPlugin: Story = {
  args: {
    title: 'REVERB PRO',
    version: '1.5.3',
    showVersion: true,
  },
};

// Without version
export const WithoutVersion: Story = {
  args: {
    title: 'MINIMAL',
    version: '1.0',
    showVersion: false,
  },
};

// Long title
export const LongTitle: Story = {
  args: {
    title: 'GRANULAR SYNTHESIS ENGINE',
    version: '0.9-beta',
    showVersion: true,
  },
};

// Compact header
export const Compact: Story = {
  render: () => <MockedCompactHeader title="SYNTH" subtitle="v1.0" />,
};

// Compact without subtitle
export const CompactNoSubtitle: Story = {
  render: () => <MockedCompactHeader title="MINI FX" />,
};

// In plugin container
export const InPluginContainer: Story = {
  render: () => (
    <div className="w-[600px] p-6 bg-gradient-to-br from-stone-950 to-black border border-orange-900/40 rounded-xl">
      <MockedPluginHeader title="POLYSYNTH" version="3.0" />
      <div className="mt-4 text-orange-200/40 text-xs">
        Plugin content would go here...
      </div>
    </div>
  ),
};

// Different plugin types showcase
export const PluginTypes: Story = {
  render: () => (
    <div className="flex flex-col gap-6 w-[500px]">
      <div className="p-4 bg-black/40 rounded-lg border border-orange-900/40">
        <MockedPluginHeader title="SYNTH-X" version="2.0" />
      </div>
      <div className="p-4 bg-black/40 rounded-lg border border-orange-900/40">
        <MockedPluginHeader title="DELAY FX" version="1.2" />
      </div>
      <div className="p-4 bg-black/40 rounded-lg border border-orange-900/40">
        <MockedPluginHeader title="COMPRESSOR" version="3.1" />
      </div>
    </div>
  ),
};

// Header variations
export const Variations: Story = {
  render: () => (
    <div className="flex flex-col gap-4 w-[400px]">
      <MockedPluginHeader title="FULL HEADER" version="1.0" showVersion={true} />
      <MockedPluginHeader title="NO VERSION" version="1.0" showVersion={false} />
      <MockedCompactHeader title="COMPACT" subtitle="mini" />
      <MockedCompactHeader title="COMPACT SOLO" />
    </div>
  ),
};

