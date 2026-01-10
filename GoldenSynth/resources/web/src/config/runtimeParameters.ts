/**
 * Runtime Parameters
 *
 * Single source of truth for plugin parameters.
 * Must match C++ EParams enum in Plugin.h
 */

/**
 * Parameter shape types - matches iPlug2 IParam::EShapeIDs
 * @see IPlugParameter.h
 */
export type ParameterShape = 'ShapeLinear' | 'ShapePowCurve' | 'ShapeExp';

/**
 * Parameter types - matches iPlug2 IParam::EParamType
 */
export type ParameterType = 'bool' | 'int' | 'enum' | 'float';

export type RuntimeParameter = {
  id: number;
  name: string;
  type: ParameterType;
  min: number;
  max: number;
  default: number;
  step: number;
  unit: string;
  group: string;
  shape: ParameterShape;
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

// Parameters
export const runtimeParameters: RuntimeParameter[] = [
  { id: 0, name: "Gain", type: "float", min: 0, max: 100, default: 80, step: 0.01, unit: "%", group: "Master", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamGain" },
  { id: 1, name: "Waveform", type: "enum", min: 0, max: 6, default: 0, step: 1, unit: "", group: "Oscillator", shape: "ShapeLinear", shapeParameter: 0, enumValues: ["Sine", "Saw", "Square", "Triangle", "Pulse", "FM", "Wavetable"], automatable: true, key: "kParamWaveform" },
  { id: 2, name: "WT Position", type: "float", min: 0, max: 100, default: 0, step: 0.1, unit: "%", group: "Oscillator", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamWavetablePosition" },
  { id: 3, name: "Attack", type: "float", min: 1, max: 1000, default: 10, step: 0.1, unit: "ms", group: "Envelope", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamAttack" },
  { id: 4, name: "Decay", type: "float", min: 1, max: 2000, default: 100, step: 0.1, unit: "ms", group: "Envelope", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamDecay" },
  { id: 5, name: "Sustain", type: "float", min: 0, max: 100, default: 70, step: 0.1, unit: "%", group: "Envelope", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamSustain" },
  { id: 6, name: "Release", type: "float", min: 1, max: 5000, default: 200, step: 0.1, unit: "ms", group: "Envelope", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamRelease" },
  // Filter parameters
  { id: 7, name: "Filter Cutoff", type: "float", min: 20, max: 20000, default: 10000, step: 1, unit: "Hz", group: "Filter", shape: "ShapePowCurve", shapeParameter: 3, enumValues: null, automatable: true, key: "kParamFilterCutoff" },
  { id: 8, name: "Filter Reso", type: "float", min: 0, max: 100, default: 0, step: 0.1, unit: "%", group: "Filter", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamFilterResonance" },
  { id: 9, name: "Filter Type", type: "enum", min: 0, max: 3, default: 0, step: 1, unit: "", group: "Filter", shape: "ShapeLinear", shapeParameter: 0, enumValues: ["Lowpass", "Highpass", "Bandpass", "Notch"], automatable: true, key: "kParamFilterType" },
  // Pulse width modulation
  { id: 10, name: "Pulse Width", type: "float", min: 5, max: 95, default: 50, step: 0.1, unit: "%", group: "Oscillator", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamPulseWidth" },
  // FM synthesis parameters (DX7-style coarse + fine)
  { id: 11, name: "FM Ratio", type: "enum", min: 0, max: 8, default: 2, step: 1, unit: "", group: "Oscillator", shape: "ShapeLinear", shapeParameter: 0, enumValues: ["0.5:1", "1:1", "2:1", "3:1", "4:1", "5:1", "6:1", "7:1", "8:1"], automatable: true, key: "kParamFMRatio" },
  { id: 12, name: "FM Fine", type: "float", min: -50, max: 50, default: 0, step: 0.1, unit: "%", group: "Oscillator", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamFMFine" },
  { id: 13, name: "FM Depth", type: "float", min: 0, max: 100, default: 50, step: 0.1, unit: "%", group: "Oscillator", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamFMDepth" },
];

// Control tags (for SCMFD - meters, etc.)
export const controlTags: ControlTag[] = [
  { id: 0, key: "kCtrlTagMeter", senderType: "IPeakAvgSender", channels: 2 },
  { id: 1, key: "kCtrlTagWaveform", senderType: "IBufferSender", channels: 1 },
];

// Message tags (for SAMFD - visualizations)
export const msgTags: MsgTag[] = [];

// Convenience lookups (must match C++ enums)
export const EParams = {
  kParamGain: 0,
  kParamWaveform: 1,
  kParamWavetablePosition: 2,
  kParamAttack: 3,
  kParamDecay: 4,
  kParamSustain: 5,
  kParamRelease: 6,
  kParamFilterCutoff: 7,
  kParamFilterResonance: 8,
  kParamFilterType: 9,
  kParamPulseWidth: 10,
  kParamFMRatio: 11,
  kParamFMFine: 12,
  kParamFMDepth: 13,
} as const;

export const EControlTags = {
  kCtrlTagMeter: 0,
  kCtrlTagWaveform: 1,
} as const;

export const EMsgTags = {} as const;

// Type helpers
export type EParamsKey = keyof typeof EParams;
export type EControlTagsKey = keyof typeof EControlTags;
export type EMsgTagsKey = keyof typeof EMsgTags;
