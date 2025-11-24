/**
 * Callback handlers for processor-to-UI communication
 */

import { EParams, EControlTags } from "../../config/constants";
import { normalizedToDisplay, getParamInputId, getParamValueId } from "../../utils/parameter";

let updatingFromProcessor = false;

function setUpdatingFromProcessor(value: boolean): void {
  updatingFromProcessor = value;
}

/**
 * Check if we're updating from processor (to prevent feedback loops)
 */
export function isUpdatingFromProcessor(): boolean {
  return updatingFromProcessor;
}

/**
 * Setup callback handlers for processor-to-UI messages
 */
export function setupCallbacks(): void {
  // SPVFD: Send Parameter Value From Delegate
  window.SPVFD = (paramIdx: number, normalizedValue: number) => {
    updateParameterFromProcessor(paramIdx, normalizedValue);
  };

  // SCVFD: Send Control Value From Delegate
  window.SCVFD = (ctrlTag: number, normalizedValue: number) => {
    updateControlValue(ctrlTag, normalizedValue);
  };

  // SCMFD: Send Control Message From Delegate
  window.SCMFD = (ctrlTag: number, msgTag: number, dataSize: number, base64Data: string) => {
    handleControlMessage(ctrlTag, msgTag, dataSize, base64Data);
  };

  // SAMFD: Send Arbitrary Message From Delegate
  window.SAMFD = (msgTag: number, dataSize: number, base64Data: string) => {
    console.log("SAMFD received:", msgTag, dataSize, base64Data);
  };

  // SMMFD: Send MIDI Message From Delegate
  window.SMMFD = (statusByte: number, dataByte1: number, dataByte2: number) => {
    console.log("SMMFD received:", statusByte, dataByte1, dataByte2);
  };

  // StartIdleTimer: Start periodic updates
  window.StartIdleTimer = () => {
    startIdleTimer();
  };
}

/**
 * Update parameter from processor (SPVFD)
 */
function updateParameterFromProcessor(paramIdx: number, normalizedValue: number): void {
  setUpdatingFromProcessor(true);

  try {
    const inputId = getParamInputId(paramIdx as EParams);
    const valueId = getParamValueId(paramIdx as EParams);
    const inputEl = document.getElementById(inputId) as HTMLInputElement | HTMLSelectElement | null;
    const valueEl = document.getElementById(valueId);

    if (inputEl) {
      if (inputEl instanceof HTMLInputElement && inputEl.type === "range") {
        inputEl.value = normalizedValue.toString();
      } else if (inputEl instanceof HTMLSelectElement) {
        inputEl.selectedIndex = Math.round(normalizedValue);
      } else if (inputEl instanceof HTMLInputElement && inputEl.type === "checkbox") {
        inputEl.checked = normalizedValue > 0.5;
      }

      if (valueEl) {
        valueEl.textContent = normalizedToDisplay(paramIdx as EParams, normalizedValue);
      }
    }
  } catch (error) {
    console.error("Error updating parameter:", paramIdx, error);
  } finally {
    setUpdatingFromProcessor(false);
  }
}

/**
 * Update control value from processor (SCVFD)
 */
function updateControlValue(ctrlTag: number, normalizedValue: number): void {
  switch (ctrlTag) {
    case EControlTags.kCtrlTagMeter:
      break;
    default:
      console.log("Unknown control tag:", ctrlTag);
  }
}

/**
 * Handle control message from processor (SCMFD)
 */
function handleControlMessage(
  ctrlTag: number,
  msgTag: number,
  dataSize: number,
  base64Data: string,
): void {
  if (ctrlTag === EControlTags.kCtrlTagMeter) {
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

      if (window.updateMeter) {
        window.updateMeter(0, leftPeak, leftRMS);
        window.updateMeter(1, rightPeak, rightRMS);
      }
    } catch (error) {
      console.error("Error decoding meter data:", error);
    }
  }
}

let idleTimerInterval: number | null = null;

/**
 * Start idle timer for periodic updates
 */
function startIdleTimer(): void {
  console.log("StartIdleTimer called - setting up periodic TICK messages");

  if (idleTimerInterval) {
    clearInterval(idleTimerInterval);
  }

  idleTimerInterval = window.setInterval(() => {
    if (window.TemplateProject_WAM && typeof window.TemplateProject_WAM.sendMessage === "function") {
      window.TemplateProject_WAM.sendMessage("TICK", "", 0);
    }
  }, 16);
}

