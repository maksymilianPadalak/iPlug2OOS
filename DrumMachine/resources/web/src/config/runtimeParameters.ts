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

// Parameters - matches Plugin.h EParams enum
export const runtimeParameters: RuntimeParameter[] = [
  // Master
  { id: 0, name: "Gain", type: "float", min: 0, max: 100, default: 80, step: 0.01, unit: "%", group: "Master", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamGain" },

  // Kick (MIDI note 36 / C1)
  { id: 1, name: "Kick Pitch Start", type: "float", min: 100, max: 500, default: 300, step: 1, unit: "Hz", group: "Kick", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamKickPitchStart" },
  { id: 2, name: "Kick Pitch End", type: "float", min: 30, max: 150, default: 50, step: 1, unit: "Hz", group: "Kick", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamKickPitchEnd" },
  { id: 3, name: "Kick Pitch Decay", type: "float", min: 10, max: 200, default: 50, step: 1, unit: "ms", group: "Kick", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamKickPitchDecay" },
  { id: 4, name: "Kick Amp Decay", type: "float", min: 50, max: 1000, default: 300, step: 1, unit: "ms", group: "Kick", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamKickAmpDecay" },

  // Snare (MIDI note 38 / D1)
  { id: 5, name: "Snare Filter", type: "float", min: 500, max: 8000, default: 2000, step: 10, unit: "Hz", group: "Snare", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamSnareFilterFreq" },
  { id: 6, name: "Snare Q", type: "float", min: 0.3, max: 5.0, default: 1.0, step: 0.1, unit: "", group: "Snare", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamSnareFilterQ" },
  { id: 7, name: "Snare Decay", type: "float", min: 50, max: 500, default: 150, step: 1, unit: "ms", group: "Snare", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamSnareNoiseDecay" },
  { id: 8, name: "Snare Body", type: "float", min: 0, max: 100, default: 30, step: 1, unit: "%", group: "Snare", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamSnareBodyMix" },

  // HiHat (MIDI note 42 closed, 46 open)
  { id: 9, name: "HiHat Tone", type: "float", min: 4000, max: 16000, default: 8000, step: 100, unit: "Hz", group: "HiHat", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamHiHatFilterFreq" },
  { id: 10, name: "HiHat Closed", type: "float", min: 5, max: 100, default: 30, step: 1, unit: "ms", group: "HiHat", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamHiHatClosedDecay" },
  { id: 11, name: "HiHat Open", type: "float", min: 100, max: 1000, default: 400, step: 10, unit: "ms", group: "HiHat", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamHiHatOpenDecay" },

  // Tom (MIDI note 45 / A1)
  { id: 12, name: "Tom Pitch Start", type: "float", min: 80, max: 400, default: 200, step: 1, unit: "Hz", group: "Tom", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamTomPitchStart" },
  { id: 13, name: "Tom Pitch End", type: "float", min: 40, max: 200, default: 80, step: 1, unit: "Hz", group: "Tom", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamTomPitchEnd" },
  { id: 14, name: "Tom Pitch Decay", type: "float", min: 20, max: 150, default: 60, step: 1, unit: "ms", group: "Tom", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamTomPitchDecay" },
  { id: 15, name: "Tom Amp Decay", type: "float", min: 100, max: 800, default: 400, step: 1, unit: "ms", group: "Tom", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamTomAmpDecay" },

  // Clap (MIDI note 39 / D#1)
  { id: 16, name: "Clap Filter", type: "float", min: 500, max: 4000, default: 1500, step: 10, unit: "Hz", group: "Clap", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamClapFilterFreq" },
  { id: 17, name: "Clap Decay", type: "float", min: 50, max: 500, default: 200, step: 1, unit: "ms", group: "Clap", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamClapDecay" },
  { id: 18, name: "Clap Spread", type: "float", min: 5, max: 50, default: 20, step: 1, unit: "ms", group: "Clap", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamClapSpread" },

  // Rim (MIDI note 37 / C#1)
  { id: 19, name: "Rim Pitch", type: "float", min: 400, max: 2000, default: 800, step: 10, unit: "Hz", group: "Rim", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamRimPitch" },
  { id: 20, name: "Rim Decay", type: "float", min: 5, max: 80, default: 20, step: 1, unit: "ms", group: "Rim", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamRimDecay" },
  { id: 21, name: "Rim Click", type: "float", min: 0, max: 100, default: 50, step: 1, unit: "%", group: "Rim", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamRimClick" },
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
  // Kick
  kParamKickPitchStart: 1,
  kParamKickPitchEnd: 2,
  kParamKickPitchDecay: 3,
  kParamKickAmpDecay: 4,
  // Snare
  kParamSnareFilterFreq: 5,
  kParamSnareFilterQ: 6,
  kParamSnareNoiseDecay: 7,
  kParamSnareBodyMix: 8,
  // HiHat
  kParamHiHatFilterFreq: 9,
  kParamHiHatClosedDecay: 10,
  kParamHiHatOpenDecay: 11,
  // Tom
  kParamTomPitchStart: 12,
  kParamTomPitchEnd: 13,
  kParamTomPitchDecay: 14,
  kParamTomAmpDecay: 15,
  // Clap
  kParamClapFilterFreq: 16,
  kParamClapDecay: 17,
  kParamClapSpread: 18,
  // Rim
  kParamRimPitch: 19,
  kParamRimDecay: 20,
  kParamRimClick: 21,
} as const;

export const EControlTags = {
  kCtrlTagMeter: 0,
} as const;

export const EMsgTags = {} as const;

// Type helpers
export type EParamsKey = keyof typeof EParams;
export type EControlTagsKey = keyof typeof EControlTags;
export type EMsgTagsKey = keyof typeof EMsgTags;
