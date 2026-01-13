/**
 * GoldenEffect2 - Simple Gain Controller
 * Runtime parameters for UI binding
 */

export type RuntimeParameter = {
  id: number;
  name: string;
  type: string;
  min: number;
  max: number;
  default: number;
  step: number;
  unit: string;
  group: string;
  shape: string;
  shapeParameter: number;
  enumValues: string[] | null;
  automatable: boolean;
  key: string;
};

export type ControlTag = {
  id: number;
  key: string;
  senderType: 'ISender' | 'IPeakSender' | 'IPeakAvgSender' | 'IBufferSender' | 'ISpectrumSender' | null;
  channels: number | null;
};

export type MsgTag = {
  id: number;
  key: string;
};

export const runtimeParameters: RuntimeParameter[] = [
  {
    "id": 0,
    "name": "Gain",
    "type": "float",
    "min": -60,
    "max": 12,
    "default": 0,
    "step": 0.1,
    "unit": "dB",
    "group": "Main",
    "shape": "ShapeLinear",
    "shapeParameter": 0,
    "enumValues": null,
    "automatable": true,
    "key": "kParamGain"
  }
];

export const controlTags: ControlTag[] = [
  {
    "id": 0,
    "key": "kCtrlTagMeter",
    "senderType": "IPeakAvgSender",
    "channels": 2
  }
];

export const msgTags: MsgTag[] = [];

// Convenience lookups (type-safe)
export const EParams = {
  kParamGain: 0,
} as const;

export const EControlTags = {
  kCtrlTagMeter: 0,
} as const;

export const EMsgTags = {

} as const;

// Type helpers
export type EParamsKey = keyof typeof EParams;
export type EControlTagsKey = keyof typeof EControlTags;
export type EMsgTagsKey = keyof typeof EMsgTags;
