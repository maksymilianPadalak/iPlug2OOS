/**
 * UI Manifest - Available components for plugin UI generation
 *
 * Used by: src/server/lib/langchain/tools/generateUiPlan/utils/manifestToXml
 * to generate XML context for the LLM system prompt.
 */

export const controls = {
  Knob: {
    path: '@/components/controls/Knob',
    props: {
      paramId: 'number - EParams enum value',
      label: 'string - optional display label',
    },
    description: 'Rotary control for continuous parameters. Drag up/down to change.',
  },
  Dropdown: {
    path: '@/components/controls/Dropdown',
    props: {
      paramId: 'number - EParams enum value',
      label: 'string - optional display label',
    },
    description: 'Selection control for enum parameters. Options are auto-read from runtimeParameters.',
  },
};

export const visualizations = {
  Meter: {
    path: '@/components/visualizations/Meter',
    props: {
      channel: '0 | 1 - left or right channel',
    },
    description: 'Audio level meter with peak/RMS display.',
  },
};

export const layouts = {
  Section: {
    path: '@/components/layouts/Section',
    props: {
      title: 'string - section header',
      description: 'string - optional subtitle',
      children: 'ReactNode - section content',
    },
    description: 'Container for grouping controls.',
  },
  Tabs: {
    path: '@/components/layouts/Tabs',
    props: {
      tabs: '{ id: string, label: string, content: ReactNode }[]',
    },
    description: 'Tabbed container for organizing multiple views.',
  },
};

export const hooks = {
  useParameter: {
    path: '@/glue/hooks/useParameter',
    signature: 'useParameter(paramIdx: number)',
    returns: '{ value, setValue, beginChange, endChange }',
    description: 'Connect control to DSP parameter with DAW automation.',
  },
  useMeter: {
    path: '@/glue/hooks/useMeter',
    signature: 'useMeter(channel: 0 | 1)',
    returns: '{ peak, rms }',
    description: 'Read audio level data.',
  },
  useMidi: {
    path: '@/glue/hooks/useMidi',
    signature: 'useMidi()',
    returns: '{ activeNotes, isNoteActive, getNoteVelocity }',
    description: 'Read MIDI note state.',
  },
  useArbitraryMessage: {
    path: '@/glue/hooks/useArbitraryMessage',
    signature: 'useArbitraryMessage(msgTag: number)',
    returns: '{ data: ArrayBuffer, timestamp: number } | null',
    description: 'Read raw binary data for custom visualizations (spectrum, oscilloscope).',
  },
};

export const requiredWrapper = {
  BridgeProvider: {
    path: '@/glue/BridgeProvider',
    description: 'Must wrap the entire app - initializes DSP communication.',
  },
};
