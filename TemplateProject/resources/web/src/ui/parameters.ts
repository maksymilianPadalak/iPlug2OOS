/**
 * Parameter UI management
 */

import { EParams } from '../config/constants';
import { sendParameterValue, sendParameterEnum } from '../communication/iplug-bridge';
import { isUpdatingFromProcessor } from '../communication/callbacks';
import { normalizedToDisplay, getParamValueId } from '../utils/parameter';

/**
 * Update parameter value from UI interaction
 */
export function updateParam(paramIdx: EParams, normalizedValue: number): void {
  if (!isUpdatingFromProcessor()) {
    sendParameterValue(paramIdx, normalizedValue);
  }
  updateParamDisplay(paramIdx, normalizedValue);
}

/**
 * Update parameter enum value from UI interaction
 */
export function updateParamEnum(paramIdx: EParams, enumValue: number): void {
  if (!isUpdatingFromProcessor()) {
    sendParameterEnum(paramIdx, enumValue);
  }
}

/**
 * Update parameter display value
 */
export function updateParamDisplay(paramIdx: EParams, normalizedValue: number): void {
  const valueId = getParamValueId(paramIdx);
  const valueEl = document.getElementById(valueId);
  if (valueEl) {
    valueEl.textContent = normalizedToDisplay(paramIdx, normalizedValue);
  }
}

/**
 * Toggle LFO sync mode
 */
export function toggleLFOSync(): void {
  const syncCheckbox = document.getElementById('paramLFOSync') as HTMLInputElement;
  if (!syncCheckbox) return;

  const syncValue = syncCheckbox.checked ? 1.0 : 0.0;
  updateParam(EParams.kParamLFORateMode, syncValue);

  // Toggle visibility of Hz vs Tempo controls
  const hzContainer = document.getElementById('lfoRateHzContainer');
  const tempoContainer = document.getElementById('lfoRateTempoContainer');
  
  if (hzContainer && tempoContainer) {
    if (syncCheckbox.checked) {
      hzContainer.style.display = 'none';
      tempoContainer.style.display = 'block';
    } else {
      hzContainer.style.display = 'block';
      tempoContainer.style.display = 'none';
    }
  }
}

