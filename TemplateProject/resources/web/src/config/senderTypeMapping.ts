/**
 * Single source of truth for sender type → visualization component mapping.
 *
 * Used by:
 * - BridgeProvider.tsx (runtime message routing)
 * - uiManifest.ts (component metadata for AI)
 * - operationRules.ts (AI prompt rules)
 */

export type SenderType =
  | 'ISender'
  | 'IPeakSender'
  | 'IPeakAvgSender'
  | 'IBufferSender'
  | 'ISpectrumSender';

export type VisualizationComponent = 'Meter' | 'WaveformDisplay';

/**
 * Maps sender types to their corresponding visualization components.
 * null means no visualization component exists for this sender type yet.
 */
export const SENDER_TO_VISUALIZATION: Record<SenderType, VisualizationComponent | null> = {
  ISender: null,              // Generic sender - no standard visualization
  IPeakSender: 'Meter',       // Single peak value → Meter
  IPeakAvgSender: 'Meter',    // Peak + RMS values → Meter
  IBufferSender: 'WaveformDisplay',  // Sample buffer → WaveformDisplay
  ISpectrumSender: null,      // FFT bins - no component yet
};

/**
 * Returns the visualization component for a given sender type.
 */
export function getVisualizationForSenderType(
  senderType: SenderType | null
): VisualizationComponent | null {
  if (!senderType) return null;
  return SENDER_TO_VISUALIZATION[senderType];
}

/**
 * Returns all sender types that map to a given visualization component.
 */
export function getSenderTypesForVisualization(
  component: VisualizationComponent
): SenderType[] {
  return (Object.entries(SENDER_TO_VISUALIZATION) as [SenderType, VisualizationComponent | null][])
    .filter(([, viz]) => viz === component)
    .map(([senderType]) => senderType);
}
