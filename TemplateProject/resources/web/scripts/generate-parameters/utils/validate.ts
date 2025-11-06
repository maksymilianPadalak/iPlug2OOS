/**
 * Validate generated code before writing
 */

import type { ParameterMetadata } from "./schemas.js";

export interface ValidationResult {
  valid: boolean;
  errors: string[];
  warnings: string[];
}

/**
 * Validate generated code
 */
export function validateGeneratedCode(
  constantsContent: string,
  parameterContent: string,
  metadata: ParameterMetadata[],
  paramIndices: Map<string, number>,
): ValidationResult {
  const errors: string[] = [];
  const warnings: string[] = [];

  // Check 1: All parameters have indices
  for (const param of metadata) {
    if (!paramIndices.has(param.paramIdx)) {
      errors.push(`Parameter ${param.paramIdx} has no index`);
    }
  }

  // Check 2: All parameters appear in constants.ts
  for (const param of metadata) {
    if (param.paramIdx !== "kNumParams" && !constantsContent.includes(param.paramIdx)) {
      errors.push(`Parameter ${param.paramIdx} not found in constants.ts`);
    }
  }

  // Check 3: Basic TypeScript syntax checks
  if (!constantsContent.includes("export enum EParams")) {
    errors.push("constants.ts missing EParams enum");
  }

  if (!parameterContent.includes("export function normalizedToActual")) {
    errors.push("parameter.ts missing normalizedToActual function");
  }

  if (!parameterContent.includes("export function actualToNormalized")) {
    errors.push("parameter.ts missing actualToNormalized function");
  }

  // Check 4: All parameters have conversion cases
  for (const param of metadata) {
    if (param.paramIdx === "kNumParams") {
      continue;
    }
    if (!parameterContent.includes(`case EParams.${param.paramIdx}:`)) {
      warnings.push(`Parameter ${param.paramIdx} missing conversion case`);
    }
  }

  // Check 5: Default values are present
  for (const param of metadata) {
    if (param.paramIdx === "kNumParams") {
      continue;
    }
    if (!parameterContent.includes(`EParams.${param.paramIdx}`)) {
      warnings.push(`Parameter ${param.paramIdx} missing from defaults`);
    }
  }

  return {
    valid: errors.length === 0,
    errors,
    warnings,
  };
}

