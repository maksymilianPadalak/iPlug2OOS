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
  label: z.string().optional(),
  color: z.enum(['cyan', 'magenta', 'green', 'orange']).optional(),
  showDb: z.boolean().optional(),
});
export type MeterProps = z.infer<typeof MeterPropsSchema>;

/**
 * VintageMeter - VU-style analog meter with needle display.
 * NOTE: Kept for potential future use but not in LLM component registry.
 */
export const VintageMeterPropsSchema = z.object({
  channel: z.union([z.literal(0), z.literal(1)]),
  compact: z.boolean().optional(),
});
export type VintageMeterProps = z.infer<typeof VintageMeterPropsSchema>;

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

export const TitlePropsSchema = z.object({
  title: z.string(),
  version: z.string().optional(),
  color: z.enum(['cyan', 'magenta', 'green', 'orange']).optional(),
});
export type TitleProps = z.infer<typeof TitlePropsSchema>;

export const HeaderPropsSchema = z.object({});
export type HeaderProps = z.infer<typeof HeaderPropsSchema> & {
  children: React.ReactNode;
};

export const StandalonePropsSchema = z.object({
  size: z.enum(['sm', 'md', 'lg']).optional(),
});
export type StandaloneProps = z.infer<typeof StandalonePropsSchema> & {
  children: React.ReactNode;
};
