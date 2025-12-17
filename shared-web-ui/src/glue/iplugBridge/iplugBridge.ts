/**
 * iPlug2 communication bridge
 */

import { IPlugUIMessage } from "../../types/iplug";
import { MessageTypes } from "../../config/protocol";

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
export function sendParameterValue(paramIdx: number, normalizedValue: number): void {
  sendIPlugMessage({
    msg: MessageTypes.SPVFUI,
    paramIdx,
    value: normalizedValue,
  });
}

/**
 * Send parameter enum value update
 */
export function sendParameterEnum(paramIdx: number, enumValue: number): void {
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

/**
 * Begin parameter change - signals to host that automation recording should start.
 * MUST be called before any parameter value changes during a user gesture.
 * @param paramIdx - The parameter index (from EParams enum)
 */
export function beginParameterChange(paramIdx: number): void {
  sendIPlugMessage({
    msg: MessageTypes.BPCFUI,
    paramIdx,
  });
}

/**
 * End parameter change - signals to host that automation recording should end.
 * MUST be called after user gesture completes (mouseup, touchend, etc.)
 * @param paramIdx - The parameter index (from EParams enum)
 */
export function endParameterChange(paramIdx: number): void {
  sendIPlugMessage({
    msg: MessageTypes.EPCFUI,
    paramIdx,
  });
}

/**
 * Send arbitrary binary data to processor.
 * Use for custom data like waveform uploads, preset data, etc.
 * @param msgTag - Message type identifier (from EMsgTags enum)
 * @param ctrlTag - Control tag identifier (from EControlTags enum)
 * @param data - Binary data as ArrayBuffer or number array
 */
export function sendArbitraryMessage(
  msgTag: number,
  ctrlTag: number,
  data: ArrayBuffer | number[],
): void {
  let base64Data: string;
  if (data instanceof ArrayBuffer) {
    const bytes = new Uint8Array(data);
    base64Data = btoa(String.fromCharCode(...bytes));
  } else {
    base64Data = btoa(String.fromCharCode(...data));
  }

  sendIPlugMessage({
    msg: MessageTypes.SAMFUI,
    msgTag,
    ctrlTag,
    data: base64Data,
  });
}

/**
 * Send keyboard event to processor.
 * Use for plugin shortcuts/hotkeys when not in a text input.
 * @param keyCode - DOM key code
 * @param utf8 - UTF8 character string
 * @param shift - Shift key pressed
 * @param ctrl - Ctrl/Cmd key pressed
 * @param alt - Alt/Option key pressed
 * @param isUp - True for keyup, false for keydown
 */
export function sendKeyPress(
  keyCode: number,
  utf8: string,
  shift: boolean,
  ctrl: boolean,
  alt: boolean,
  isUp: boolean,
): void {
  sendIPlugMessage({
    msg: MessageTypes.SKPFUI,
    keyCode,
    utf8,
    S: shift,
    C: ctrl,
    A: alt,
    isUp,
  });
}

/**
 * Request full state sync from DSP.
 * Call on WebView load or after connection recovery.
 * DSP will respond with SSTATE callback containing all parameter values.
 */
export function requestStateSync(): void {
  sendIPlugMessage({ msg: MessageTypes.SREQ });
}
