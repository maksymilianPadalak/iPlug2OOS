/**
 * Existing Components Reference
 *
 * Components available for plugin UI. AI can use these or create custom ones.
 */

export const existingComponents = {
  controls: {
    Knob: {
      import: "import { Knob } from '@/components/controls/Knob'",
      props: 'paramId: number, label?: string',
      description: 'Rotary control for continuous parameters',
    },
    Dropdown: {
      import: "import { Dropdown } from '@/components/controls/Dropdown'",
      props: 'paramId: number, label?: string, options: string[]',
      description: 'Selection control for enum parameters',
    },
  },

  visualizations: {
    Meter: {
      import: "import { Meter } from '@/components/visualizations/Meter'",
      props: 'channel: 0 | 1',
      description: 'Audio level meter',
    },
  },

  layouts: {
    Section: {
      import: "import { Section } from '@/components/layouts/Section'",
      props: 'title: string, description?: string, children: ReactNode',
      description: 'Container for grouping controls',
    },
  },
};
