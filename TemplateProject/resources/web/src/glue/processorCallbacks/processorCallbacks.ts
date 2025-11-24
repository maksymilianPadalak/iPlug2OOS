/**
 * Callback handlers for processor-to-UI communication
 */

import { EControlTags } from "../../config/constants";
import type { ProcessorEventHandlers } from "./types";

let updatingFromProcessor = false;
let handlers: ProcessorEventHandlers | null = null;

const microtask = typeof queueMicrotask === "function" ? queueMicrotask : (fn: () => void) => {
  Promise.resolve().then(fn);
};

export function registerProcessorCallbacks(newHandlers: ProcessorEventHandlers): void {
  handlers = newHandlers;

  window.SPVFD = (paramIdx: number, normalizedValue: number) => {
    updateParameterFromProcessor(paramIdx, normalizedValue);
  };

  window.SCVFD = (ctrlTag: number, normalizedValue: number) => {
    updateControlValue(ctrlTag, normalizedValue);
  };

  window.SCMFD = (ctrlTag: number, msgTag: number, dataSize: number, base64Data: string) => {
    handleControlMessage(ctrlTag, msgTag, dataSize, base64Data);
  };

  window.SAMFD = (msgTag: number, dataSize: number, base64Data: string) => {
    console.log("SAMFD received:", msgTag, dataSize, base64Data);
  };

  window.SMMFD = (statusByte: number, dataByte1: number, dataByte2: number) => {
    console.log("SMMFD received:", statusByte, dataByte1, dataByte2);
  };

  window.StartIdleTimer = () => {
    startIdleTimer();
  };
}

export function unregisterProcessorCallbacks(): void {
  handlers = null;
}

export function isUpdatingFromProcessor(): boolean {
  return updatingFromProcessor;
}

function updateParameterFromProcessor(paramIdx: number, normalizedValue: number): void {
  if (!handlers?.onParameterValue) {
    return;
  }

  updatingFromProcessor = true;
  handlers.onParameterValue(paramIdx, normalizedValue);

  microtask(() => {
    updatingFromProcessor = false;
  });
}

function updateControlValue(ctrlTag: number, normalizedValue: number): void {
  handlers?.onControlValue?.(ctrlTag, normalizedValue);
}

function handleControlMessage(
  ctrlTag: number,
  _msgTag: number,
  _dataSize: number,
  base64Data: string,
): void {
  if (!handlers?.onMeterData || ctrlTag !== EControlTags.kCtrlTagMeter) {
    return;
  }

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

    handlers.onMeterData(0, leftPeak, leftRMS);
    handlers.onMeterData(1, rightPeak, rightRMS);
  } catch (error) {
    console.error("Error decoding meter data:", error);
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

