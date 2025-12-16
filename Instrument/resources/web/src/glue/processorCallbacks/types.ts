/**
 * Pure transport layer handlers - no knowledge of specific message formats.
 * All parsing/routing happens in BridgeProvider.
 */
export type ProcessorEventHandlers = {
  /** SPVFD: Parameter value from DSP */
  onParameterValue?: (paramIdx: number, normalizedValue: number) => void;
  /** SCVFD: Control value (normalized) from DSP */
  onControlValue?: (ctrlTag: number, normalizedValue: number) => void;
  /** SCMFD: Control message with binary data */
  onControlMessage?: (ctrlTag: number, msgTag: number, dataSize: number, data: ArrayBuffer) => void;
  /** SAMFD: Arbitrary message with binary data */
  onArbitraryMessage?: (msgTag: number, dataSize: number, data: ArrayBuffer) => void;
  /** SMMFD: MIDI message */
  onMidiMessage?: (statusByte: number, dataByte1: number, dataByte2: number) => void;
  /** SSMFD: System Exclusive message */
  onSysexMessage?: (data: ArrayBuffer) => void;
  /** SSTATE: Full state dump (raw binary) */
  onStateDump?: (data: ArrayBuffer) => void;
};




