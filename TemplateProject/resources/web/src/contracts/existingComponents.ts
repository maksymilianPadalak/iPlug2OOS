/**
 * Existing Components Reference
 * 
 * This file documents components that ALREADY EXIST in the codebase.
 * AI can use these, modify them, or create entirely new ones.
 * 
 * THIS IS REFERENCE ONLY - NOT A CONSTRAINT.
 * AI has full creative freedom to build any UI.
 */

/**
 * Components that already exist and can be imported
 */
export const existingComponents = {
  controls: {
    Knob: {
      path: "components/controls/Knob",
      description: "Rotary control with drag interaction",
      props: "paramId: number, label?: string, size?: 'sm' | 'md' | 'lg'",
    },
    Slider: {
      path: "components/controls/Slider",
      description: "Linear slider control",
      props: "paramId: number, label?: string, orientation?: 'horizontal' | 'vertical'",
    },
    Toggle: {
      path: "components/controls/Toggle",
      description: "On/off switch",
      props: "paramId: number, label?: string",
    },
    Dropdown: {
      path: "components/controls/Dropdown",
      description: "Selection dropdown",
      props: "paramId: number, label?: string, options: string[]",
    },
  },

  layouts: {
    Section: {
      path: "components/layouts/Section",
      description: "Container with title/description",
      props: "title: string, description?: string, children: ReactNode",
    },
    TabContainer: {
      path: "components/layouts/Tabs",
      description: "Tabbed interface",
      props: "tabs: string[], activeTab: number, onTabChange: (n) => void, children",
    },
  },

  visualizations: {
    Meter: {
      path: "components/visualizations/Meter",
      description: "Audio level meter",
      props: "channel: 0 | 1, compact?: boolean",
    },
    PianoKeyboard: {
      path: "components/visualizations/PianoKeyboard",
      description: "Interactive MIDI keyboard",
      props: "none - self-contained",
    },
    LFOWaveform: {
      path: "components/visualizations/LFOWaveform",
      description: "LFO waveform display",
      props: "none - reads from context",
    },
  },

  sections: {
    PluginHeader: {
      path: "components/sections/PluginHeader",
      description: "Plugin name and version",
      props: "title?: string, version?: string",
    },
    KeyboardSection: {
      path: "components/sections/KeyboardSection",
      description: "Styled keyboard wrapper",
      props: "none",
    },
  },
};

/**
 * Quick reference - just the import paths
 */
export const componentImports = {
  Knob: "import { Knob } from '@/contracts/controls/Knob'",
  Slider: "import { Slider } from '@/contracts/controls/Slider'",
  Toggle: "import { Toggle } from '@/contracts/controls/Toggle'",
  Dropdown: "import { Dropdown } from '@/contracts/controls/Dropdown'",
  Section: "import { Section } from '@/contracts/layouts/Section'",
  TabContainer: "import { TabContainer } from '@/contracts/layouts/Tabs'",
  Meter: "import { Meter } from '@/contracts/visualizations/Meter'",
  PianoKeyboard: "import { PianoKeyboard } from '@/contracts/visualizations/PianoKeyboard'",
  LFOWaveform: "import { LFOWaveform } from '@/contracts/visualizations/LFOWaveform'",
  PluginHeader: "import { PluginHeader } from '@/contracts/sections/PluginHeader'",
  KeyboardSection: "import { KeyboardSection } from '@/contracts/sections/KeyboardSection'",
};

