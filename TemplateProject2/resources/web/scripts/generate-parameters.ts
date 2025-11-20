#!/usr/bin/env node
/**
 * Parameter Generation Pipeline
 * 
 * Generates TypeScript parameter files (constants.ts and parameter.ts)
 * from C++ iPlug2 source code using deterministic parsing.
 * 
 * Usage: npm run generate-parameters
 */

import fs from "fs/promises";
import path from "path";
import { fileURLToPath } from "url";
import type { ParameterMetadata } from "./generate-parameters/utils/schemas.js";

// Check if required files exist
async function checkRequiredFiles(): Promise<{ available: boolean; errors: string[] }> {
  const errors: string[] = [];
  try {
    const { CPP_PATHS } = await import("./generate-parameters/config.js");
    try {
      await fs.access(CPP_PATHS.header);
    } catch {
      errors.push(`C++ header file not found: ${CPP_PATHS.header}`);
    }
    try {
      await fs.access(CPP_PATHS.cpp);
    } catch {
      errors.push(`C++ source file not found: ${CPP_PATHS.cpp}`);
    }
  } catch (error) {
    errors.push(`Failed to load config: ${error instanceof Error ? error.message : "Unknown error"}`);
  }
  return { available: errors.length === 0, errors };
}

async function main() {
  console.log("🔍 Checking prerequisites for parameter generation...\n");

  // Check required files
  const filesCheck = await checkRequiredFiles();
  if (!filesCheck.available) {
    console.error("❌ Missing required files:");
    for (const error of filesCheck.errors) {
      console.error(`   - ${error}`);
    }
    console.error("\n❌ Build failed: Parameter generation requires C++ source files.\n");
    process.exit(1); // Fail build
  }
  console.log("✅ Required C++ files found\n");

  // Now import dependencies
    const { CPP_PATHS, TS_PATHS } = await import("./generate-parameters/config.js");
  const { extractParameterMetadata } = await import("./generate-parameters/utils/extract-metadata.js");
  const { calculateEnumIndices } = await import("./generate-parameters/utils/calculate-indices.js");
  const { generateConstantsFile } = await import("./generate-parameters/generators/generate-constants.js");
  const { generateParameterFile } = await import("./generate-parameters/generators/generate-parameter.js");
  const { validateGeneratedCode } = await import("./generate-parameters/utils/validate.js");
  const {
    createBackup,
    removeBackup,
    restoreBackup,
    cleanupOldBackups,
  } = await import("./generate-parameters/utils/backup.js");

  console.log("🚀 Starting parameter generation pipeline...\n");

  let constantsBackup: string | null = null;
  let parameterBackup: string | null = null;

  try {
    // Step 1: Read C++ files
    console.log("📖 Reading C++ source files...");
    const cppHeaderContent = await fs.readFile(CPP_PATHS.header, "utf-8");
    const cppSourceContent = await fs.readFile(CPP_PATHS.cpp, "utf-8");
    console.log(`✅ Read ${CPP_PATHS.header}`);
    console.log(`✅ Read ${CPP_PATHS.cpp}\n`);

    // Step 2: Extract parameter metadata using deterministic parsing
    console.log("🔍 Extracting parameter metadata from C++ code...");
    const metadata = extractParameterMetadata(
      cppHeaderContent,
      cppSourceContent,
    );
    console.log(`✅ Extracted ${metadata.length} parameters\n`);

    // Step 3: Calculate parameter indices from enum
    console.log("🔢 Calculating parameter indices...");
    const paramIndices = calculateEnumIndices(cppHeaderContent, "EParams");
    console.log(`✅ Calculated ${paramIndices.size} parameter indices\n`);

    // Step 4: Generate TypeScript files
    console.log("⚙️  Generating TypeScript files...");
    const constantsContent = generateConstantsFile(
      cppHeaderContent,
      metadata,
      paramIndices,
    );
    const parameterContent = generateParameterFile(metadata, paramIndices);
    const manifestContent = buildParameterManifest(
      metadata,
      paramIndices,
      path.basename(CPP_PATHS.header, ".h"),
    );
    console.log("✅ Generated constants.ts");
    console.log("✅ Generated parameter.ts\n");

    // Step 5: Validate generated code
    console.log("✔️  Validating generated code...");
    const validation = validateGeneratedCode(
      constantsContent,
      parameterContent,
      metadata,
      paramIndices,
    );

    if (validation.warnings.length > 0) {
      console.log("⚠️  Warnings:");
      for (const warning of validation.warnings) {
        console.log(`   - ${warning}`);
      }
      console.log();
    }

    if (!validation.valid) {
      console.error("❌ Validation failed:");
      for (const error of validation.errors) {
        console.error(`   - ${error}`);
      }
      throw new Error("Generated code validation failed");
    }
    console.log("✅ Validation passed\n");

    // Step 6: Create backups
    console.log("📦 Creating backups...");
    constantsBackup = await createBackup(TS_PATHS.constants);
    parameterBackup = await createBackup(TS_PATHS.parameter);
    console.log();

    // Step 7: Write generated files
    console.log("💾 Writing generated files...");
    await fs.writeFile(TS_PATHS.constants, constantsContent, "utf-8");
    await fs.writeFile(TS_PATHS.parameter, parameterContent, "utf-8");
    await fs.writeFile(TS_PATHS.manifest, manifestContent, "utf-8");
    console.log(`✅ Wrote ${TS_PATHS.constants}`);
    console.log(`✅ Wrote ${TS_PATHS.parameter}\n`);

    // Step 8: Remove backups on success
    console.log("🧹 Cleaning up backups...");
    if (constantsBackup) {
      await removeBackup(constantsBackup);
    }
    if (parameterBackup) {
      await removeBackup(parameterBackup);
    }
    await cleanupOldBackups(7); // Remove backups older than 7 days
    console.log();

    console.log("✅ Parameter generation completed successfully!");
    console.log(`   Generated: ${TS_PATHS.constants}`);
    console.log(`   Generated: ${TS_PATHS.parameter}`);
    console.log(`   Generated: ${TS_PATHS.manifest}`);
  } catch (error) {
    console.error("\n❌ Parameter generation failed!");
    console.error(`   Error: ${error instanceof Error ? error.message : String(error)}`);
    if (error instanceof Error && error.stack) {
      console.error(`   Stack: ${error.stack.split("\n").slice(0, 3).join("\n")}`);
    }

    // Restore backups on error
    if (constantsBackup) {
      console.log("\n🔄 Restoring backup for constants.ts...");
      try {
        await restoreBackup(constantsBackup, TS_PATHS.constants);
        console.log("   ✅ Backup restored");
      } catch (restoreError) {
        console.error(`   ❌ Failed to restore backup: ${restoreError instanceof Error ? restoreError.message : String(restoreError)}`);
      }
    }
    if (parameterBackup) {
      console.log("🔄 Restoring backup for parameter.ts...");
      try {
        await restoreBackup(parameterBackup, TS_PATHS.parameter);
        console.log("   ✅ Backup restored");
      } catch (restoreError) {
        console.error(`   ❌ Failed to restore backup: ${restoreError instanceof Error ? restoreError.message : String(restoreError)}`);
      }
    }

    console.error("\n❌ Build failed: Parameter generation encountered errors.");
    console.error("   Parameter files were restored from backup.");
    console.error("   Fix the errors above and try again.\n");
    process.exit(1); // Fail build
  }
}

type ParameterManifestEntry = {
  paramIdx: string;
  index: number;
  name: string;
  defaultValue: number;
  minValue: number;
  maxValue: number;
  step: number | null;
  unit: string;
  type: ParameterMetadata["type"];
  shape: ParameterMetadata["shape"];
  shapeParameter: number | null;
  enumValues: string[] | null;
  group: string | null;
  automatable: boolean;
};

type ParameterManifest = {
  project: string;
  generatedAt: string;
  parameterCount: number;
  parameters: ParameterManifestEntry[];
};

function buildParameterManifest(
  metadata: ParameterMetadata[],
  paramIndices: Map<string, number>,
  projectName: string,
): string {
  const metadataMap = new Map(metadata.map((entry) => [entry.paramIdx, entry]));
  const entries: ParameterManifestEntry[] = [];

  const sortedParams = Array.from(paramIndices.entries()).sort(
    (a, b) => a[1] - b[1],
  );

  for (const [paramIdx, index] of sortedParams) {
    if (paramIdx === "kNumParams") continue;
    const entry = metadataMap.get(paramIdx);
    entries.push({
      paramIdx,
      index,
      name:
        entry?.name ??
        paramIdx
          .replace(/^kParam/, "")
          .replace(/([A-Z])/g, " $1")
          .trim(),
      defaultValue: entry?.default ?? 0,
      minValue: entry?.min ?? 0,
      maxValue: entry?.max ?? 1,
      step: entry?.step ?? null,
      unit: entry?.unit ?? "",
      type: entry?.type ?? "double",
      shape: entry?.shape ?? "ShapeLinear",
      shapeParameter: entry?.shapeParam ?? null,
      enumValues: entry?.enumValues ?? null,
      group: entry?.group ?? null,
      automatable: true,
    });
  }

  const manifest: ParameterManifest = {
    project: projectName,
    generatedAt: new Date().toISOString(),
    parameterCount: entries.length,
    parameters: entries,
  };

  return JSON.stringify(manifest, null, 2);
}

// Run main function
main().catch((error) => {
  console.error("\n❌ Fatal error in parameter generation:");
  console.error(`   ${error instanceof Error ? error.message : String(error)}`);
  if (error instanceof Error && error.stack) {
    console.error(`\n   Stack trace:\n${error.stack.split("\n").slice(0, 5).map(line => `   ${line}`).join("\n")}`);
  }
  console.error("\n❌ Build failed: Parameter generation encountered a fatal error.\n");
  process.exit(1); // Fail build
});

