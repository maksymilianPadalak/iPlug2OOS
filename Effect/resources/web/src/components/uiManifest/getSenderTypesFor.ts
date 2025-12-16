/**
 * getSenderTypesFor - Maps visualization components to their DSP sender types
 *
 * Used by componentRegistry to associate visualizations (Meter, WaveformDisplay)
 * with the DSP sender types that provide their data (kSenderTypeMeter, kSenderTypeWaveform).
 */

import { SENDER_TO_VISUALIZATION } from '../../config/senderTypeMapping';
import type { SenderType, VisualizationComponent } from '../../config/senderTypeMapping';

/** Get all sender types that map to a given visualization component */
export function getSenderTypesFor(component: VisualizationComponent): SenderType[] {
  return (Object.entries(SENDER_TO_VISUALIZATION) as [SenderType, VisualizationComponent | null][])
    .filter(([, viz]) => viz === component)
    .map(([senderType]) => senderType);
}
