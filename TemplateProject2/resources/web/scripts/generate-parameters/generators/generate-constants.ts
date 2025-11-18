/**
 * Generate constants.ts file from C++ enum and parameter metadata
 */

import type { ParameterMetadata } from "../utils/schemas.js";
import { extractEnumEntries } from "../utils/calculate-indices.js";

/**
 * Generate constants.ts file content
 */
export function generateConstantsFile(
  cppHeaderContent: string,
  metadata: ParameterMetadata[],
  paramIndices: Map<string, number>,
): string {
  // Extract EParams enum entries
  const eParamsEntries = extractEnumEntries(cppHeaderContent, "EParams");
  
  // Extract EControlTags enum entries
  const eControlTagsEntries = extractEnumEntries(cppHeaderContent, "EControlTags");

  // Create EParams enum
  const eParamsEnum = generateEParamsEnum(eParamsEntries);

  // Create EControlTags enum
  const eControlTagsEnum = generateEControlTagsEnum(eControlTagsEntries);

  // Create ParamNames mapping
  const paramNames = generateParamNames(metadata, paramIndices);

  return `/**
 * Parameter indices matching C++ TemplateProject.h EParams enum
 * AUTO-GENERATED - DO NOT EDIT MANUALLY
 * Generated from C++ source files using AI extraction
 */
${eParamsEnum}

/**
 * Control tag indices matching C++ TemplateProject.h EControlTags enum
 */
${eControlTagsEnum}

/**
 * iPlug2 message types for UI-to-processor communication
 */
export const MessageTypes = {
  SPVFUI: "SPVFUI", // Send Parameter Value From UI
  BPCFUI: "BPCFUI", // Begin Parameter Change From UI
  EPCFUI: "EPCFUI", // End Parameter Change From UI
  SAMFUI: "SAMFUI", // Send Arbitrary Message From UI
  SMMFUI: "SMMFUI", // Send MIDI Message From UI
} as const;

export type MessageType = typeof MessageTypes[keyof typeof MessageTypes];

/**
 * iPlug2 message types for processor-to-UI communication
 */
export const CallbackTypes = {
  SPVFD: "SPVFD", // Send Parameter Value From Delegate
  SCVFD: "SCVFD", // Send Control Value From Delegate
  SCMFD: "SCMFD", // Send Control Message From Delegate
  SAMFD: "SAMFD", // Send Arbitrary Message From Delegate
  SMMFD: "SMMFD", // Send MIDI Message From Delegate
} as const;

/**
 * Parameter display names
 */
export const ParamNames: Record<EParams, string> = {
${paramNames}
  [EParams.kNumParams]: "",
};

/**
 * MIDI note names
 */
export const NoteNames = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'] as const;

/**
 * MIDI status bytes
 */
export const MidiStatus = {
  NoteOn: 0x90,
  NoteOff: 0x80,
} as const;
`;
}

function generateEParamsEnum(entries: Array<{ name: string; index: number }>): string {
  let result = "export enum EParams {\n";
  let lastGroup = "";

  for (const entry of entries) {
    // Skip kNumParams for now (add at end)
    if (entry.name === "kNumParams") {
      continue;
    }

    // Add comment groups based on parameter name patterns
    const group = getParameterGroup(entry.name);
    if (group !== lastGroup && group) {
      result += `  // ${group}\n`;
      lastGroup = group;
    }

    result += `  ${entry.name} = ${entry.index},\n`;
  }

  // Add kNumParams at the end
  const numParamsEntry = entries.find((e) => e.name === "kNumParams");
  if (numParamsEntry) {
    result += `  ${numParamsEntry.name} = ${numParamsEntry.index}\n`;
  }

  result += "}";
  return result;
}

function generateEControlTagsEnum(
  entries: Array<{ name: string; index: number }>,
): string {
  let result = "export enum EControlTags {\n";

  for (const entry of entries) {
    if (entry.name === "kNumCtrlTags") {
      continue;
    }
    result += `  ${entry.name} = ${entry.index},\n`;
  }

  const numCtrlTagsEntry = entries.find((e) => e.name === "kNumCtrlTags");
  if (numCtrlTagsEntry) {
    result += `  ${numCtrlTagsEntry.name} = ${numCtrlTagsEntry.index}\n`;
  }

  result += "}";
  return result;
}

function generateParamNames(
  metadata: ParameterMetadata[],
  paramIndices: Map<string, number>,
): string {
  // Create a map of paramIdx -> name
  const paramNameMap = new Map<string, string>();
  for (const param of metadata) {
    paramNameMap.set(param.paramIdx, param.name);
  }

  // Generate entries sorted by index
  const entries: Array<{ index: number; name: string; displayName: string }> = [];
  for (const [paramIdx, index] of paramIndices.entries()) {
    if (paramIdx === "kNumParams") {
      continue;
    }
    const displayName = paramNameMap.get(paramIdx) || paramIdx;
    entries.push({ index, name: paramIdx, displayName });
  }

  entries.sort((a, b) => a.index - b.index);

  let result = "";
  let lastGroup = "";

  for (const entry of entries) {
    const group = getParameterGroup(entry.name);
    if (group !== lastGroup && group) {
      result += `  // ${group}\n`;
      lastGroup = group;
    }

    result += `  [EParams.${entry.name}]: "${entry.displayName}",\n`;
  }

  return result;
}

function getParameterGroup(paramName: string): string {
  if (paramName.includes("Osc") && !paramName.includes("Sync")) {
    return "Oscillators";
  }
  if (paramName.includes("Filter")) {
    return "Filter";
  }
  if (paramName.includes("LFO2")) {
    return "LFO2";
  }
  if (paramName.includes("Delay")) {
    return "Delay";
  }
  if (paramName.includes("Sync")) {
    return "Sync";
  }
  if (paramName.includes("Reverb")) {
    return "Reverb";
  }
  return "";
}

