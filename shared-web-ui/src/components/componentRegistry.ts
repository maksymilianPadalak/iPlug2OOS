/**
 * Component Registry - Single source of truth for available UI components
 *
 * Defines all components the LLM can use when generating plugin UIs:
 * - controls: Knob, Dropdown, XYPad (user-interactive parameter controls)
 * - visualizations: Meter, FuturisticMeter, WaveformDisplay (DSP data displays)
 * - displays: ADSRDisplay (parameter-driven visualizations)
 * - layouts: Section, SubGroup (structural containers)
 *
 * Each component includes:
 * - path: import path for the React component
 * - propsSchema: Zod schema from planSchemas.ts (for LLM output validation)
 * - description: usage guidance shown to the LLM
 * - senderTypes: (visualizations only) which DSP sender types provide data
 *
 * Used by server-side tools (buildSchemasFromManifest, buildComponentDescriptions).
 */

import { getSenderTypesFor } from './getSenderTypesFor';
import {
  KnobPlanSchema,
  DropdownPlanSchema,
  XYPadPlanSchema,
  MeterPlanSchema,
  FuturisticMeterPlanSchema,
  WaveformDisplayPlanSchema,
  ADSRDisplayPlanSchema,
  SectionPlanSchema,
  SubGroupPlanSchema,
} from './planSchemas';

export const controls = {
  Knob: {
    path: 'sharedUi/components/Knob',
    propsSchema: KnobPlanSchema,
    description: 'Rotary control for continuous parameters.',
  },
  Dropdown: {
    path: 'sharedUi/components/Dropdown',
    propsSchema: DropdownPlanSchema,
    description: 'Selection control for enum parameters.',
  },
  XYPad: {
    path: 'sharedUi/components/XYPad',
    propsSchema: XYPadPlanSchema,
    description: '2D controller for two related parameters (e.g., delay time/feedback, filter cutoff/resonance). Use when user mentions "xy pad" or two parameters commonly adjusted together.',
  },
} as const;

export const visualizations = {
  Meter: {
    path: 'sharedUi/components/Meter',
    senderTypes: getSenderTypesFor('Meter'),
    propsSchema: MeterPlanSchema,
    description: 'Audio level meter with peak/RMS display.',
  },
  FuturisticMeter: {
    path: 'sharedUi/components/FuturisticMeter',
    senderTypes: getSenderTypesFor('Meter'),
    propsSchema: FuturisticMeterPlanSchema,
    description: 'Cyberpunk-style meter with glow effects. Supports color themes.',
  },
  WaveformDisplay: {
    path: 'sharedUi/components/WaveformDisplay',
    senderTypes: getSenderTypesFor('WaveformDisplay'),
    propsSchema: WaveformDisplayPlanSchema,
    description:
      'Oscilloscope-style waveform display. Auto-spans full width in grid layouts (col-span-full).',
  },
} as const;

export const displays = {
  ADSRDisplay: {
    path: 'sharedUi/components/ADSRDisplay',
    propsSchema: ADSRDisplayPlanSchema,
    description:
      'Visualizes ADSR envelope curve. Use when 4 ADSR params exist (Attack/Decay/Sustain/Release pattern in names). Place in grid-4 SubGroup with ADSR knobs.',
  },
} as const;

export const layouts = {
  Section: {
    path: 'sharedUi/components/Section',
    propsSchema: SectionPlanSchema,
    description: 'Groups related controls. Sizes: compact=1col, wide=2cols, full=4cols.',
  },
  SubGroup: {
    path: 'sharedUi/components/SubGroup',
    propsSchema: SubGroupPlanSchema,
    description:
      'Groups controls within Section. Use grid-4 for ADSR, grid-2 for stereo pairs/meters, row for default.',
  },
} as const;
