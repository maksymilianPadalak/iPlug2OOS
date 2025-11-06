/**
 * Extract parameter metadata from C++ code using AI
 */

import OpenAI from "openai";
import { zodTextFormat } from "openai/helpers/zod";
import {
  ParameterMetadataResponseSchema,
  type ParameterMetadata,
} from "./schemas.js";
import { AI_CONFIG, getOpenAIApiKey } from "../config.js";

const openai = new OpenAI({ apiKey: getOpenAIApiKey() });

/**
 * Extract parameter metadata from C++ initialization code
 */
export async function extractParameterMetadata(
  cppHeaderContent: string,
  cppSourceContent: string,
): Promise<ParameterMetadata[]> {
  const instructions = `You are an expert C++ code analyzer specializing in iPlug2 audio plugin parameter definitions.

Your task is to extract parameter metadata from C++ code that initializes iPlug2 parameters.

ANALYZE THE FOLLOWING:
1. Parameter initialization calls (GetParam(...)->InitDouble, InitPercentage, InitFrequency, InitInt, InitBool, InitEnum, InitMilliseconds)
2. Parameter shapes (ShapePowCurve(n), ShapeExp(), ShapeLinear(), etc.)
3. Parameter ranges (min, max values)
4. Default values
5. Units and labels
6. Parameter types (double, int, bool, enum)
7. Enum values (for InitEnum calls)
8. Parameter groups (from the group parameter)

DETECT SHAPE TYPES:
- InitDouble with ShapePowCurve(n) → shape: "ShapePowCurve", shapeParam: n
- InitFrequency → shape: "ShapeExp" (exponential frequency mapping)
- InitPercentage → shape: "ShapeLinear", type: "percentage"
- InitMilliseconds → shape: "ShapeLinear", type: "milliseconds"
- InitDouble with ShapeExp() → shape: "ShapeExp"
- InitDouble with ShapeLinear() or no shape → shape: "ShapeLinear"
- InitInt → type: "int", shape: "ShapeLinear"
- InitBool → type: "bool", shape: "ShapeLinear"
- InitEnum → type: "enum", shape: "ShapeLinear"

EXTRACT ACCURATELY:
- paramIdx: The parameter index name (e.g., kParamAttack)
- name: The display name from Init call (e.g., "Attack")
- default: The default value (first numeric argument after name)
- min: Minimum value (second numeric argument)
- max: Maximum value (third numeric argument)
- step: Step size (if provided, usually 4th numeric argument)
- unit: Unit label (e.g., "ms", "%", "Hz", "cents", "")
- shape: Shape type (see above)
- shapeParam: Shape parameter value (for ShapePowCurve)
- type: Parameter type (see above)
- enumValues: Array of enum option strings (for InitEnum)
- group: Parameter group name (if provided)

RETURN:
An object with a "parameters" field containing an array of parameter metadata objects, one for each GetParam(...)->Init* call found in the code.

Be precise and extract all parameters. Do not miss any.`;

  const input = `Extract parameter metadata from this C++ code:

HEADER FILE (TemplateProject.h):
${cppHeaderContent}

SOURCE FILE (TemplateProject.cpp):
${cppSourceContent}

Extract all parameter initialization calls and return structured metadata.`;

  console.log("🤖 Extracting parameter metadata using AI...");
  console.log(`Model: gpt-5-nano-2025-08-07`);

  try {
    const response = await openai.responses.parse({
      model: "gpt-5-nano-2025-08-07",
      instructions,
      input,
      text: {
        format: zodTextFormat(
          ParameterMetadataResponseSchema,
          "ParameterMetadataResponse",
        ),
        verbosity: "low",
      },
      store: false,
      max_output_tokens: AI_CONFIG.maxTokens,
      reasoning: { effort: "low" },
    });

    const responseData = response.output_parsed;
    const metadata = responseData?.parameters || [];

    if (!metadata || metadata.length === 0) {
      throw new Error("No parameter metadata extracted from C++ code");
    }

    console.log(`✅ Extracted ${metadata.length} parameters`);
    return metadata;
  } catch (error) {
    console.error("❌ Failed to extract parameter metadata:", error);
    throw error;
  }
}

