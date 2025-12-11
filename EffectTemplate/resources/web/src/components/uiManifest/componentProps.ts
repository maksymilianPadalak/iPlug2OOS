/**
 * Component Props - TypeScript types for React components
 *
 * These are the actual prop types used by React components at runtime.
 * They use NUMBER values (paramId: number) because that's what the DSP bridge expects.
 *
 * Compare with planSchemas.ts which uses STRING keys for LLM output:
 *   planSchemas: paramId: "kParamGain" (string for LLM)
 *   componentProps: paramId: 0 (number for React/DSP)
 *
 * Types are derived from Zod schemas using z.infer<> for single source of truth.
 * React components import types from here (e.g., KnobProps, MeterProps).
 */

import { z } from 'zod';

export const KnobPropsSchema = z.object({
  paramId: z.number(),
  label: z.string().optional(),
});
export type KnobProps = z.infer<typeof KnobPropsSchema>;

export const DropdownPropsSchema = z.object({
  paramId: z.number(),
  label: z.string().optional(),
});
export type DropdownProps = z.infer<typeof DropdownPropsSchema>;

export const MeterPropsSchema = z.object({
  channel: z.union([z.literal(0), z.literal(1)]),
  compact: z.boolean().optional(),
});
export type MeterProps = z.infer<typeof MeterPropsSchema>;

export const WaveformDisplayPropsSchema = z.object({
  ctrlTag: z.number(),
  label: z.string().optional(),
});
export type WaveformDisplayProps = z.infer<typeof WaveformDisplayPropsSchema>;

export const ADSRDisplayPropsSchema = z.object({
  attackParam: z.number(),
  decayParam: z.number(),
  sustainParam: z.number(),
  releaseParam: z.number(),
  label: z.string().optional(),
});
export type ADSRDisplayProps = z.infer<typeof ADSRDisplayPropsSchema>;

export const SectionPropsSchema = z.object({
  title: z.string(),
  description: z.string().optional(),
  size: z.enum(['compact', 'wide', 'full']).optional(),
  variant: z.enum(['dark', 'light']).optional(),
  children: z.custom<React.ReactNode>(),
});
export type SectionProps = z.infer<typeof SectionPropsSchema>;

export const SubGroupPropsSchema = z.object({
  title: z.string().optional(),
  layout: z.enum(['row', 'grid-2', 'grid-3', 'grid-4']).optional(),
  children: z.custom<React.ReactNode>(),
});
export type SubGroupProps = z.infer<typeof SubGroupPropsSchema>;
