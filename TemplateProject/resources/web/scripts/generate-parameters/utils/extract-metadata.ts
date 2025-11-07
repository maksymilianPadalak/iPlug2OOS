/**
 * Extract parameter metadata from C++ code using deterministic parsing
 * No AI required - parses Init* calls directly
 */

import type { ParameterMetadata } from "./schemas.js";

/**
 * Parse a C++ InitDouble call
 * Format: InitDouble("Name", default, min, max, step, "unit", flags, group, shape)
 * Example: InitDouble("Attack", 10., 1., 1000., 0.1, "ms", IParam::kFlagsNone, "ADSR", IParam::ShapePowCurve(3.))
 */
function parseInitDouble(line: string): ParameterMetadata | null {
  // Match: InitDouble("Name", default, min, max, step, "unit", flags, group, shape)
  // Example: InitDouble("Attack", 10., 1., 1000., 0.1, "ms", IParam::kFlagsNone, "ADSR", IParam::ShapePowCurve(3.))
  
  // First, check for ShapePowCurve in the entire line
  const powCurveMatch = line.match(/ShapePowCurve\s*\(\s*([\d.]+)\s*\)/);
  const hasExp = line.includes("ShapeExp");
  
  // Match the basic InitDouble pattern
  const match = line.match(
    /InitDouble\s*\(\s*"([^"]+)"\s*,\s*([\d.]+)\s*,\s*([\d.-]+)\s*,\s*([\d.]+)\s*(?:,\s*([\d.]+))?\s*(?:,\s*"([^"]*)")?\s*(?:,\s*[^,)]+)?\s*(?:,\s*"([^"]*)")?\s*(?:,\s*[^)]+)?\s*\)/,
  );

  if (!match) return null;

  const [, name, defaultValue, min, max, step, unit, group] = match;

  // Determine shape
  let shape: ParameterMetadata["shape"] = "ShapeLinear";
  let shapeParam: number | null = null;

  if (powCurveMatch) {
    shape = "ShapePowCurve";
    shapeParam = parseFloat(powCurveMatch[1]);
  } else if (hasExp) {
    shape = "ShapeExp";
  }

  return {
    paramIdx: "", // Will be filled later
    name: name.trim(),
    default: parseFloat(defaultValue),
    min: parseFloat(min),
    max: parseFloat(max),
    step: step ? parseFloat(step) : null,
    unit: unit || "",
    shape,
    shapeParam,
    type: "double",
    enumValues: null,
    group: group || null,
  };
}

/**
 * Parse a C++ InitPercentage call
 * Format: InitPercentage("Name", default)
 */
function parseInitPercentage(line: string): ParameterMetadata | null {
  const match = line.match(/InitPercentage\s*\(\s*"([^"]+)"\s*(?:,\s*([\d.]+))?\s*\)/);
  if (!match) return null;

  const [, name, defaultValue] = match;

  return {
    paramIdx: "",
    name: name.trim(),
    default: defaultValue ? parseFloat(defaultValue) : 0,
    min: 0,
    max: 100,
    step: null,
    unit: "%",
    shape: "ShapeLinear",
    shapeParam: null,
    type: "percentage",
    enumValues: null,
    group: null,
  };
}

/**
 * Parse a C++ InitFrequency call
 * Format: InitFrequency("Name", default, min, max)
 */
function parseInitFrequency(line: string): ParameterMetadata | null {
  const match = line.match(
    /InitFrequency\s*\(\s*"([^"]+)"\s*,\s*([\d.]+)\s*,\s*([\d.]+)\s*,\s*([\d.]+)\s*\)/,
  );
  if (!match) return null;

  const [, name, defaultValue, min, max] = match;

  return {
    paramIdx: "",
    name: name.trim(),
    default: parseFloat(defaultValue),
    min: parseFloat(min),
    max: parseFloat(max),
    step: null,
    unit: "Hz",
    shape: "ShapeExp", // Frequency uses exponential mapping
    shapeParam: null,
    type: "frequency",
    enumValues: null,
    group: null,
  };
}

/**
 * Parse a C++ InitMilliseconds call
 * Format: InitMilliseconds("Name", default, min, max)
 */
function parseInitMilliseconds(line: string): ParameterMetadata | null {
  const match = line.match(
    /InitMilliseconds\s*\(\s*"([^"]+)"\s*,\s*([\d.]+)\s*,\s*([\d.]+)\s*,\s*([\d.]+)\s*\)/,
  );
  if (!match) return null;

  const [, name, defaultValue, min, max] = match;

  return {
    paramIdx: "",
    name: name.trim(),
    default: parseFloat(defaultValue),
    min: parseFloat(min),
    max: parseFloat(max),
    step: null,
    unit: "ms",
    shape: "ShapeLinear",
    shapeParam: null,
    type: "milliseconds",
    enumValues: null,
    group: null,
  };
}

/**
 * Parse a C++ InitInt call
 * Format: InitInt("Name", default, min, max, "unit")
 */
function parseInitInt(line: string): ParameterMetadata | null {
  const match = line.match(
    /InitInt\s*\(\s*"([^"]+)"\s*,\s*(\d+)\s*,\s*([\d-]+)\s*,\s*(\d+)\s*(?:,\s*"([^"]*)")?\s*\)/,
  );
  if (!match) return null;

  const [, name, defaultValue, min, max, unit] = match;

  return {
    paramIdx: "",
    name: name.trim(),
    default: parseInt(defaultValue, 10),
    min: parseInt(min, 10),
    max: parseInt(max, 10),
    step: 1,
    unit: unit || "",
    shape: "ShapeLinear",
    shapeParam: null,
    type: "int",
    enumValues: null,
    group: null,
  };
}

/**
 * Parse a C++ InitBool call
 * Format: InitBool("Name", default, "label", flags, group, "offText", "onText")
 */
function parseInitBool(line: string): ParameterMetadata | null {
  const match = line.match(/InitBool\s*\(\s*"([^"]+)"\s*,\s*(true|false)\s*/);
  if (!match) return null;

  const [, name, defaultValue] = match;

  return {
    paramIdx: "",
    name: name.trim(),
    default: defaultValue === "true" ? 1 : 0,
    min: 0,
    max: 1,
    step: null,
    unit: "",
    shape: "ShapeLinear",
    shapeParam: null,
    type: "bool",
    enumValues: null,
    group: null,
  };
}

/**
 * Parse a C++ InitEnum call
 * Format: InitEnum("Name", default, {enumValues})
 */
function parseInitEnum(line: string): ParameterMetadata | null {
  // Match InitEnum with array of strings
  const match = line.match(
    /InitEnum\s*\(\s*"([^"]+)"\s*,\s*(\d+)\s*,\s*\{([^}]+)\}\s*\)/,
  );
  if (!match) return null;

  const [, name, defaultValue, enumValuesStr] = match;

  // Parse enum values: {"Sine", "Saw", "Square", "Triangle"}
  const enumValues = enumValuesStr
    .split(",")
    .map((s) => s.trim().replace(/^"|"$/g, ""))
    .filter((s) => s.length > 0);

  return {
    paramIdx: "",
    name: name.trim(),
    default: parseInt(defaultValue, 10),
    min: 0,
    max: enumValues.length - 1,
    step: 1,
    unit: "",
    shape: "ShapeLinear",
    shapeParam: null,
    type: "enum",
    enumValues,
    group: null,
  };
}

/**
 * Extract parameter metadata from C++ source code
 */
export function extractParameterMetadata(
  cppHeaderContent: string,
  cppSourceContent: string,
): ParameterMetadata[] {
  const parameters: ParameterMetadata[] = [];
  
  // Join lines to handle multi-line calls, then split by GetParam
  const fullText = cppSourceContent.replace(/\n\s*/g, " ");
  const getParamMatches = fullText.matchAll(/GetParam\s*\(\s*(\w+)\s*\)\s*->\s*([^;]+);/g);

  for (const match of getParamMatches) {
    const [, paramIdx, initCall] = match;

    // Skip kNumParams
    if (paramIdx === "kNumParams") continue;

    // Try each Init* parser
    let metadata: ParameterMetadata | null = null;

    if (initCall.includes("InitDouble")) {
      metadata = parseInitDouble(initCall);
    } else if (initCall.includes("InitPercentage")) {
      metadata = parseInitPercentage(initCall);
    } else if (initCall.includes("InitFrequency")) {
      metadata = parseInitFrequency(initCall);
    } else if (initCall.includes("InitMilliseconds")) {
      metadata = parseInitMilliseconds(initCall);
    } else if (initCall.includes("InitInt")) {
      metadata = parseInitInt(initCall);
    } else if (initCall.includes("InitBool")) {
      metadata = parseInitBool(initCall);
    } else if (initCall.includes("InitEnum")) {
      metadata = parseInitEnum(initCall);
    }

    if (metadata) {
      metadata.paramIdx = paramIdx;
      parameters.push(metadata);
    }
  }

  return parameters;
}
