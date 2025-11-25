export type ProcessorEventHandlers = {
  onParameterValue?: (paramIdx: number, normalizedValue: number) => void;
  onControlValue?: (ctrlTag: number, normalizedValue: number) => void;
  onMeterData?: (channel: number, peak: number, rms: number) => void;
};


