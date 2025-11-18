import path from "path";
import { fileURLToPath } from "url";

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

// Project root (resources/web)
const PROJECT_ROOT = path.resolve(__dirname, "../..");

// Paths to C++ source files
export const CPP_PATHS = {
  header: path.resolve(PROJECT_ROOT, "../../TemplateProject2.h"),
  cpp: path.resolve(PROJECT_ROOT, "../../TemplateProject2.cpp"),
};

// Paths to output TypeScript files
export const TS_PATHS = {
  constants: path.resolve(PROJECT_ROOT, "src/config/constants.ts"),
  parameter: path.resolve(PROJECT_ROOT, "src/utils/parameter.ts"),
};

// Backup directory
export const BACKUP_DIR = path.resolve(PROJECT_ROOT, ".parameter-backups");

// AI configuration
export const AI_CONFIG = {
  maxTokens: 10000,
};

// OpenAI API key (from environment)
export function getOpenAIApiKey(): string {
  const key = process.env.OPENAI_API_KEY;
  if (!key) {
    throw new Error(
      "OPENAI_API_KEY environment variable is not set. Please set it before running parameter generation.",
    );
  }
  return key;
}

