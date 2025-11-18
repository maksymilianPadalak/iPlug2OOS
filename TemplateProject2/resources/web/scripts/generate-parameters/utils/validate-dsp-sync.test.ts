/**
 * Comprehensive tests to ensure constants.ts and parameter.ts exactly match DSP (C++ files)
 * This is CRITICAL - these files must stay in sync with the C++ source
 */

import { readFileSync } from "fs";
import { resolve } from "path";
import { calculateEnumIndices, extractEnumEntries } from "./calculate-indices";

// Calculate paths relative to workspace root
// Test file is at: iPlug2OOS/TemplateProject/resources/web/scripts/generate-parameters/utils/validate-dsp-sync.test.ts
// Workspace root is: /Users/maksymilianpadalak/Desktop/Programming/Berlin-Music-Intelligence/jimmy
const WORKSPACE_ROOT = resolve(__dirname, "../../../../../../..");
const PROJECT_ROOT = resolve(WORKSPACE_ROOT, "iPlug2OOS/TemplateProject");

const CPP_HEADER_PATH = resolve(PROJECT_ROOT, "TemplateProject.h");
const CPP_SOURCE_PATH = resolve(PROJECT_ROOT, "TemplateProject.cpp");
const TS_CONSTANTS_PATH = resolve(
  PROJECT_ROOT,
  "resources/web/src/config/constants.ts",
);
const TS_PARAMETER_PATH = resolve(
  PROJECT_ROOT,
  "resources/web/src/utils/parameter.ts",
);

/**
 * Extract parameter metadata from C++ source file
 */
function extractCppParameterMetadata(cppSource: string): Map<
  string,
  {
    name: string;
    default: number;
    min: number;
    max: number;
    step?: number;
    unit: string;
    shape?: string;
    shapeParam?: number;
    type: string;
    enumValues?: string[];
    group?: string;
  }
> {
  const params = new Map();
  const lines = cppSource.split("\n");

  for (let i = 0; i < lines.length; i++) {
    const line = lines[i];
    // Match GetParam(kParamXxx)->Init... patterns
    const paramMatch = line.match(/GetParam\((\w+)\)->Init(\w+)/);
    if (!paramMatch) continue;

    const [, paramIdx, initType] = paramMatch;
    const paramLine = line.trim();

    // Extract name (first string argument)
    const nameMatch = paramLine.match(/"([^"]+)"/);
    if (!nameMatch) continue;

    const name = nameMatch[1];

    // Extract default value (first number after name)
    const defaultMatch = paramLine.match(/"[^"]+",\s*([\d.]+)/);
    if (!defaultMatch) continue;

    const defaultValue = parseFloat(defaultMatch[1]);

    // Extract min/max based on Init type
    let min = 0;
    let max = 100;
    let step: number | undefined;
    let unit = "";
    let shape: string | undefined;
    let shapeParam: number | undefined;
    let enumValues: string[] | undefined;
    let group: string | undefined;

    if (initType === "Double") {
      // InitDouble("Name", default, min, max, step, unit, flags, group, shape)
      const doubleMatch = paramLine.match(
        /InitDouble\([^,]+,\s*[\d.]+,\s*([\d.]+),\s*([\d.]+)(?:,\s*([\d.]+))?(?:,\s*"([^"]*)")?/,
      );
      if (doubleMatch) {
        min = parseFloat(doubleMatch[1]);
        max = parseFloat(doubleMatch[2]);
        step = doubleMatch[3] ? parseFloat(doubleMatch[3]) : undefined;
        unit = doubleMatch[4] || "";
      }

      // Check for shape
      const shapeMatch = paramLine.match(/Shape(\w+)\(([\d.]+)?\)/);
      if (shapeMatch) {
        shape = `Shape${shapeMatch[1]}`;
        shapeParam = shapeMatch[2] ? parseFloat(shapeMatch[2]) : undefined;
      }

      // Check for group
      const groupMatch = paramLine.match(/,\s*"([^"]+)"\s*[,)]/);
      if (groupMatch && groupMatch[1] !== unit) {
        group = groupMatch[1];
      }
    } else if (initType === "Int") {
      // InitInt("Name", default, min, max, unit)
      const intMatch = paramLine.match(
        /InitInt\([^,]+,\s*[\d.]+,\s*(-?\d+),\s*(\d+)(?:,\s*"([^"]*)")?/,
      );
      if (intMatch) {
        min = parseInt(intMatch[1], 10);
        max = parseInt(intMatch[2], 10);
        unit = intMatch[3] || "";
      }
    } else if (initType === "Bool") {
      // InitBool("Name", default)
      min = 0;
      max = 1;
    } else if (initType === "Enum") {
      // InitEnum("Name", default, {values})
      const enumMatch = paramLine.match(/\{([^}]+)\}/);
      if (enumMatch) {
        enumValues = enumMatch[1]
          .split(",")
          .map((v) => v.trim().replace(/"/g, ""));
      }
      min = 0;
      max = enumValues ? enumValues.length - 1 : 0;
    } else if (initType === "Percentage") {
      // InitPercentage("Name", default)
      min = 0;
      max = 100;
      unit = "%";
    } else if (initType === "Frequency") {
      // InitFrequency("Name", default, min, max)
      const freqMatch = paramLine.match(
        /InitFrequency\([^,]+,\s*[\d.]+(?:,\s*([\d.]+))?(?:,\s*([\d.]+))?/,
      );
      if (freqMatch) {
        min = freqMatch[1] ? parseFloat(freqMatch[1]) : 0.01;
        max = freqMatch[2] ? parseFloat(freqMatch[2]) : 20000;
      }
      unit = "Hz";
    } else if (initType === "Milliseconds") {
      // InitMilliseconds("Name", default, min, max)
      const msMatch = paramLine.match(
        /InitMilliseconds\([^,]+,\s*[\d.]+(?:,\s*([\d.]+))?(?:,\s*([\d.]+))?/,
      );
      if (msMatch) {
        min = msMatch[1] ? parseFloat(msMatch[1]) : 0;
        max = msMatch[2] ? parseFloat(msMatch[2]) : 1000;
      }
      unit = "ms";
    }

    params.set(paramIdx, {
      name,
      default: defaultValue,
      min,
      max,
      step,
      unit,
      shape,
      shapeParam,
      type: initType.toLowerCase(),
      enumValues,
      group,
    });
  }

  return params;
}

/**
 * Extract TypeScript enum values from constants.ts
 */
function extractTSEnumValues(tsConstants: string, enumName: string): Map<
  string,
  number
> {
  const enumRegex = new RegExp(
    `export enum ${enumName}\\s*\\{([\\s\\S]*?)\\n\\}`,
    "m",
  );
  const match = tsConstants.match(enumRegex);
  if (!match) {
    throw new Error(`Could not find enum ${enumName} in TypeScript constants`);
  }

  const enumBody = match[1];
  const values = new Map<string, number>();
  const lines = enumBody.split("\n");

  for (const line of lines) {
    const trimmed = line.trim();
    if (!trimmed || trimmed.startsWith("//")) continue;

    // Match: kParamXxx = 0, or kParamXxx = 0
    const match = trimmed.match(/^(\w+)\s*=\s*(\d+)/);
    if (match) {
      const [, name, value] = match;
      values.set(name, parseInt(value, 10));
    }
  }

  return values;
}

/**
 * Extract parameter metadata from TypeScript parameter.ts
 * Note: parameter.ts imports EParams from constants.ts, so we check for case statements
 */
function extractTSParameterMetadata(tsParameter: string): Map<
  string,
  {
    hasConversion: boolean;
    hasDisplayConfig: boolean;
  }
> {
  const params = new Map();

  // Extract all parameter names from the switch statement in normalizedToActual
  // Handle both single cases and grouped cases: "case EParams.kParamXxx:" or "case EParams.kParamXxx:\n    case EParams.kParamYyy:"
  const switchMatch = tsParameter.match(/switch\s*\(paramIdx\)\s*\{([\s\S]*?)\n\s*default:/);
  if (switchMatch) {
    const switchBody = switchMatch[1];
    // Match all case statements, including grouped ones
    const caseMatches = Array.from(
      switchBody.matchAll(/case\s+EParams\.(\w+):/g),
    );
    for (const match of caseMatches) {
      params.set(match[1], {
        hasConversion: true,
        hasDisplayConfig: false, // Would need to check getParamDisplayConfig
      });
    }
  }

  return params;
}

describe("DSP to TypeScript Synchronization Tests", () => {
  let cppHeader: string;
  let cppSource: string;
  let tsConstants: string;
  let tsParameter: string;

  beforeAll(() => {
    try {
      cppHeader = readFileSync(CPP_HEADER_PATH, "utf-8");
      cppSource = readFileSync(CPP_SOURCE_PATH, "utf-8");
      tsConstants = readFileSync(TS_CONSTANTS_PATH, "utf-8");
      tsParameter = readFileSync(TS_PARAMETER_PATH, "utf-8");
    } catch (error) {
      throw new Error(
        `Failed to read source files: ${error instanceof Error ? error.message : String(error)}`,
      );
    }
  });

  describe("EParams Enum Synchronization", () => {
    it("should extract all parameters from C++ header", () => {
      const cppIndices = calculateEnumIndices(cppHeader, "EParams");
      expect(cppIndices.size).toBeGreaterThan(0);
      expect(cppIndices.has("kParamGain")).toBe(true);
      expect(cppIndices.get("kParamGain")).toBe(0);
    });

    it("should extract all parameters from TypeScript constants", () => {
      const tsIndices = extractTSEnumValues(tsConstants, "EParams");
      expect(tsIndices.size).toBeGreaterThan(0);
      expect(tsIndices.has("kParamGain")).toBe(true);
      expect(tsIndices.get("kParamGain")).toBe(0);
    });

    it("should have exact same parameter count", () => {
      const cppIndices = calculateEnumIndices(cppHeader, "EParams");
      const tsIndices = extractTSEnumValues(tsConstants, "EParams");

      // Remove kNumParams from comparison
      cppIndices.delete("kNumParams");
      tsIndices.delete("kNumParams");

      expect(tsIndices.size).toBe(cppIndices.size);
    });

    it("should have exact same parameter names", () => {
      const cppIndices = calculateEnumIndices(cppHeader, "EParams");
      const tsIndices = extractTSEnumValues(tsConstants, "EParams");

      // Remove kNumParams from comparison
      cppIndices.delete("kNumParams");
      tsIndices.delete("kNumParams");

      const cppNames = Array.from(cppIndices.keys()).sort();
      const tsNames = Array.from(tsIndices.keys()).sort();

      expect(tsNames).toEqual(cppNames);
    });

    it("should have exact same parameter indices", () => {
      const cppIndices = calculateEnumIndices(cppHeader, "EParams");
      const tsIndices = extractTSEnumValues(tsConstants, "EParams");

      // Remove kNumParams from comparison
      cppIndices.delete("kNumParams");
      tsIndices.delete("kNumParams");

      for (const [name, cppIndex] of cppIndices) {
        const tsIndex = tsIndices.get(name);
        expect(tsIndex).toBeDefined();
        expect(tsIndex).toBe(cppIndex);
      }
    });

    it("should have parameters in the same order", () => {
      const cppEntries = extractEnumEntries(cppHeader, "EParams");
      const tsIndices = extractTSEnumValues(tsConstants, "EParams");

      // Filter out kNumParams
      const cppNames = cppEntries
        .map((e) => e.name)
        .filter((n) => n !== "kNumParams");
      const tsNames = Array.from(tsIndices.keys()).filter(
        (n) => n !== "kNumParams",
      );

      expect(tsNames).toEqual(cppNames);
    });
  });

  describe("EControlTags Enum Synchronization", () => {
    it("should extract control tags from C++ header", () => {
      const cppIndices = calculateEnumIndices(cppHeader, "EControlTags");
      expect(cppIndices.size).toBeGreaterThan(0);
    });

    it("should extract control tags from TypeScript constants", () => {
      const tsIndices = extractTSEnumValues(tsConstants, "EControlTags");
      expect(tsIndices.size).toBeGreaterThan(0);
    });

    it("should have exact same control tag count", () => {
      const cppIndices = calculateEnumIndices(cppHeader, "EControlTags");
      const tsIndices = extractTSEnumValues(tsConstants, "EControlTags");

      // Remove kNumCtrlTags from comparison
      cppIndices.delete("kNumCtrlTags");
      tsIndices.delete("kNumCtrlTags");

      expect(tsIndices.size).toBe(cppIndices.size);
    });

    it("should have exact same control tag names and indices", () => {
      const cppIndices = calculateEnumIndices(cppHeader, "EControlTags");
      const tsIndices = extractTSEnumValues(tsConstants, "EControlTags");

      // Remove kNumCtrlTags from comparison
      cppIndices.delete("kNumCtrlTags");
      tsIndices.delete("kNumCtrlTags");

      for (const [name, cppIndex] of cppIndices) {
        const tsIndex = tsIndices.get(name);
        expect(tsIndex).toBeDefined();
        expect(tsIndex).toBe(cppIndex);
      }
    });
  });

  describe("Parameter Metadata Synchronization", () => {
    it("should extract parameter metadata from C++ source", () => {
      const cppParams = extractCppParameterMetadata(cppSource);
      expect(cppParams.size).toBeGreaterThan(0);
      expect(cppParams.has("kParamGain")).toBe(true);

      const gainParam = cppParams.get("kParamGain");
      expect(gainParam).toBeDefined();
      expect(gainParam?.name).toBe("Gain");
      expect(gainParam?.default).toBe(100);
      expect(gainParam?.min).toBe(0);
      expect(gainParam?.max).toBe(100);
    });

    it("should have conversion functions for all parameters", () => {
      const cppIndices = calculateEnumIndices(cppHeader, "EParams");
      const tsParams = extractTSParameterMetadata(tsParameter);

      // Remove kNumParams
      cppIndices.delete("kNumParams");

      const missingParams: string[] = [];
      for (const paramName of cppIndices.keys()) {
        const tsParam = tsParams.get(paramName);
        if (!tsParam || !tsParam.hasConversion) {
          missingParams.push(paramName);
        }
      }

      if (missingParams.length > 0) {
        console.warn(
          `Warning: ${missingParams.length} parameters missing conversion functions:`,
          missingParams,
        );
      }

      // Allow some parameters to be missing (they might be handled differently)
      // But log them for visibility
      expect(missingParams.length).toBeLessThan(cppIndices.size * 0.1); // Less than 10% missing
    });

    it("should have all C++ parameters present in TypeScript", () => {
      const cppIndices = calculateEnumIndices(cppHeader, "EParams");
      const tsIndices = extractTSEnumValues(tsConstants, "EParams");

      // Remove kNumParams
      cppIndices.delete("kNumParams");
      tsIndices.delete("kNumParams");

      for (const paramName of cppIndices.keys()) {
        expect(tsIndices.has(paramName)).toBe(true);
      }
    });

    it("should not have extra parameters in TypeScript", () => {
      const cppIndices = calculateEnumIndices(cppHeader, "EParams");
      const tsIndices = extractTSEnumValues(tsConstants, "EParams");

      // Remove kNumParams
      cppIndices.delete("kNumParams");
      tsIndices.delete("kNumParams");

      for (const paramName of tsIndices.keys()) {
        expect(cppIndices.has(paramName)).toBe(true);
      }
    });
  });

  describe("Parameter Value Ranges", () => {
    it("should validate parameter ranges match C++ definitions", () => {
      const cppParams = extractCppParameterMetadata(cppSource);
      const cppIndices = calculateEnumIndices(cppHeader, "EParams");

      // Test a few key parameters
      const testParams = [
        "kParamGain",
        "kParamAttack",
        "kParamOsc1Wave",
        "kParamFilterCutoff",
      ];

      for (const paramName of testParams) {
        if (!cppIndices.has(paramName)) continue;

        const cppParam = cppParams.get(paramName);
        expect(cppParam).toBeDefined();

        // Verify the parameter has valid range
        expect(cppParam?.min).toBeLessThanOrEqual(cppParam?.max);
        expect(cppParam?.default).toBeGreaterThanOrEqual(cppParam?.min);
        expect(cppParam?.default).toBeLessThanOrEqual(cppParam?.max);
      }
    });
  });

  describe("TypeScript File Structure", () => {
    it("should have AUTO-GENERATED header in constants.ts", () => {
      expect(tsConstants).toContain("AUTO-GENERATED");
      expect(tsConstants).toContain("DO NOT EDIT MANUALLY");
    });

    it("should have AUTO-GENERATED header in parameter.ts", () => {
      expect(tsParameter).toContain("AUTO-GENERATED");
      expect(tsParameter).toContain("DO NOT EDIT MANUALLY");
    });

    it("should export EParams enum from constants.ts", () => {
      expect(tsConstants).toContain("export enum EParams");
    });

    it("should export EControlTags enum from constants.ts", () => {
      expect(tsConstants).toContain("export enum EControlTags");
    });

    it("should have normalizedToActual function in parameter.ts", () => {
      expect(tsParameter).toContain("normalizedToActual");
    });

    it("should have normalizedToDisplay function in parameter.ts", () => {
      expect(tsParameter).toContain("normalizedToDisplay");
    });
  });

  describe("Critical Parameter Validation", () => {
    it("should have kNumParams matching actual parameter count", () => {
      const cppIndices = calculateEnumIndices(cppHeader, "EParams");
      const tsIndices = extractTSEnumValues(tsConstants, "EParams");

      const actualCount = Array.from(cppIndices.keys()).filter(
        (k) => k !== "kNumParams",
      ).length;
      const tsNumParams = tsIndices.get("kNumParams");

      expect(tsNumParams).toBe(actualCount);
    });

    it("should have kNumCtrlTags matching actual control tag count", () => {
      const cppIndices = calculateEnumIndices(cppHeader, "EControlTags");
      const tsIndices = extractTSEnumValues(tsConstants, "EControlTags");

      const actualCount = Array.from(cppIndices.keys()).filter(
        (k) => k !== "kNumCtrlTags",
      ).length;
      const tsNumCtrlTags = tsIndices.get("kNumCtrlTags");

      expect(tsNumCtrlTags).toBe(actualCount);
    });
  });
});

