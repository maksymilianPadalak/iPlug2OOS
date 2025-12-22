/**
 * Plan Schemas - Zod schemas for LLM structured output
 *
 * These schemas define the shape of data the LLM outputs when generating UI plans.
 * They use STRING keys (e.g., "kParamGain") because LLMs work with text.
 *
 * The generateUiCode step transforms these strings to actual enum references:
 *   "kParamGain" -> EParams.kParamGain
 *
 * Compare with componentProps.ts which uses NUMBER values for actual React components.
 */

import { z } from 'zod';

export const KnobPlanSchema = z.object({
  paramId: z.string().describe('EParams key like kParamGain'),
  label: z.string().nullable().describe('Optional label'),
  color: z.enum(['cyan', 'magenta', 'green', 'orange']).nullable().describe('Knob color theme'),
  size: z.enum(['small', 'medium']).nullable().describe('Knob size - use "small" inside TabbedPanel tabs'),
});

export const DropdownPlanSchema = z.object({
  paramId: z.string().describe('EParams key'),
  label: z.string().nullable().describe('Optional label'),
});

export const XYPadPlanSchema = z.object({
  paramIdX: z.string().describe('EParams key for X axis'),
  paramIdY: z.string().describe('EParams key for Y axis'),
  labelX: z.string().nullable().describe('Optional X axis label'),
  labelY: z.string().nullable().describe('Optional Y axis label'),
});

export const MeterPlanSchema = z.object({
  channel: z.number().describe('0=left, 1=right'),
  label: z.string().nullable().describe('Optional label'),
  color: z.enum(['cyan', 'magenta', 'green', 'orange']).nullable().describe('Meter color theme'),
});

export const WaveformDisplayPlanSchema = z.object({
  ctrlTag: z.string().describe('EControlTags key'),
  label: z.string().nullable().describe('Optional label'),
});

export const ADSRDisplayPlanSchema = z.object({
  attackParam: z.string().describe('EParams key for attack'),
  decayParam: z.string().describe('EParams key for decay'),
  sustainParam: z.string().describe('EParams key for sustain'),
  releaseParam: z.string().describe('EParams key for release'),
  label: z.string().nullable().describe('Optional label'),
  height: z.number().nullable().describe('Display height in pixels (default 120, use reduced height inside tabs)'),
});

export const WaveSelectorPlanSchema = z.object({
  paramId: z.string().describe('EParams key for waveform selection (enum parameter)'),
  label: z.string().nullable().describe('Optional label'),
  height: z.number().nullable().describe('Display height in pixels (default 120, use reduced height inside tabs)'),
});

// =============================================================================
// Layout Schemas
// =============================================================================

export const SectionPlanSchema = z.object({
  title: z.string().describe('Section title'),
  description: z.string().nullable().describe('Optional description'),
  size: z.enum(['compact', 'wide', 'full']).nullable().describe('Section width'),
  variant: z.enum(['dark', 'light']).nullable().describe('Visual variant'),
});

export const SubGroupPlanSchema = z.object({
  title: z.string().nullable().describe('Optional group title'),
  layout: z.enum(['row', 'grid-2', 'grid-3', 'grid-4']).nullable().describe('Layout mode'),
});


// TabbedPanel props - children added to each tab in schemas.ts
export const TabbedPanelPlanSchema = z.object({
  tabs: z.array(z.object({
    title: z.string().describe('Tab title like "OSC 1" or "LFO 1"'),
    color: z.enum(['cyan', 'magenta', 'green', 'orange', 'purple', 'yellow']).describe('Tab LED color - EACH TAB MUST HAVE A DIFFERENT COLOR (e.g., OSC1=cyan, OSC2=purple, Filter=green)'),
  })),
  defaultTab: z.number().nullable().describe('Default active tab index (0-based)'),
});

export const VoiceCardPlanSchema = z.object({
  title: z.string().describe('Voice name'),
  color: z.enum(['cyan', 'magenta', 'green', 'orange']).nullable().describe('Card color theme'),
  triggerId: z.number().nullable().describe('MIDI note number for trigger pulse animation'),
  icon: z.enum(['decay', 'burst', 'shimmer', 'resonant', 'staccato', 'spike', 'sine', 'saw', 'square', 'noise']).nullable().describe('Waveform icon: decay=exponential falloff, burst=noise attack, shimmer=high-freq oscillation, resonant=pitched ring, staccato=multiple hits, spike=sharp transient, sine/saw/square/noise=standard waves'),
});
