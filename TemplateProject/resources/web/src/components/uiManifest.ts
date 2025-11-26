/**
 * UI Manifest - Available building blocks for plugin UI generation
 *
 * This manifest describes what components, hooks, and layouts are available
 * for AI to use when generating plugin UIs.
 */

/**
 * Available control components
 */
export const controls = {
  Knob: {
    path: 'components/controls/Knob',
    props: {
      paramId: 'number - EParams enum value',
      label: 'string - display label',
      size: "'sm' | 'md' | 'lg' - optional, defaults to 'md'",
    },
    description: 'Rotary knob for continuous parameters (gain, frequency, etc.)',
  },
  Slider: {
    path: 'components/controls/Slider',
    props: {
      paramId: 'number - EParams enum value',
      label: 'string - display label',
    },
    description: 'Vertical slider for continuous parameters',
  },
  Dropdown: {
    path: 'components/controls/Dropdown',
    props: {
      paramId: 'number - EParams enum value',
      label: 'string - display label',
      options: 'string[] - list of option labels',
    },
    description: 'Dropdown selector for enum parameters (waveform, mode, etc.)',
  },
  Toggle: {
    path: 'components/controls/Toggle',
    props: {
      paramId: 'number - EParams enum value',
      label: 'string - display label',
    },
    description: 'On/off toggle for boolean parameters',
  },
};

/**
 * Available visualization components
 */
export const visualizations = {
  Meter: {
    path: 'components/visualizations/Meter',
    props: {
      channel: '0 | 1 - left or right channel',
    },
    description: 'Audio level meter with peak/RMS display',
  },
  PianoKeyboard: {
    path: 'components/visualizations/PianoKeyboard',
    props: {
      startNote: 'number - MIDI note number for first key',
      endNote: 'number - MIDI note number for last key',
    },
    description: 'Interactive piano keyboard with MIDI I/O',
  },
};

/**
 * Available layout components
 */
export const layouts = {
  Section: {
    path: 'components/layouts/Section',
    props: {
      title: 'string - section header',
      description: 'string - optional subtitle',
      children: 'ReactNode - section content',
    },
    description: 'Styled container for grouping controls',
  },
  Tabs: {
    path: 'components/layouts/Tabs',
    props: {
      tabs: '{ id: string, label: string, content: ReactNode }[]',
    },
    description: 'Tabbed container for organizing multiple views',
  },
};

/**
 * Available hooks
 */
export const hooks = {
  useParameter: {
    path: 'glue/hooks/useParameter',
    returns: '{ value: number, setValue: (v: number) => void, beginChange: () => void, endChange: () => void }',
    description: 'Connect any control to a parameter with DAW automation support',
  },
  useMeter: {
    path: 'glue/hooks/useMeter',
    returns: '{ peak: number, rms: number }',
    description: 'Read audio level data for a channel',
  },
  useMidi: {
    path: 'glue/hooks/useMidi',
    returns: '{ activeNotes: Map, isNoteActive: (note: number) => boolean, getNoteVelocity: (note: number) => number }',
    description: 'Read MIDI note state from DSP',
  },
  useArbitraryMessage: {
    path: 'glue/hooks/useArbitraryMessage',
    returns: '{ data: ArrayBuffer, timestamp: number } | null',
    description: 'Read raw binary data for custom visualizations (spectrum, oscilloscope, etc.)',
  },
};

/**
 * Required wrapper
 */
export const requiredWrapper = {
  BridgeProvider: {
    path: 'glue/BridgeProvider',
    description: 'Must wrap the entire app - initializes DSP communication',
  },
};
