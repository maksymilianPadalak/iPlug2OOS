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
  { id: 7, name: "Env Velocity", type: "float", min: 0, max: 100, default: 50, step: 1, unit: "%", group: "Envelope", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamEnvVelocity" },
  // Filter parameters
  { id: 8, name: "Filter Cutoff", type: "float", min: 20, max: 20000, default: 10000, step: 1, unit: "Hz", group: "Filter", shape: "ShapePowCurve", shapeParameter: 3, enumValues: null, automatable: true, key: "kParamFilterCutoff" },
  { id: 9, name: "Filter Reso", type: "float", min: 0, max: 100, default: 0, step: 0.1, unit: "%", group: "Filter", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamFilterResonance" },
  { id: 10, name: "Filter Type", type: "enum", min: 0, max: 3, default: 0, step: 1, unit: "", group: "Filter", shape: "ShapeLinear", shapeParameter: 0, enumValues: ["Lowpass", "Highpass", "Bandpass", "Notch"], automatable: true, key: "kParamFilterType" },
  // Pulse width modulation
  { id: 11, name: "Pulse Width", type: "float", min: 5, max: 95, default: 50, step: 0.1, unit: "%", group: "Oscillator", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamPulseWidth" },
  // FM synthesis parameters (DX7-style coarse + fine)
  { id: 12, name: "FM Ratio", type: "enum", min: 0, max: 8, default: 2, step: 1, unit: "", group: "Oscillator", shape: "ShapeLinear", shapeParameter: 0, enumValues: ["0.5:1", "1:1", "2:1", "3:1", "4:1", "5:1", "6:1", "7:1", "8:1"], automatable: true, key: "kParamFMRatio" },
  { id: 13, name: "FM Fine", type: "float", min: -50, max: 50, default: 0, step: 0.1, unit: "%", group: "Oscillator", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamFMFine" },
  { id: 14, name: "FM Depth", type: "float", min: 0, max: 100, default: 50, step: 0.1, unit: "%", group: "Oscillator", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamFMDepth" },
  { id: 15, name: "Osc1 Level", type: "float", min: 0, max: 100, default: 100, step: 0.1, unit: "%", group: "Oscillator", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamOsc1Level" },
  { id: 16, name: "Osc1 Octave", type: "enum", min: 0, max: 4, default: 2, step: 1, unit: "", group: "Oscillator", shape: "ShapeLinear", shapeParameter: 0, enumValues: ["-2", "-1", "0", "+1", "+2"], automatable: true, key: "kParamOsc1Octave" },
  { id: 17, name: "Osc1 Detune", type: "float", min: -100, max: 100, default: 0, step: 0.1, unit: "cents", group: "Oscillator", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamOsc1Detune" },
  // Oscillator 2 parameters (fully independent like Serum)
  { id: 18, name: "Osc2 Wave", type: "enum", min: 0, max: 6, default: 1, step: 1, unit: "", group: "Oscillator 2", shape: "ShapeLinear", shapeParameter: 0, enumValues: ["Sine", "Saw", "Square", "Triangle", "Pulse", "FM", "Wavetable"], automatable: true, key: "kParamOsc2Waveform" },
  { id: 19, name: "Osc2 Octave", type: "enum", min: 0, max: 4, default: 2, step: 1, unit: "", group: "Oscillator 2", shape: "ShapeLinear", shapeParameter: 0, enumValues: ["-2", "-1", "0", "+1", "+2"], automatable: true, key: "kParamOsc2Octave" },
  { id: 20, name: "Osc2 Detune", type: "float", min: -100, max: 100, default: 7, step: 0.1, unit: "cents", group: "Oscillator 2", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamOsc2Detune" },
  { id: 21, name: "Osc2 Level", type: "float", min: 0, max: 100, default: 50, step: 0.1, unit: "%", group: "Oscillator 2", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamOsc2Level" },
  // Osc2 independent waveform-specific parameters
  { id: 22, name: "Osc2 Morph", type: "float", min: 0, max: 100, default: 0, step: 0.1, unit: "%", group: "Oscillator 2", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamOsc2Morph" },
  { id: 23, name: "Osc2 PW", type: "float", min: 5, max: 95, default: 50, step: 0.1, unit: "%", group: "Oscillator 2", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamOsc2PulseWidth" },
  { id: 24, name: "Osc2 FM Ratio", type: "enum", min: 0, max: 8, default: 2, step: 1, unit: "", group: "Oscillator 2", shape: "ShapeLinear", shapeParameter: 0, enumValues: ["0.5:1", "1:1", "2:1", "3:1", "4:1", "5:1", "6:1", "7:1", "8:1"], automatable: true, key: "kParamOsc2FMRatio" },
  { id: 25, name: "Osc2 FM Fine", type: "float", min: -50, max: 50, default: 0, step: 0.1, unit: "%", group: "Oscillator 2", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamOsc2FMFine" },
  { id: 26, name: "Osc2 FM Depth", type: "float", min: 0, max: 100, default: 50, step: 0.1, unit: "%", group: "Oscillator 2", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamOsc2FMDepth" },
  // Osc1 Unison parameters - per-oscillator unison (Serum-style)
  { id: 27, name: "Osc1 Uni", type: "enum", min: 0, max: 7, default: 0, step: 1, unit: "", group: "Oscillator", shape: "ShapeLinear", shapeParameter: 0, enumValues: ["1", "2", "3", "4", "5", "6", "7", "8"], automatable: true, key: "kParamOsc1UnisonVoices" },
  { id: 28, name: "Osc1 Uni Det", type: "float", min: 0, max: 100, default: 25, step: 0.1, unit: "%", group: "Oscillator", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamOsc1UnisonDetune" },
  { id: 29, name: "Osc1 Uni Wid", type: "float", min: 0, max: 100, default: 80, step: 0.1, unit: "%", group: "Oscillator", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamOsc1UnisonWidth" },
  { id: 30, name: "Osc1 Uni Bld", type: "float", min: 0, max: 100, default: 75, step: 0.1, unit: "%", group: "Oscillator", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamOsc1UnisonBlend" },
  // Osc2 Unison parameters - independent from Osc1
  { id: 31, name: "Osc2 Uni", type: "enum", min: 0, max: 7, default: 0, step: 1, unit: "", group: "Oscillator 2", shape: "ShapeLinear", shapeParameter: 0, enumValues: ["1", "2", "3", "4", "5", "6", "7", "8"], automatable: true, key: "kParamOsc2UnisonVoices" },
  { id: 32, name: "Osc2 Uni Det", type: "float", min: 0, max: 100, default: 25, step: 0.1, unit: "%", group: "Oscillator 2", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamOsc2UnisonDetune" },
  { id: 33, name: "Osc2 Uni Wid", type: "float", min: 0, max: 100, default: 80, step: 0.1, unit: "%", group: "Oscillator 2", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamOsc2UnisonWidth" },
  { id: 34, name: "Osc2 Uni Bld", type: "float", min: 0, max: 100, default: 75, step: 0.1, unit: "%", group: "Oscillator 2", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamOsc2UnisonBlend" },
  // Oscillator sync - classic analog sync sound
  { id: 35, name: "Osc Sync", type: "enum", min: 0, max: 1, default: 0, step: 1, unit: "", group: "Oscillator", shape: "ShapeLinear", shapeParameter: 0, enumValues: ["Off", "Hard"], automatable: true, key: "kParamOscSync" },
  // LFO - Low Frequency Oscillator for filter modulation
  { id: 36, name: "LFO Rate", type: "float", min: 0.01, max: 20, default: 1, step: 0.01, unit: "Hz", group: "LFO", shape: "ShapePowCurve", shapeParameter: 3, enumValues: null, automatable: true, key: "kParamLFORate" },
  { id: 37, name: "LFO Depth", type: "float", min: 0, max: 100, default: 0, step: 0.1, unit: "%", group: "LFO", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamLFODepth" },
  { id: 38, name: "LFO Wave", type: "enum", min: 0, max: 5, default: 0, step: 1, unit: "", group: "LFO", shape: "ShapeLinear", shapeParameter: 0, enumValues: ["Sine", "Triangle", "Saw Up", "Saw Down", "Square", "S&H"], automatable: true, key: "kParamLFOWaveform" },
  { id: 39, name: "LFO Retrig", type: "enum", min: 0, max: 1, default: 0, step: 1, unit: "", group: "LFO", shape: "ShapeLinear", shapeParameter: 0, enumValues: ["Free", "Retrig"], automatable: true, key: "kParamLFORetrigger" },
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
  kParamEnvVelocity: 7,
  kParamFilterCutoff: 8,
  kParamFilterResonance: 9,
  kParamFilterType: 10,
  kParamPulseWidth: 11,
  kParamFMRatio: 12,
  kParamFMFine: 13,
  kParamFMDepth: 14,
  kParamOsc1Level: 15,
  kParamOsc1Octave: 16,
  kParamOsc1Detune: 17,
  kParamOsc2Waveform: 18,
  kParamOsc2Octave: 19,
  kParamOsc2Detune: 20,
  kParamOsc2Level: 21,
  kParamOsc2Morph: 22,
  kParamOsc2PulseWidth: 23,
  kParamOsc2FMRatio: 24,
  kParamOsc2FMFine: 25,
  kParamOsc2FMDepth: 26,
  // Osc1 Unison parameters
  kParamOsc1UnisonVoices: 27,
  kParamOsc1UnisonDetune: 28,
  kParamOsc1UnisonWidth: 29,
  kParamOsc1UnisonBlend: 30,
  // Osc2 Unison parameters
  kParamOsc2UnisonVoices: 31,
  kParamOsc2UnisonDetune: 32,
  kParamOsc2UnisonWidth: 33,
  kParamOsc2UnisonBlend: 34,
  // Oscillator sync
  kParamOscSync: 35,
  // LFO parameters
  kParamLFORate: 36,
  kParamLFODepth: 37,
  kParamLFOWaveform: 38,
  kParamLFORetrigger: 39,
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
