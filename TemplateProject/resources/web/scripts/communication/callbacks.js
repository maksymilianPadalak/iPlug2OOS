/**
 * Callback handlers for processor-to-UI communication
 */
import { EControlTags } from '../config/constants';
import { normalizedToDisplay, getParamInputId, getParamValueId } from '../utils/parameter';
let _updatingFromProcessor = false;
function setUpdatingFromProcessor(value) {
    _updatingFromProcessor = value;
}
/**
 * Check if we're updating from processor (to prevent feedback loops)
 */
export function isUpdatingFromProcessor() {
    return _updatingFromProcessor;
}
/**
 * Setup callback handlers for processor-to-UI messages
 */
export function setupCallbacks() {
    // SPVFD: Send Parameter Value From Delegate
    window.SPVFD = (paramIdx, normalizedValue) => {
        updateParameterFromProcessor(paramIdx, normalizedValue);
    };
    // SCVFD: Send Control Value From Delegate
    window.SCVFD = (ctrlTag, normalizedValue) => {
        updateControlValue(ctrlTag, normalizedValue);
    };
    // SCMFD: Send Control Message From Delegate
    window.SCMFD = (ctrlTag, msgTag, dataSize, base64Data) => {
        handleControlMessage(ctrlTag, msgTag, dataSize, base64Data);
    };
    // SAMFD: Send Arbitrary Message From Delegate
    window.SAMFD = (msgTag, dataSize, base64Data) => {
        // Handle arbitrary messages if needed
        console.log('SAMFD received:', msgTag, dataSize, base64Data);
    };
    // SMMFD: Send MIDI Message From Delegate
    window.SMMFD = (statusByte, dataByte1, dataByte2) => {
        // Handle MIDI messages from processor if needed
        console.log('SMMFD received:', statusByte, dataByte1, dataByte2);
    };
    // StartIdleTimer: Start periodic updates
    window.StartIdleTimer = () => {
        startIdleTimer();
    };
}
/**
 * Update parameter from processor (SPVFD)
 */
function updateParameterFromProcessor(paramIdx, normalizedValue) {
    setUpdatingFromProcessor(true);
    try {
        const inputId = getParamInputId(paramIdx);
        const valueId = getParamValueId(paramIdx);
        const inputEl = document.getElementById(inputId);
        const valueEl = document.getElementById(valueId);
        if (inputEl) {
            if (inputEl instanceof HTMLInputElement && inputEl.type === 'range') {
                inputEl.value = normalizedValue.toString();
            }
            else if (inputEl instanceof HTMLSelectElement) {
                inputEl.selectedIndex = Math.round(normalizedValue);
            }
            else if (inputEl instanceof HTMLInputElement && inputEl.type === 'checkbox') {
                inputEl.checked = normalizedValue > 0.5;
            }
            // Update display value
            if (valueEl) {
                valueEl.textContent = normalizedToDisplay(paramIdx, normalizedValue);
            }
        }
    }
    catch (e) {
        console.error('Error updating parameter:', paramIdx, e);
    }
    finally {
        setUpdatingFromProcessor(false);
    }
}
/**
 * Update control value from processor (SCVFD)
 */
function updateControlValue(ctrlTag, normalizedValue) {
    switch (ctrlTag) {
        case EControlTags.kCtrlTagLFOVis:
            // Handle LFO visualization update
            // This will be handled by SCMFD for waveform data
            break;
        default:
            console.log('Unknown control tag:', ctrlTag);
    }
}
/**
 * Handle control message from processor (SCMFD)
 */
function handleControlMessage(ctrlTag, msgTag, dataSize, base64Data) {
    if (ctrlTag === EControlTags.kCtrlTagLFOVis) {
        // Decode LFO waveform data
        try {
            const binaryString = atob(base64Data);
            const buffer = new ArrayBuffer(binaryString.length);
            const view = new Uint8Array(buffer);
            for (let i = 0; i < binaryString.length; i++) {
                view[i] = binaryString.charCodeAt(i);
            }
            const dataView = new DataView(buffer);
            const lfoValue = dataView.getFloat32(12, true); // vals[0] is at offset 12
            // Update LFO waveform - this will be handled by the LFO visualizer module
            if (window.updateLFOWaveform) {
                window.updateLFOWaveform(lfoValue);
            }
        }
        catch (e) {
            console.error('Error decoding LFO data:', e);
        }
    }
    else if (ctrlTag === EControlTags.kCtrlTagMeter) {
        // Decode meter data (peak/rms values)
        try {
            const binaryString = atob(base64Data);
            const buffer = new ArrayBuffer(binaryString.length);
            const view = new Uint8Array(buffer);
            for (let i = 0; i < binaryString.length; i++) {
                view[i] = binaryString.charCodeAt(i);
            }
            const dataView = new DataView(buffer);
            const leftPeak = dataView.getFloat32(12, true);
            const leftRMS = dataView.getFloat32(16, true);
            const rightPeak = dataView.getFloat32(20, true);
            const rightRMS = dataView.getFloat32(24, true);
            // Update meters - this will be handled by the meters module
            if (window.updateMeter) {
                window.updateMeter(0, leftPeak, leftRMS);
                window.updateMeter(1, rightPeak, rightRMS);
            }
        }
        catch (e) {
            console.error('Error decoding meter data:', e);
        }
    }
}
let idleTimerInterval = null;
/**
 * Start idle timer for periodic updates
 */
function startIdleTimer() {
    console.log('StartIdleTimer called - setting up periodic TICK messages');
    if (idleTimerInterval) {
        clearInterval(idleTimerInterval);
    }
    // Send TICK messages periodically to trigger OnIdle() in processor
    idleTimerInterval = window.setInterval(() => {
        if (window.TemplateProject_WAM && typeof window.TemplateProject_WAM.sendMessage === 'function') {
            window.TemplateProject_WAM.sendMessage('TICK', '', 0);
        }
    }, 16); // ~60fps
}
//# sourceMappingURL=callbacks.js.map