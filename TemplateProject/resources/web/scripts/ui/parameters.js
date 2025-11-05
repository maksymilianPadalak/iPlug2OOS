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
export function updateParam(paramIdx, normalizedValue) {
    if (!isUpdatingFromProcessor()) {
        sendParameterValue(paramIdx, normalizedValue);
    }
    updateParamDisplay(paramIdx, normalizedValue);
}
/**
 * Update parameter enum value from UI interaction
 */
export function updateParamEnum(paramIdx, enumValue) {
    if (!isUpdatingFromProcessor()) {
        sendParameterEnum(paramIdx, enumValue);
    }
}
/**
 * Update parameter display value
 */
export function updateParamDisplay(paramIdx, normalizedValue) {
    const valueId = getParamValueId(paramIdx);
    const valueEl = document.getElementById(valueId);
    if (valueEl) {
        valueEl.textContent = normalizedToDisplay(paramIdx, normalizedValue);
    }
}
/**
 * Toggle LFO sync mode
 */
export function toggleLFOSync() {
    const syncCheckbox = document.getElementById('paramLFOSync');
    if (!syncCheckbox)
        return;
    const syncValue = syncCheckbox.checked ? 1.0 : 0.0;
    updateParam(EParams.kParamLFORateMode, syncValue);
    // Toggle visibility of Hz vs Tempo controls
    const hzContainer = document.getElementById('lfoRateHzContainer');
    const tempoContainer = document.getElementById('lfoRateTempoContainer');
    if (hzContainer && tempoContainer) {
        if (syncCheckbox.checked) {
            hzContainer.style.display = 'none';
            tempoContainer.style.display = 'block';
        }
        else {
            hzContainer.style.display = 'block';
            tempoContainer.style.display = 'none';
        }
    }
}
//# sourceMappingURL=parameters.js.map