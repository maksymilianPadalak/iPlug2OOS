/**
 * Component Props - Single source of truth for all component prop types
 *
 * All shared UI components import their prop types from here.
 * These use NUMBER values for paramId because that's what the DSP bridge expects.
 */

import { z } from 'zod';

export const KnobPropsSchema = z.object({
  paramId: z.number(),
  label: z.string().optional(),
  color: z.enum(['cyan', 'magenta', 'green', 'orange']).optional(),
  size: z.enum(['small', 'medium', 'large']).optional(),
});
export type KnobProps = z.infer<typeof KnobPropsSchema>;

export const DropdownPropsSchema = z.object({
  paramId: z.number(),
  label: z.string().optional(),
});
export type DropdownProps = z.infer<typeof DropdownPropsSchema>;

export const XYPadPropsSchema = z.object({
  paramIdX: z.number(),
  paramIdY: z.number(),
  labelX: z.string().optional(),
  labelY: z.string().optional(),
  size: z.number().optional(),
});
export type XYPadProps = z.infer<typeof XYPadPropsSchema>;

export const MeterPropsSchema = z.object({
  channel: z.union([z.literal(0), z.literal(1)]),
  compact: z.boolean().optional(),
});
export type MeterProps = z.infer<typeof MeterPropsSchema>;

export const FuturisticMeterPropsSchema = z.object({
  channel: z.union([z.literal(0), z.literal(1)]),
  label: z.string().optional(),
  color: z.enum(['cyan', 'magenta', 'green', 'orange']).optional(),
  showDb: z.boolean().optional(),
});
export type FuturisticMeterProps = z.infer<typeof FuturisticMeterPropsSchema>;

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
});
export type SectionProps = z.infer<typeof SectionPropsSchema> & {
  children: React.ReactNode;
};

export const SubGroupPropsSchema = z.object({
  title: z.string().optional(),
  layout: z.enum(['row', 'grid-2', 'grid-3', 'grid-4']).optional(),
});
export type SubGroupProps = z.infer<typeof SubGroupPropsSchema> & {
  children: React.ReactNode;
};
