#!/usr/bin/env node
/**
 * Initialize iPlug2 Vector Store
 * 
 * Creates and populates an OpenAI vector store with the iPlug2 codebase.
 * This enables semantic search for improved DSP code generation.
 * 
 * Usage: npm run init-vector-store
 */

import {
  initializeVectorStore,
  getIPlug2VectorStore,
} from "./generate-parameters/utils/vector-store.js";

async function main() {
  console.log("🚀 Initializing iPlug2 Vector Store...\n");

  try {
    // Check if vector store already exists
    const existingId = await getIPlug2VectorStore();
    if (existingId) {
      console.log(`✅ Vector store already exists: ${existingId}`);
      console.log("💡 To re-upload files, delete .vector-store-config.json and run again.\n");
      return;
    }

    // Initialize vector store
    const result = await initializeVectorStore((step, progress) => {
      if (progress) {
        const percent = Math.round((progress.current / progress.total) * 100);
        console.log(`${step} (${progress.current}/${progress.total} - ${percent}%)`);
      } else {
        console.log(step);
      }
    });

    console.log("\n✅ Vector store initialization complete!");
    console.log(`📦 Vector Store ID: ${result.vectorStoreId}`);
    console.log(`📁 Files uploaded: ${result.fileCount}`);
    console.log("\n💡 The vector store ID has been saved and will be reused automatically.");
  } catch (error) {
    console.error("\n❌ Failed to initialize vector store:");
    console.error(error instanceof Error ? error.message : String(error));
    if (error instanceof Error && error.stack) {
      console.error("\nStack trace:");
      console.error(error.stack);
    }
    process.exit(1);
  }
}

main();

