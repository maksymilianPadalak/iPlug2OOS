/**
 * UI Manifest - Available components for plugin UI generation
 *
 * Used by: src/server/lib/langchain/tools/generateUiPlan/utils/manifestToXml
 * to generate XML context for the LLM system prompt.
 *
 * Note: senderTypes are derived from SENDER_TO_VISUALIZATION in senderTypeMapping.ts
 * which is the single source of truth for sender → visualization mapping.
 */

import { SENDER_TO_VISUALIZATION } from '../config/senderTypeMapping';
import type { SenderType, VisualizationComponent } from '../config/senderTypeMapping';

/** Get all sender types that map to a given visualization component */
function getSenderTypesFor(component: VisualizationComponent): SenderType[] {
  return (Object.entries(SENDER_TO_VISUALIZATION) as [SenderType, VisualizationComponent | null][])
    .filter(([, viz]) => viz === component)
    .map(([senderType]) => senderType);
}

export const controls = {
  Knob: {
    path: '@/components/controls/Knob',
    props: {
      paramId: 'EParams enum value',
    },
    description: 'Rotary control for continuous parameters.',
  },
  Dropdown: {
    path: '@/components/controls/Dropdown',
    props: {
      paramId: 'EParams enum value',
    },
    description: 'Selection control for enum parameters.',
  },
};

export const visualizations = {
  Meter: {
    path: '@/components/visualizations/Meter',
    senderTypes: getSenderTypesFor('Meter'),
    props: {
      channel: '0 | 1',
    },
    description: 'Audio level meter with peak/RMS display.',
  },
  WaveformDisplay: {
    path: '@/components/visualizations/WaveformDisplay',
    senderTypes: getSenderTypesFor('WaveformDisplay'),
    props: {
      ctrlTag: 'EControlTags key',
    },
    description: 'Oscilloscope-style waveform display.',
  },
};
