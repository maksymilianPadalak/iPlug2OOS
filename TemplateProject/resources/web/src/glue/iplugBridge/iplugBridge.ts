/**
 * iPlug2 communication bridge
 */

import { IPlugUIMessage } from "../../types/iplug";
import { MessageTypes, EParams } from "../../config/constants";

/**
 * Send a message to the iPlug2 processor
 */
export function sendIPlugMessage(message: IPlugUIMessage): void {
  if (typeof window.IPlugSendMsg === "function") {
    window.IPlugSendMsg(message);
  } else {
    console.warn("IPlugSendMsg not available");
  }
}

/**
 * Send parameter value update
 */
export function sendParameterValue(paramIdx: EParams, normalizedValue: number): void {
  sendIPlugMessage({
    msg: MessageTypes.SPVFUI,
    paramIdx,
    value: normalizedValue,
  });
}

/**
 * Send parameter enum value update
 */
export function sendParameterEnum(paramIdx: EParams, enumValue: number): void {
  sendIPlugMessage({
    msg: MessageTypes.SPVFUI,
    paramIdx,
    value: enumValue,
  });
}

/**
 * Send MIDI message
 */
export function sendMIDIMessage(statusByte: number, dataByte1: number, dataByte2: number): void {
  sendIPlugMessage({
    msg: MessageTypes.SMMFUI,
    statusByte,
    dataByte1,
    dataByte2,
  });
}

/**
 * Send note on
 */
export function sendNoteOn(noteNumber: number, velocity: number = 127): void {
  sendMIDIMessage(0x90, noteNumber, velocity);
}

/**
 * Send note off
 */
export function sendNoteOff(noteNumber: number, velocity: number = 0): void {
  sendMIDIMessage(0x80, noteNumber, velocity);
}

/**
 * Check if IPlugSendMsg is available
 */
export function isIPlugAvailable(): boolean {
  return typeof window.IPlugSendMsg === "function";
}

