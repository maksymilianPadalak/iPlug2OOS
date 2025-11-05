/**
 * iPlug2 communication bridge
 */
import { MessageTypes } from '../config/constants';
/**
 * Send a message to the iPlug2 processor
 */
export function sendIPlugMessage(message) {
    if (typeof window.IPlugSendMsg === 'function') {
        window.IPlugSendMsg(message);
    }
    else {
        console.warn('IPlugSendMsg not available');
    }
}
/**
 * Send parameter value update
 */
export function sendParameterValue(paramIdx, normalizedValue) {
    sendIPlugMessage({
        msg: MessageTypes.SPVFUI,
        paramIdx,
        value: normalizedValue
    });
}
/**
 * Send parameter enum value update
 */
export function sendParameterEnum(paramIdx, enumValue) {
    sendIPlugMessage({
        msg: MessageTypes.SPVFUI,
        paramIdx,
        value: enumValue
    });
}
/**
 * Send MIDI message
 */
export function sendMIDIMessage(statusByte, dataByte1, dataByte2) {
    sendIPlugMessage({
        msg: MessageTypes.SMMFUI,
        statusByte,
        dataByte1,
        dataByte2
    });
}
/**
 * Send note on
 */
export function sendNoteOn(noteNumber, velocity = 127) {
    sendMIDIMessage(0x90, noteNumber, velocity);
}
/**
 * Send note off
 */
export function sendNoteOff(noteNumber, velocity = 0) {
    sendMIDIMessage(0x80, noteNumber, velocity);
}
/**
 * Check if IPlugSendMsg is available
 */
export function isIPlugAvailable() {
    return typeof window.IPlugSendMsg === 'function';
}
//# sourceMappingURL=iplug-bridge.js.map