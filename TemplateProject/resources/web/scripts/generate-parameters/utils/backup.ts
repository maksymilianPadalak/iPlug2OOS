/**
 * Backup utilities for parameter generation
 */

import fs from "fs/promises";
import path from "path";
import { BACKUP_DIR } from "../config.js";

/**
 * Create a backup of a file before overwriting
 * @param filePath Path to the file to backup
 * @returns Path to the backup file
 */
export async function createBackup(filePath: string): Promise<string> {
  // Ensure backup directory exists
  await fs.mkdir(BACKUP_DIR, { recursive: true });

  // Generate backup filename with timestamp
  const timestamp = new Date().toISOString().replace(/[:.]/g, "-");
  const fileName = path.basename(filePath);
  const backupFileName = `${fileName}.backup.${timestamp}`;
  const backupPath = path.join(BACKUP_DIR, backupFileName);

  // Copy file to backup location
  await fs.copyFile(filePath, backupPath);

  console.log(`📦 Created backup: ${backupPath}`);
  return backupPath;
}

/**
 * Remove a backup file
 * @param backupPath Path to the backup file
 */
export async function removeBackup(backupPath: string): Promise<void> {
  try {
    await fs.unlink(backupPath);
    console.log(`✅ Removed backup: ${backupPath}`);
  } catch (error) {
    console.warn(`⚠️ Failed to remove backup ${backupPath}:`, error);
  }
}

/**
 * Restore a file from backup
 * @param backupPath Path to the backup file
 * @param originalPath Path to restore to
 */
export async function restoreBackup(
  backupPath: string,
  originalPath: string,
): Promise<void> {
  await fs.copyFile(backupPath, originalPath);
  console.log(`🔄 Restored from backup: ${originalPath}`);
}

/**
 * Remove all backups older than specified days (default: 7)
 * @param daysOld Number of days old backups to keep
 */
export async function cleanupOldBackups(daysOld: number = 7): Promise<void> {
  try {
    const files = await fs.readdir(BACKUP_DIR);
    const now = Date.now();
    const maxAge = daysOld * 24 * 60 * 60 * 1000;

    for (const file of files) {
      const filePath = path.join(BACKUP_DIR, file);
      const stats = await fs.stat(filePath);
      const age = now - stats.mtimeMs;

      if (age > maxAge) {
        await fs.unlink(filePath);
        console.log(`🗑️ Removed old backup: ${file}`);
      }
    }
  } catch (error) {
    // Backup directory might not exist, ignore
    if ((error as NodeJS.ErrnoException).code !== "ENOENT") {
      console.warn(`⚠️ Failed to cleanup backups:`, error);
    }
  }
}

