/**
 * iPlug2 Vector Store Utility
 * 
 * Manages OpenAI vector store for iPlug2 codebase to improve DSP code generation.
 */

import OpenAI from "openai";
import fs from "fs/promises";
import fsSync from "fs";
import path from "path";
import { fileURLToPath } from "url";
import { glob } from "glob";
import { tmpdir } from "os";

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

// Get OpenAI API key from environment
function getOpenAIApiKey(): string {
  const key = process.env.OPENAI_API_KEY;
  if (!key) {
    throw new Error(
      "OPENAI_API_KEY environment variable is not set. Please set it before running vector store operations.",
    );
  }
  return key;
}

// Lazy initialization of OpenAI client to avoid module-level initialization issues
function getOpenAIClient(): OpenAI {
  return new OpenAI({ apiKey: getOpenAIApiKey() });
}

// Config file path (relative to scripts/generate-parameters/)
const CONFIG_PATH = path.resolve(__dirname, "../.vector-store-config.json");

// iPlug2 root directory (7 levels up from utils/ to reach iPlug2OOS/)
// utils/ -> generate-parameters/ -> scripts/ -> web/ -> resources/ -> TemplateProject/ -> iPlug2OOS/ -> iPlug2
const IPLUG2_ROOT = path.resolve(__dirname, "../../../../../../iPlug2");

let VECTOR_STORE_ID: string | null = null;

/**
 * Get or create the iPlug2 vector store
 */
export async function getIPlug2VectorStore(): Promise<string | null> {
  // Check if we already have a vector store ID in memory
  if (VECTOR_STORE_ID) {
    console.log(`📦 Using cached vector store ID: ${VECTOR_STORE_ID}`);
    return VECTOR_STORE_ID;
  }

  // Try to load from config file
  try {
    const configContent = await fs.readFile(CONFIG_PATH, "utf-8");
    const config = JSON.parse(configContent);
    if (config.vectorStoreId) {
      VECTOR_STORE_ID = config.vectorStoreId;
      console.log(`📦 Using existing vector store from config: ${VECTOR_STORE_ID}`);
      return VECTOR_STORE_ID;
    }
  } catch {
    console.log(`ℹ️  No existing vector store config found at ${CONFIG_PATH}`);
  }

  // No existing vector store found
  return null;
}

/**
 * Create a new vector store
 */
export async function createVectorStore(): Promise<string> {
  console.log("📦 Creating new iPlug2 vector store...");
  
  const openai = getOpenAIClient();
  const vectorStore = await openai.vectorStores.create({
    name: "iPlug2 DSP Codebase",
  });

  VECTOR_STORE_ID = vectorStore.id;

  // Save for future use
  await fs.mkdir(path.dirname(CONFIG_PATH), { recursive: true });
  await fs.writeFile(
    CONFIG_PATH,
    JSON.stringify({ vectorStoreId: VECTOR_STORE_ID, createdAt: new Date().toISOString() }, null, 2),
    "utf-8"
  );

  console.log(`✅ Vector store created: ${VECTOR_STORE_ID}`);
  return VECTOR_STORE_ID;
}

/**
 * Discover all valuable iPlug2 files
 */
export async function discoverIPlug2Files(): Promise<string[]> {
  console.log("🔍 Discovering iPlug2 files...");
  console.log(`📁 iPlug2 root path: ${IPLUG2_ROOT}`);
  
  // Check if iPlug2 directory exists
  try {
    const stats = await fs.stat(IPLUG2_ROOT);
    if (!stats.isDirectory()) {
      throw new Error(`iPlug2 path exists but is not a directory: ${IPLUG2_ROOT}`);
    }
    console.log(`✅ iPlug2 directory exists`);
  } catch (error) {
    console.error(`❌ iPlug2 directory not found at: ${IPLUG2_ROOT}`);
    console.error(`   Error: ${error instanceof Error ? error.message : String(error)}`);
    throw new Error(`iPlug2 directory not found at ${IPLUG2_ROOT}. Please ensure iPlug2OOS/iPlug2 exists.`);
  }
  
  const filePatterns = [
    // Core framework source files
    "IPlug/**/*.h",
    "IPlug/**/*.cpp",
    "IPlug/**/*.hpp",
    "IPlug/**/*.mm",
    "IPlug/**/*.m",
    
    // DSP Extras (oscillators, filters, envelopes)
    "IPlug/Extras/**/*.h",
    "IPlug/Extras/**/*.cpp",
    
    // Examples (complete working implementations)
    "Examples/**/*.h",
    "Examples/**/*.cpp",
    "Examples/**/*.mm",
    "Examples/**/*.m",
    
    // Tests (test patterns)
    "Tests/**/*.h",
    "Tests/**/*.cpp",
    
    // Documentation
    "Documentation/**/*.md",
    "*.md",
    "**/README.md",
    
    // Web-specific code
    "IPlug/WEB/**/*.h",
    "IPlug/WEB/**/*.cpp",
    "IPlug/WEB/**/*.js",
    
    // Configuration files (might show patterns)
    "*.xcconfig",
    "*.props",
    
    // Scripts (useful patterns)
    "Scripts/**/*.py",
    "Scripts/**/*.sh",
  ];

  const allFiles: string[] = [];
  
  console.log(`📋 Searching ${filePatterns.length} file patterns...`);
  
  for (const pattern of filePatterns) {
    try {
      const files = await glob(pattern, {
        cwd: IPLUG2_ROOT,
        absolute: true,
        ignore: [
          // Build artifacts
          "**/*.o",
          "**/*.a",
          "**/*.dylib",
          "**/*.so",
          "**/*.dll",
          "**/*.exe",
          "**/*.bundle",
          "**/*.app",
          "**/*.vst",
          "**/*.vst3",
          "**/*.component",
          // Build directories
          "**/build/**",
          "**/Build/**",
          "**/*.xcodeproj/build/**",
          "**/*.xcworkspace/xcuserdata/**",
          // IGraphics (UI framework, not needed for DSP)
          "**/IGraphics/**",
          // Git
          "**/.git/**",
          // TemplateProject (user's plugin, not framework code)
          "**/TemplateProject/**",
          // Dependencies SDKs (large third-party SDKs)
          "**/Dependencies/IPlug/*_SDK/**",
          "**/Dependencies/IPlug/AAX_SDK/**",
          "**/Dependencies/IPlug/VST2_SDK/**",
          "**/Dependencies/IPlug/VST3_SDK/**",
          "**/Dependencies/IPlug/CLAP_SDK/**",
          "**/Dependencies/IPlug/CLAP_HELPERS/**",
          "**/Dependencies/IPlug/WAM_SDK/**",
          "**/Dependencies/IPlug/WAM_AWP/**",
          // Node modules
          "**/node_modules/**",
          // Large binary files (keep small ones)
          "**/*.png",
          "**/*.jpg",
          "**/*.jpeg",
          "**/*.gif",
          "**/*.ico",
          "**/*.icns",
          "**/*.pdf", // Exclude PDFs (academic papers are large)
          // Generated files
          "**/*.map",
          "**/*.log",
        ],
      });
      
      if (files.length > 0) {
        console.log(`  ✅ Pattern "${pattern}": found ${files.length} files`);
        allFiles.push(...files);
      } else {
        console.log(`  ⚠️  Pattern "${pattern}": no files found`);
      }
    } catch (error) {
      console.error(`  ❌ Failed to glob pattern "${pattern}":`, error instanceof Error ? error.message : String(error));
    }
  }

  // Remove duplicates
  const uniqueFiles = Array.from(new Set(allFiles));
  console.log(`📊 Found ${allFiles.length} total files (${uniqueFiles.length} unique)`);
  
  if (uniqueFiles.length === 0) {
    console.error(`❌ No files found! This might indicate:`);
    console.error(`   1. iPlug2 directory is empty or doesn't contain source files`);
    console.error(`   2. File patterns don't match the actual file structure`);
    console.error(`   3. All files are being filtered out by ignore patterns`);
    console.error(`   📁 Checked directory: ${IPLUG2_ROOT}`);
    
    // Try to list what's actually in the directory
    try {
      const dirContents = await fs.readdir(IPLUG2_ROOT);
      console.error(`   📂 Directory contents (first 20):`, dirContents.slice(0, 20));
    } catch (err) {
      console.error(`   ❌ Could not read directory contents:`, err);
    }
    
    throw new Error(`No files found in iPlug2 directory. Checked ${filePatterns.length} patterns, found 0 files.`);
  }
  
  // Filter by file size (max 10MB per file, OpenAI limit is 512MB)
  const validFiles: string[] = [];
  let skippedLarge = 0;
  let skippedErrors = 0;
  
  console.log(`🔍 Validating ${uniqueFiles.length} files (checking size, max 10MB per file)...`);
  
  for (const filePath of uniqueFiles) {
    try {
      const stats = await fs.stat(filePath);
      const sizeMB = stats.size / (1024 * 1024);
      
      if (sizeMB > 10) {
        skippedLarge++;
        if (skippedLarge <= 5) {
          console.warn(`⚠️  Skipping large file (${sizeMB.toFixed(2)}MB): ${path.relative(IPLUG2_ROOT, filePath)}`);
        }
        continue;
      }
      
      validFiles.push(filePath);
    } catch (error) {
      skippedErrors++;
      if (skippedErrors <= 5) {
        console.warn(`⚠️  Failed to stat ${path.relative(IPLUG2_ROOT, filePath)}:`, error instanceof Error ? error.message : String(error));
      }
    }
  }
  
  if (skippedLarge > 0) {
    console.log(`   ⚠️  Skipped ${skippedLarge} files that were too large (>10MB)`);
  }
  if (skippedErrors > 0) {
    console.log(`   ⚠️  Skipped ${skippedErrors} files due to stat errors`);
  }

  console.log(`✅ Discovered ${validFiles.length} valid files (${skippedLarge} too large, ${skippedErrors} errors)`);
  return validFiles;
}

/**
 * Get the file extension that OpenAI supports
 * Converts unsupported extensions to .txt
 */
function getSupportedExtension(filePath: string): string {
  const ext = path.extname(filePath).toLowerCase().slice(1); // Remove the dot
  
  // OpenAI supported formats: c, cpp, cs, css, csv, doc, docx, gif, go, html, java, jpeg, jpg, js, json, md, pdf, php, pkl, png, pptx, py, rb, tar, tex, ts, txt, webp, xlsx, xml, zip
  const supportedExtensions = new Set([
    "c", "cpp", "cs", "css", "csv", "doc", "docx", "gif", "go", "html", 
    "java", "jpeg", "jpg", "js", "json", "md", "pdf", "php", "pkl", "png", 
    "pptx", "py", "rb", "tar", "tex", "ts", "txt", "webp", "xlsx", "xml", "zip"
  ]);
  
  if (supportedExtensions.has(ext)) {
    return ext;
  }
  
  // Convert unsupported extensions to txt
  // Common ones: .h (C++ headers), .mm (Objective-C++), .sh (shell scripts), .hpp, .m (Objective-C)
  return "txt";
}

/**
 * Upload files to vector store
 */
export async function uploadFilesToVectorStore(
  vectorStoreId: string,
  files: string[],
  onProgress?: (uploaded: number, total: number) => void
): Promise<number> {
  console.log(`📤 Uploading ${files.length} files to vector store...`);
  
  if (files.length === 0) {
    throw new Error("No files to upload. File discovery returned 0 files. Check logs above for details.");
  }
  
  const openai = getOpenAIClient();
  const fileIds: string[] = [];
  const batchSize = 100; // OpenAI batch limit
  let uploadErrors = 0;
  let convertedFiles = 0;
  
  for (let i = 0; i < files.length; i += batchSize) {
    const batch = files.slice(i, i + batchSize);
    console.log(`📦 Uploading batch ${Math.floor(i / batchSize) + 1}/${Math.ceil(files.length / batchSize)} (${batch.length} files)...`);
    
    const uploadPromises = batch.map(async (filePath) => {
      try {
        const originalExt = path.extname(filePath).toLowerCase();
        const supportedExt = getSupportedExtension(filePath);
        const needsConversion = originalExt !== `.${supportedExt}`;
        
        if (needsConversion) {
          convertedFiles++;
          if (convertedFiles <= 10) {
            console.log(`  🔄 Converting ${originalExt} -> .${supportedExt}: ${path.relative(IPLUG2_ROOT, filePath)}`);
          }
        }
        
        if (needsConversion) {
          // Read the file content
          let fileContent = await fs.readFile(filePath, "utf-8");
          
          // Add context header about the original extension
          const relPath = path.relative(IPLUG2_ROOT, filePath);
          const contextHeader = `/*
 * Original file: ${relPath}
 * Original extension: ${originalExt}
 * Converted to .txt for OpenAI vector store compatibility
 * 
 * File type context:
${originalExt === '.h' || originalExt === '.hpp' ? ' * C++ header file - Contains class declarations, function prototypes, constants, and type definitions' : ''}
${originalExt === '.mm' ? ' * Objective-C++ source file - Contains implementation mixing Objective-C and C++' : ''}
${originalExt === '.sh' ? ' * Shell script - Contains bash/shell commands for build automation' : ''}
${originalExt === '.m' ? ' * Objective-C source file - Contains Objective-C implementation' : ''}
 */

`;
          
          // Prepend context header to file content
          fileContent = contextHeader + fileContent;
          
          // Use system temp directory for better reliability
          const tempFileName = `iplug2_${Date.now()}_${Math.random().toString(36).substring(7)}.txt`;
          const tempFilePath = path.join(tmpdir(), tempFileName);
          
          try {
            // Write content to temp file with .txt extension
            await fs.writeFile(tempFilePath, fileContent, "utf-8");
            
            // Ensure file is written and closed before reading
            await fs.access(tempFilePath);
            
            // Upload the temp file (SDK will infer .txt from filename)
            const uploadedFile = await openai.files.create({
              file: fsSync.createReadStream(tempFilePath),
              purpose: "assistants",
            });
            
            // Clean up temp file after upload starts
            // Use setTimeout to ensure stream is consumed first
            setTimeout(async () => {
              await fs.unlink(tempFilePath).catch(() => {});
            }, 1000);
            
            return { success: true, id: uploadedFile.id, path: filePath };
          } catch (uploadError) {
            // Clean up temp file on error
            await fs.unlink(tempFilePath).catch(() => {});
            throw uploadError;
          }
        } else {
          // Use original file stream for supported extensions
          const uploadedFile = await openai.files.create({
            file: fsSync.createReadStream(filePath),
            purpose: "assistants",
          });
          
          return { success: true, id: uploadedFile.id, path: filePath };
        }
        
      } catch (error) {
        uploadErrors++;
        const relPath = path.relative(IPLUG2_ROOT, filePath);
        console.warn(`⚠️  Failed to upload ${relPath}:`, error instanceof Error ? error.message : String(error));
        return { success: false, id: null, path: filePath };
      }
    });
    
    const batchResults = await Promise.all(uploadPromises);
    const successful = batchResults.filter(r => r.success);
    const failed = batchResults.filter(r => !r.success);
    
    if (successful.length > 0) {
      fileIds.push(...successful.map(r => r.id!));
      console.log(`  ✅ Uploaded ${successful.length}/${batch.length} files in this batch`);
    }
    
    if (failed.length > 0) {
      console.log(`  ❌ Failed to upload ${failed.length}/${batch.length} files in this batch`);
    }
    
    if (onProgress) {
      onProgress(fileIds.length, files.length);
    }
  }

  if (fileIds.length === 0) {
    console.error(`❌ Upload failed: No files were successfully uploaded`);
    console.error(`   Total files attempted: ${files.length}`);
    console.error(`   Upload errors: ${uploadErrors}`);
    throw new Error(`No files were successfully uploaded. ${uploadErrors} upload errors occurred.`);
  }
  
  if (convertedFiles > 0) {
    console.log(`📝 Converted ${convertedFiles} files to supported formats (.h, .mm, .sh -> .txt)`);
  }
  
  console.log(`✅ Successfully uploaded ${fileIds.length}/${files.length} files (${uploadErrors} errors)`);

  // Add files to vector store in batches
  console.log(`\n📦 Adding ${fileIds.length} files to vector store...`);
  const addBatchSize = 100;
  
  for (let i = 0; i < fileIds.length; i += addBatchSize) {
    const batch = fileIds.slice(i, i + addBatchSize);
    // Add files one by one (OpenAI API requires individual file_id)
    for (const fileId of batch) {
      await openai.vectorStores.files.create(vectorStoreId, {
        file_id: fileId,
      });
    }
    console.log(`  ✅ Added batch ${Math.floor(i / addBatchSize) + 1}/${Math.ceil(fileIds.length / addBatchSize)}`);
  }

  return fileIds.length;
}

/**
 * Wait for vector store to finish processing files
 */
export async function waitForVectorStoreProcessing(
  vectorStoreId: string,
  onStatus?: (status: string) => void
): Promise<void> {
  console.log("⏳ Waiting for vector store processing...");
  
  const openai = getOpenAIClient();
  const startTime = Date.now();
  let checkCount = 0;
  
  while (true) {
    checkCount++;
    const vs = await openai.vectorStores.retrieve(vectorStoreId);
    
    const elapsedSeconds = Math.floor((Date.now() - startTime) / 1000);
    const elapsedMinutes = Math.floor(elapsedSeconds / 60);
    
    // Log status and file counts for debugging
    const statusInfo = vs.file_counts 
      ? ` | Files: ${vs.file_counts.completed || 0}/${(vs.file_counts.completed || 0) + (vs.file_counts.in_progress || 0) + (vs.file_counts.failed || 0)} completed`
      : '';
    console.log(`📊 Check #${checkCount} | Status: ${vs.status}${statusInfo} | Elapsed: ${elapsedMinutes}m ${elapsedSeconds % 60}s`);
    
    if (onStatus) {
      onStatus(vs.status);
    }

    // Check for error states first (before any type narrowing)
    const status = vs.status as string;
    if (status === "expired" || status === "cancelled") {
      throw new Error(`Vector store processing failed with status: ${status}`);
    }

    // Check if processing is complete based on file_counts
    // This is more reliable than status alone - files might be ready even if status isn't "completed"
    if (vs.file_counts) {
      const totalFiles = (vs.file_counts.completed || 0) + (vs.file_counts.in_progress || 0) + (vs.file_counts.failed || 0);
      const completedFiles = vs.file_counts.completed || 0;
      
      // If all files are completed (or failed), we're done
      if (totalFiles > 0 && completedFiles + (vs.file_counts.failed || 0) >= totalFiles) {
        if (vs.file_counts.failed && vs.file_counts.failed > 0) {
          console.log(`⚠️  Vector store processing completed with ${vs.file_counts.failed} failed files`);
        } else {
          console.log(`✅ Vector store processing completed! All ${completedFiles} files processed.`);
        }
        break;
      }
    }

    // Also check status for "completed" (backup check)
    if (vs.status === "completed") {
      console.log("✅ Vector store processing completed!");
      break;
    }

    // If status is not in_progress and not completed, log warning but continue
    if (status !== "in_progress" && status !== "completed") {
      console.warn(`⚠️  Unknown status: ${status}, continuing to wait...`);
    }

    // Wait 5 seconds before checking again
    await new Promise((resolve) => setTimeout(resolve, 5000));
  }
}

/**
 * Initialize vector store with iPlug2 codebase
 */
export async function initializeVectorStore(
  onProgress?: (step: string, progress?: { current: number; total: number }) => void
): Promise<{ vectorStoreId: string; fileCount: number }> {
  // Get or create vector store
  let vectorStoreId = await getIPlug2VectorStore();
  
  if (!vectorStoreId) {
    onProgress?.("Creating vector store...");
    vectorStoreId = await createVectorStore();
  } else {
    onProgress?.("Using existing vector store...");
  }

  // Discover files
  onProgress?.("Discovering files...");
  const files = await discoverIPlug2Files();

  // Upload files
  onProgress?.("Uploading files...", { current: 0, total: files.length });
  const uploadedCount = await uploadFilesToVectorStore(
    vectorStoreId,
    files,
    (uploaded, total) => {
      onProgress?.("Uploading files...", { current: uploaded, total });
    }
  );

  // Wait for processing
  onProgress?.("Processing files...");
  await waitForVectorStoreProcessing(vectorStoreId, (status) => {
    onProgress?.(`Processing files... (${status})`);
  });

  return {
    vectorStoreId,
    fileCount: uploadedCount,
  };
}


