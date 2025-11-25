export type ProcessorEventHandlers = {
  onParameterValue?: (paramIdx: number, normalizedValue: number) => void;
  onControlValue?: (ctrlTag: number, normalizedValue: number) => void;
  onMeterData?: (channel: number, peak: number, rms: number) => void;
  onArbitraryMessage?: (msgTag: number, dataSize: number, data: ArrayBuffer) => void;
  onMidiMessage?: (statusByte: number, dataByte1: number, dataByte2: number) => void;
};




