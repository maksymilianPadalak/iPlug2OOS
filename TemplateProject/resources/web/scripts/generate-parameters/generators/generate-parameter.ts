/**
 * Generate parameter.ts file with conversion functions
 */

import type { ParameterMetadata } from "../utils/schemas.js";
import {
  generateNormalizedToActual,
  generateActualToNormalized,
  generateDefaultNormalized,
} from "../utils/generate-conversion-formulas.js";

/**
 * Generate parameter.ts file content
 */
export function generateParameterFile(
  metadata: ParameterMetadata[],
  paramIndices: Map<string, number>,
): string {
  // Sort metadata by index
  const sortedMetadata = [...metadata].sort((a, b) => {
    const idxA = paramIndices.get(a.paramIdx) ?? 9999;
    const idxB = paramIndices.get(b.paramIdx) ?? 9999;
    return idxA - idxB;
  });

  const normalizedToActualCases = generateNormalizedToActualCases(
    sortedMetadata,
    paramIndices,
  );
  const actualToNormalizedCases = generateActualToNormalizedCases(
    sortedMetadata,
    paramIndices,
  );
  const displayConfigCases = generateDisplayConfigCases(sortedMetadata);
  const defaultValues = generateDefaultValues(sortedMetadata, paramIndices);
  const paramInputIds = generateParamInputIds(sortedMetadata, paramIndices);

  return `/**
 * Parameter value conversion and display utilities
 * AUTO-GENERATED - DO NOT EDIT MANUALLY
 * Generated from C++ source files using deterministic parsing
 */

import { EParams } from '../config/constants';

/**
 * Parameter display configuration
 */
interface ParamDisplayConfig {
  unit: string;
  format: (value: number) => string;
}

/**
 * Convert normalized parameter value (0-1) to display value
 */
export function normalizedToDisplay(paramIdx: EParams, normalizedValue: number): string {
  const config = getParamDisplayConfig(paramIdx);
  const displayValue = normalizedToActual(paramIdx, normalizedValue);
  return config.format(displayValue) + ' ' + config.unit;
}

/**
 * Convert normalized parameter value (0-1) to actual value
 */
export function normalizedToActual(paramIdx: EParams, normalizedValue: number): number {
  const value = Math.max(0, Math.min(1, normalizedValue));
  
  switch (paramIdx) {
${normalizedToActualCases}
    default:
      return value;
  }
}

/**
 * Get parameter display configuration
 */
function getParamDisplayConfig(paramIdx: EParams): ParamDisplayConfig {
  switch (paramIdx) {
${displayConfigCases}
    default:
      return {
        unit: '',
        format: (v) => v.toString()
      };
  }
}

/**
 * Convert actual parameter value to normalized value (0-1)
 * This is the inverse of normalizedToActual
 */
export function actualToNormalized(paramIdx: EParams, actualValue: number): number {
  switch (paramIdx) {
${actualToNormalizedCases}
    default:
      return actualValue;
  }
}

/**
 * Get default normalized values matching C++ DSP defaults
 * These values exactly match the defaults in TemplateProject.cpp
 */
export function getDefaultNormalizedValues(): Map<EParams, number> {
  const defaults = new Map<EParams, number>();
  
${defaultValues}
  
  return defaults;
}

export function getParamInputId(paramIdx: EParams): string {
  const paramNames: Record<EParams, string> = {
${paramInputIds}
    [EParams.kNumParams]: '',
  };
  
  return paramNames[paramIdx];
}

/**
 * Get parameter value display element ID
 */
export function getParamValueId(paramIdx: EParams): string {
  return getParamInputId(paramIdx) + 'Value';
}
`;
}

function generateNormalizedToActualCases(
  metadata: ParameterMetadata[],
  paramIndices: Map<string, number>,
): string {
  // Group parameters by their conversion formula (for case fall-through)
  const formulaGroups = new Map<string, ParameterMetadata[]>();

  for (const param of metadata) {
    if (param.paramIdx === "kNumParams") {
      continue;
    }
    const formula = generateNormalizedToActual(param);
    const key = formula.trim();
    if (!formulaGroups.has(key)) {
      formulaGroups.set(key, []);
    }
    formulaGroups.get(key)!.push(param);
  }

  let result = "";
  let lastGroup = "";

  for (const param of metadata) {
    if (param.paramIdx === "kNumParams") {
      continue;
    }

    const group = getParameterGroup(param.paramIdx);
    if (group !== lastGroup && group) {
      result += `    // ${group}\n`;
      lastGroup = group;
    }

    const formula = generateNormalizedToActual(param);
    const groupParams = formulaGroups.get(formula.trim()) || [param];

    if (groupParams.length > 1 && groupParams[0].paramIdx === param.paramIdx) {
      // First param in group - use fall-through
      result += `    case EParams.${param.paramIdx}:\n`;
      for (let i = 1; i < groupParams.length; i++) {
        result += `    case EParams.${groupParams[i].paramIdx}:\n`;
      }
      result += `      return ${formula}\n`;
    } else if (groupParams.length === 1) {
      // Single param
      result += `    case EParams.${param.paramIdx}:\n`;
      result += `      return ${formula}\n`;
    }
  }

  return result;
}

function generateActualToNormalizedCases(
  metadata: ParameterMetadata[],
  paramIndices: Map<string, number>,
): string {
  const formulaGroups = new Map<string, ParameterMetadata[]>();

  for (const param of metadata) {
    if (param.paramIdx === "kNumParams") {
      continue;
    }
    const formula = generateActualToNormalized(param);
    const key = formula.trim();
    if (!formulaGroups.has(key)) {
      formulaGroups.set(key, []);
    }
    formulaGroups.get(key)!.push(param);
  }

  let result = "";
  let lastGroup = "";

  for (const param of metadata) {
    if (param.paramIdx === "kNumParams") {
      continue;
    }

    const group = getParameterGroup(param.paramIdx);
    if (group !== lastGroup && group) {
      result += `    // ${group}\n`;
      lastGroup = group;
    }

    const formula = generateActualToNormalized(param);
    const groupParams = formulaGroups.get(formula.trim()) || [param];

    if (groupParams.length > 1 && groupParams[0].paramIdx === param.paramIdx) {
      result += `    case EParams.${param.paramIdx}:\n`;
      for (let i = 1; i < groupParams.length; i++) {
        result += `    case EParams.${groupParams[i].paramIdx}:\n`;
      }
      result += `      return ${formula}\n`;
    } else if (groupParams.length === 1) {
      result += `    case EParams.${param.paramIdx}:\n`;
      result += `      return ${formula}\n`;
    }
  }

  return result;
}

function generateDisplayConfigCases(
  metadata: ParameterMetadata[],
): string {
  // Group by unit and format
  const configGroups = new Map<string, ParameterMetadata[]>();

  for (const param of metadata) {
    if (param.paramIdx === "kNumParams") {
      continue;
    }
    const unit = getDisplayUnit(param);
    const format = getDisplayFormat(param);
    const key = `${unit}|${format}`;
    if (!configGroups.has(key)) {
      configGroups.set(key, []);
    }
    configGroups.get(key)!.push(param);
  }

  let result = "";

  for (const [key, params] of configGroups.entries()) {
    const [unit, format] = key.split("|");
    const param = params[0]; // Use first param for format code
    const formatCode = getFormatCode(format, param);

    if (params.length > 1) {
      result += `    case EParams.${params[0].paramIdx}:\n`;
      for (let i = 1; i < params.length; i++) {
        result += `    case EParams.${params[i].paramIdx}:\n`;
      }
    } else {
      result += `    case EParams.${params[0].paramIdx}:\n`;
    }

    result += `      return {\n`;
    result += `        unit: '${unit}',\n`;
    result += `        format: ${formatCode}\n`;
    result += `      };\n`;
  }

  return result;
}

function generateDefaultValues(
  metadata: ParameterMetadata[],
  paramIndices: Map<string, number>,
): string {
  let result = "";

  for (const param of metadata) {
    if (param.paramIdx === "kNumParams") {
      continue;
    }

    const defaultNormalized = generateDefaultNormalized(param);
    const comment = `// ${param.name}: ${param.default} (${param.min}-${param.max}${param.unit ? ` ${param.unit}` : ""})`;

    result += `  ${comment}\n`;
    result += `  defaults.set(EParams.${param.paramIdx}, ${defaultNormalized});\n`;
  }

  return result;
}

function generateParamInputIds(
  metadata: ParameterMetadata[],
  paramIndices: Map<string, number>,
): string {
  let result = "";

  // Use paramIndices to include ALL enum values, not just those in metadata
  // This ensures enum parameters (like kParamLFOShape) are included even if
  // they're not in the metadata array
  const sortedParams = Array.from(paramIndices.entries())
    .filter(([key]) => key !== "kNumParams")
    .sort(([, idxA], [, idxB]) => idxA - idxB);

  for (const [paramIdx] of sortedParams) {
    const inputId = paramIdxToInputId(paramIdx);
    result += `    [EParams.${paramIdx}]: '${inputId}',\n`;
  }

  return result;
}

function getParameterGroup(paramIdx: string): string {
  if (paramIdx.includes("Osc") && !paramIdx.includes("Sync")) {
    return "Oscillator parameters";
  }
  if (paramIdx.includes("Filter")) {
    return "Filter parameters";
  }
  if (paramIdx.includes("LFO2")) {
    return "LFO2";
  }
  if (paramIdx.includes("Delay")) {
    return "Delay";
  }
  if (paramIdx.includes("Sync")) {
    return "Sync";
  }
  if (paramIdx.includes("Reverb")) {
    return "Reverb parameters";
  }
  return "";
}

function getDisplayUnit(param: ParameterMetadata): string {
  const unit = param.unit.trim();
  if (unit === "%") {
    return "%";
  }
  if (unit === "ms") {
    return "ms";
  }
  if (unit === "Hz") {
    return "Hz";
  }
  if (unit === "cents") {
    return "ct";
  }
  if (unit === "Hz/key") {
    return "Hz/key";
  }
  if (unit === ":1") {
    return ":1";
  }
  return "";
}

function getDisplayFormat(param: ParameterMetadata): string {
  const unit = param.unit.trim();
  if (unit === "%" || unit === "ms" || unit === "Hz" || unit === "Hz/key") {
    return "fixed1";
  }
  if (unit === "ct") {
    return "fixed1";
  }
  if (unit === ":1") {
    return "fixed3";
  }
  if (param.min < 0) {
    return "signed";
  }
  return "default";
}

function getFormatCode(format: string, param: ParameterMetadata): string {
  switch (format) {
    case "fixed1":
      return "(v) => v.toFixed(1)";
    case "fixed2":
      return "(v) => v.toFixed(2)";
    case "fixed3":
      return "(v) => v.toFixed(3)";
    case "fixed0":
      return "(v) => v.toFixed(0)";
    case "signed":
      return "(v) => v >= 0 ? `+${v.toFixed(0)}` : `${v.toFixed(0)}`";
    case "octave":
      return "(v) => v >= 0 ? `+${v}` : `${v}`";
    default:
      return "(v) => v.toString()";
  }
}

function paramIdxToInputId(paramIdx: string): string {
  // Convert kParamAttack -> paramAttack
  return paramIdx.replace(/^kParam/, "param");
}

