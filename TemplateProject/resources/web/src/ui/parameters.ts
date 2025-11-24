/**
 * Parameter UI management
 */

import { EParams } from '../config/constants';
import { sendParameterEnum, sendParameterValue } from '../glue/iplugBridge/iplugBridge';
import { isUpdatingFromProcessor } from '../glue/processorCallbacks/processorCallbacks';
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


