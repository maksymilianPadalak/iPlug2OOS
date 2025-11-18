/**
 * Zod schema for parameter metadata extracted from C++ code
 * Updated: All optional fields must be nullable for OpenAI structured outputs compatibility
 */

import { z } from "zod";

export const ParameterMetadataSchema = z.object({
  paramIdx: z.string().describe("Parameter index name (e.g., 'kParamAttack')"),
  name: z.string().describe("Parameter display name (e.g., 'Attack')"),
  default: z.number().describe("Default value"),
  min: z.number().describe("Minimum value"),
  max: z.number().describe("Maximum value"),
  step: z.number().nullable().optional().describe("Step size (if applicable)"),
  unit: z.string().describe("Unit label (e.g., 'ms', '%', 'Hz', 'cents', '')"),
  shape: z
    .enum([
      "ShapeLinear",
      "ShapePowCurve",
      "ShapeExp",
      "ShapeLog",
      "ShapeSin",
      "ShapeTan",
      "ShapeSigmoid",
    ])
    .describe("Parameter shape/curve type"),
  shapeParam: z
    .number()
    .nullable()
    .optional()
    .describe("Shape parameter (e.g., 3.0 for ShapePowCurve(3.))"),
  type: z
    .enum(["double", "int", "bool", "enum", "percentage", "frequency", "milliseconds"])
    .describe("Parameter type"),
  enumValues: z
    .array(z.string())
    .nullable()
    .optional()
    .describe("Enum values (for enum type parameters)"),
  group: z.string().nullable().optional().describe("Parameter group name (e.g., 'ADSR', 'Filter')"),
});

export const ParameterMetadataArraySchema = z.array(ParameterMetadataSchema);

// Wrapper schema for OpenAI API (must be an object, not array)
export const ParameterMetadataResponseSchema = z.object({
  parameters: ParameterMetadataArraySchema,
});

export type ParameterMetadata = z.infer<typeof ParameterMetadataSchema>;
export type ParameterMetadataResponse = z.infer<typeof ParameterMetadataResponseSchema>;

