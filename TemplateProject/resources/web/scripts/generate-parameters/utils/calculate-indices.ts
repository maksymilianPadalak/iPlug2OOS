/**
 * Calculate parameter indices from C++ enum
 * Handles both explicit assignments and auto-incrementing
 */

/**
 * Extract the enum body from C++ enum declaration
 */
function extractEnumBody(enumText: string, enumName: string): string {
  // Find the enum declaration
  const enumRegex = new RegExp(
    `enum\\s+${enumName}\\s*\\{([\\s\\S]*?)\\n\\}`,
    "m",
  );
  const match = enumText.match(enumRegex);

  if (!match) {
    throw new Error(`Could not find enum ${enumName} in provided text`);
  }

  return match[1];
}

/**
 * Parse a single enum line and extract the parameter name and optional value
 */
function parseEnumLine(line: string): {
  name: string;
  explicitValue?: number;
} | null {
  // Remove comments
  const withoutComments = line.replace(/\/\/.*$/, "").trim();

  // Skip empty lines
  if (!withoutComments) {
    return null;
  }

  // Check for explicit assignment: "kParamGain = 0," or "kParamGain = 0"
  const explicitMatch = withoutComments.match(/^(\w+)\s*=\s*(\d+)/);
  if (explicitMatch) {
    const [, name, value] = explicitMatch;
    return {
      name,
      explicitValue: parseInt(value, 10),
    };
  }

  // Check for auto-increment: "kParamNoteGlideTime," or "kParamNoteGlideTime"
  const autoMatch = withoutComments.match(/^(\w+)/);
  if (autoMatch) {
    const [, name] = autoMatch;
    return { name };
  }

  return null;
}

/**
 * Calculate parameter indices from C++ enum text
 * @param enumText Full C++ file content or enum declaration
 * @param enumName Name of the enum (e.g., "EParams", "EControlTags")
 * @returns Map of parameter name to index
 */
export function calculateEnumIndices(
  enumText: string,
  enumName: string,
): Map<string, number> {
  const indices = new Map<string, number>();
  let currentIndex = -1; // Start at -1, first entry will be 0

  // Extract enum body
  const enumBody = extractEnumBody(enumText, enumName);

  // Split by lines and process
  const lines = enumBody.split("\n");

  for (const line of lines) {
    const parsed = parseEnumLine(line);
    if (!parsed) {
      continue;
    }

    if (parsed.explicitValue !== undefined) {
      // Explicit assignment
      currentIndex = parsed.explicitValue;
      indices.set(parsed.name, currentIndex);
    } else {
      // Auto-increment
      currentIndex += 1;
      indices.set(parsed.name, currentIndex);
    }
  }

  return indices;
}

/**
 * Extract enum entries in order (for generating TypeScript enum)
 */
export function extractEnumEntries(
  enumText: string,
  enumName: string,
): Array<{ name: string; index: number }> {
  const indices = calculateEnumIndices(enumText, enumName);
  const enumBody = extractEnumBody(enumText, enumName);
  const entries: Array<{ name: string; index: number }> = [];

  const lines = enumBody.split("\n");
  let currentIndex = -1;

  for (const line of lines) {
    const parsed = parseEnumLine(line);
    if (!parsed) {
      continue;
    }

    if (parsed.explicitValue !== undefined) {
      currentIndex = parsed.explicitValue;
    } else {
      currentIndex += 1;
    }

    entries.push({
      name: parsed.name,
      index: currentIndex,
    });
  }

  return entries;
}

