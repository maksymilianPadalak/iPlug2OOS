/**
 * Pure transport layer for DSP-UI communication.
 *
 * This module is AGNOSTIC - it has no knowledge of specific message types,
 * enum values, or binary formats. It simply:
 * 1. Receives messages from iPlug2
 * 2. Decodes base64 payloads
 * 3. Passes raw data to registered handlers
 *
 * All parsing and routing logic belongs in BridgeProvider.
 */

import { reportBridgeError } from "../errors/bridgeErrors";
import type { ProcessorEventHandlers } from "./types";

let updatingFromProcessor = false;
let handlers: ProcessorEventHandlers | null = null;

const microtask =
  typeof queueMicrotask === "function"
    ? queueMicrotask
    : (fn: () => void) => {
        Promise.resolve().then(fn);
      };

/** Decode base64 string to ArrayBuffer */
function decodeBase64(data: string): ArrayBuffer {
  const binary = atob(data);
  const buffer = new ArrayBuffer(binary.length);
  const view = new Uint8Array(buffer);
  for (let i = 0; i < binary.length; i++) {
    view[i] = binary.charCodeAt(i);
  }
  return buffer;
}

export function registerProcessorCallbacks(
  newHandlers: ProcessorEventHandlers
): void {
  handlers = newHandlers;

  // SPVFD: Parameter value from delegate
  window.SPVFD = (paramIdx: number, normalizedValue: number) => {
    if (!handlers?.onParameterValue) return;
    try {
      updatingFromProcessor = true;
      handlers.onParameterValue(paramIdx, normalizedValue);
      microtask(() => {
        updatingFromProcessor = false;
      });
    } catch (error) {
      reportBridgeError(
        "HANDLER_THREW",
        error instanceof Error ? error.message : "onParameterValue error",
        { paramIdx, error }
      );
    }
  };

  // SCVFD: Control value from delegate
  window.SCVFD = (ctrlTag: number, normalizedValue: number) => {
    if (!handlers?.onControlValue) return;
    try {
      handlers.onControlValue(ctrlTag, normalizedValue);
    } catch (error) {
      reportBridgeError(
        "HANDLER_THREW",
        error instanceof Error ? error.message : "onControlValue error",
        { ctrlTag, error }
      );
    }
  };

  // SCMFD: Control message from delegate
  window.SCMFD = (
    ctrlTag: number,
    msgTag: number,
    dataSize: number,
    base64Data: string
  ) => {
    if (!handlers?.onControlMessage) return;
    try {
      const buffer = decodeBase64(base64Data);
      if (buffer.byteLength < dataSize) {
        reportBridgeError(
          "MESSAGE_MALFORMED",
          `Buffer size mismatch: expected ${dataSize}, got ${buffer.byteLength}`,
          { ctrlTag, msgTag }
        );
        return;
      }
      handlers.onControlMessage(ctrlTag, msgTag, dataSize, buffer);
    } catch (error) {
      reportBridgeError(
        "HANDLER_THREW",
        error instanceof Error ? error.message : "onControlMessage error",
        { ctrlTag, msgTag, error }
      );
    }
  };

  // SAMFD: Arbitrary message from delegate
  window.SAMFD = (msgTag: number, dataSize: number, base64Data: string) => {
    if (!handlers?.onArbitraryMessage) return;
    try {
      const buffer = decodeBase64(base64Data);
      handlers.onArbitraryMessage(msgTag, dataSize, buffer);
    } catch (error) {
      reportBridgeError(
        "HANDLER_THREW",
        error instanceof Error ? error.message : "onArbitraryMessage error",
        { msgTag, error }
      );
    }
  };

  // SMMFD: MIDI message from delegate
  window.SMMFD = (statusByte: number, dataByte1: number, dataByte2: number) => {
    if (!handlers?.onMidiMessage) return;
    try {
      handlers.onMidiMessage(statusByte, dataByte1, dataByte2);
    } catch (error) {
      reportBridgeError(
        "HANDLER_THREW",
        error instanceof Error ? error.message : "onMidiMessage error",
        { statusByte, error }
      );
    }
  };

  // SSMFD: SysEx message from delegate
  window.SSMFD = (_dataSize: number, base64Data: string) => {
    if (!handlers?.onSysexMessage) return;
    try {
      const buffer = decodeBase64(base64Data);
      handlers.onSysexMessage(buffer);
    } catch (error) {
      reportBridgeError(
        "HANDLER_THREW",
        error instanceof Error ? error.message : "onSysexMessage error",
        { error }
      );
    }
  };

  // SSTATE: Full state dump from delegate
  window.SSTATE = (base64Data: string) => {
    if (!handlers?.onStateDump) return;
    try {
      const buffer = decodeBase64(base64Data);
      updatingFromProcessor = true;
      handlers.onStateDump(buffer);
      microtask(() => {
        updatingFromProcessor = false;
      });
    } catch (error) {
      reportBridgeError(
        "HANDLER_THREW",
        error instanceof Error ? error.message : "onStateDump error",
        { error }
      );
    }
  };

  // StartIdleTimer: Begin periodic TICK messages
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

let idleTimerInterval: number | null = null;

function startIdleTimer(): void {
  console.log("StartIdleTimer called - setting up periodic TICK messages");

  if (idleTimerInterval) {
    clearInterval(idleTimerInterval);
  }

  idleTimerInterval = window.setInterval(() => {
    if (
      window.Plugin_WAM &&
      typeof window.Plugin_WAM.sendMessage === "function"
    ) {
      window.Plugin_WAM.sendMessage("TICK", "", 0);
    }
  }, 16);
}
