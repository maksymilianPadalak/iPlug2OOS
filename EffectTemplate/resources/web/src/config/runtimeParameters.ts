/**
 * Runtime Parameters (Reverb + Delay Effect)
 *
 * Single source of truth for plugin parameters.
 * Must match C++ EParams enum in EffectTemplate.h
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

// Parameters - must match C++ EParams enum order
export const runtimeParameters: RuntimeParameter[] = [
  { id: 0, name: "Gain", type: "float", min: 0, max: 200, default: 100, step: 0.01, unit: "%", group: "Output", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamGain" },
  { id: 1, name: "Mix", type: "float", min: 0, max: 100, default: 50, step: 0.1, unit: "%", group: "Master", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamMix" },
  { id: 2, name: "Delay Time", type: "float", min: 0, max: 1000, default: 250, step: 1, unit: "ms", group: "Delay", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamDelayTime" },
  { id: 3, name: "Feedback", type: "float", min: 0, max: 100, default: 30, step: 0.1, unit: "%", group: "Delay", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamDelayFeedback" },
  { id: 4, name: "Room Size", type: "float", min: 0, max: 100, default: 50, step: 0.1, unit: "%", group: "Reverb", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamReverbSize" },
  { id: 5, name: "Damping", type: "float", min: 0, max: 100, default: 50, step: 0.1, unit: "%", group: "Reverb", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamReverbDamping" },
  { id: 6, name: "Width", type: "float", min: 0, max: 100, default: 100, step: 0.1, unit: "%", group: "Reverb", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamReverbWidth" },
  { id: 7, name: "Bypass", type: "bool", min: 0, max: 1, default: 0, step: 1, unit: "", group: "Master", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamBypass" },
];

// Control tags (for SCMFD - meters, etc.)
export const controlTags: ControlTag[] = [
  { id: 0, key: "kCtrlTagMeter", senderType: "IPeakAvgSender", channels: 2 },
];

// Message tags (for SAMFD - visualizations)
export const msgTags: MsgTag[] = [];

// Convenience lookups (must match C++ enums)
export const EParams = {
  kParamGain: 0,
  kParamMix: 1,
  kParamDelayTime: 2,
  kParamDelayFeedback: 3,
  kParamReverbSize: 4,
  kParamReverbDamping: 5,
  kParamReverbWidth: 6,
  kParamBypass: 7,
} as const;

export const EControlTags = {
  kCtrlTagMeter: 0,
} as const;

export const EMsgTags = {} as const;

// Type helpers
export type EParamsKey = keyof typeof EParams;
export type EControlTagsKey = keyof typeof EControlTags;
export type EMsgTagsKey = keyof typeof EMsgTags;
