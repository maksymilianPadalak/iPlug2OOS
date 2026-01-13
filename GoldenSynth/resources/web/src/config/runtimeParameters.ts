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
  { id: 8, name: "Filter On", type: "bool", min: 0, max: 1, default: 1, step: 1, unit: "", group: "Filter", shape: "ShapeLinear", shapeParameter: 0, enumValues: ["Off", "On"], automatable: true, key: "kParamFilterEnable" },
  { id: 9, name: "Filter Cutoff", type: "float", min: 20, max: 20000, default: 10000, step: 1, unit: "Hz", group: "Filter", shape: "ShapePowCurve", shapeParameter: 3, enumValues: null, automatable: true, key: "kParamFilterCutoff" },
  { id: 10, name: "Filter Reso", type: "float", min: 0, max: 100, default: 0, step: 0.1, unit: "%", group: "Filter", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamFilterResonance" },
  { id: 11, name: "Filter Type", type: "enum", min: 0, max: 3, default: 0, step: 1, unit: "", group: "Filter", shape: "ShapeLinear", shapeParameter: 0, enumValues: ["Lowpass", "Highpass", "Bandpass", "Notch"], automatable: true, key: "kParamFilterType" },
  // Pulse width modulation
  { id: 12, name: "Pulse Width", type: "float", min: 5, max: 95, default: 50, step: 0.1, unit: "%", group: "Oscillator", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamPulseWidth" },
  // FM synthesis parameters (DX7-style coarse + fine)
  { id: 13, name: "FM Ratio", type: "enum", min: 0, max: 8, default: 2, step: 1, unit: "", group: "Oscillator", shape: "ShapeLinear", shapeParameter: 0, enumValues: ["0.5:1", "1:1", "2:1", "3:1", "4:1", "5:1", "6:1", "7:1", "8:1"], automatable: true, key: "kParamFMRatio" },
  { id: 14, name: "FM Fine", type: "float", min: -50, max: 50, default: 0, step: 0.1, unit: "%", group: "Oscillator", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamFMFine" },
  { id: 15, name: "FM Depth", type: "float", min: 0, max: 100, default: 50, step: 0.1, unit: "%", group: "Oscillator", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamFMDepth" },
  { id: 16, name: "Osc1 Level", type: "float", min: 0, max: 100, default: 100, step: 0.1, unit: "%", group: "Oscillator", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamOsc1Level" },
  { id: 17, name: "Osc1 Octave", type: "enum", min: 0, max: 4, default: 2, step: 1, unit: "", group: "Oscillator", shape: "ShapeLinear", shapeParameter: 0, enumValues: ["-2", "-1", "0", "+1", "+2"], automatable: true, key: "kParamOsc1Octave" },
  { id: 18, name: "Osc1 Detune", type: "float", min: -100, max: 100, default: 0, step: 0.1, unit: "cents", group: "Oscillator", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamOsc1Detune" },
  { id: 19, name: "Osc1 Pan", type: "float", min: -100, max: 100, default: 0, step: 0.1, unit: "%", group: "Oscillator", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamOsc1Pan" },
  // Oscillator 2 parameters (fully independent like Serum)
  { id: 20, name: "Osc2 Wave", type: "enum", min: 0, max: 6, default: 1, step: 1, unit: "", group: "Oscillator 2", shape: "ShapeLinear", shapeParameter: 0, enumValues: ["Sine", "Saw", "Square", "Triangle", "Pulse", "FM", "Wavetable"], automatable: true, key: "kParamOsc2Waveform" },
  { id: 21, name: "Osc2 Octave", type: "enum", min: 0, max: 4, default: 2, step: 1, unit: "", group: "Oscillator 2", shape: "ShapeLinear", shapeParameter: 0, enumValues: ["-2", "-1", "0", "+1", "+2"], automatable: true, key: "kParamOsc2Octave" },
  { id: 22, name: "Osc2 Detune", type: "float", min: -100, max: 100, default: 7, step: 0.1, unit: "cents", group: "Oscillator 2", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamOsc2Detune" },
  { id: 23, name: "Osc2 Level", type: "float", min: 0, max: 100, default: 50, step: 0.1, unit: "%", group: "Oscillator 2", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamOsc2Level" },
  // Osc2 independent waveform-specific parameters
  { id: 24, name: "Osc2 Morph", type: "float", min: 0, max: 100, default: 0, step: 0.1, unit: "%", group: "Oscillator 2", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamOsc2Morph" },
  { id: 25, name: "Osc2 PW", type: "float", min: 5, max: 95, default: 50, step: 0.1, unit: "%", group: "Oscillator 2", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamOsc2PulseWidth" },
  { id: 26, name: "Osc2 FM Ratio", type: "enum", min: 0, max: 8, default: 2, step: 1, unit: "", group: "Oscillator 2", shape: "ShapeLinear", shapeParameter: 0, enumValues: ["0.5:1", "1:1", "2:1", "3:1", "4:1", "5:1", "6:1", "7:1", "8:1"], automatable: true, key: "kParamOsc2FMRatio" },
  { id: 27, name: "Osc2 FM Fine", type: "float", min: -50, max: 50, default: 0, step: 0.1, unit: "%", group: "Oscillator 2", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamOsc2FMFine" },
  { id: 28, name: "Osc2 FM Depth", type: "float", min: 0, max: 100, default: 50, step: 0.1, unit: "%", group: "Oscillator 2", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamOsc2FMDepth" },
  { id: 29, name: "Osc2 Pan", type: "float", min: -100, max: 100, default: 0, step: 0.1, unit: "%", group: "Oscillator 2", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamOsc2Pan" },
  // Osc1 Unison parameters - per-oscillator unison (Serum-style)
  { id: 30, name: "Osc1 Uni", type: "enum", min: 0, max: 7, default: 0, step: 1, unit: "", group: "Oscillator", shape: "ShapeLinear", shapeParameter: 0, enumValues: ["1", "2", "3", "4", "5", "6", "7", "8"], automatable: true, key: "kParamOsc1UnisonVoices" },
  { id: 31, name: "Osc1 Uni Det", type: "float", min: 0, max: 100, default: 25, step: 0.1, unit: "%", group: "Oscillator", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamOsc1UnisonDetune" },
  { id: 32, name: "Osc1 Uni Wid", type: "float", min: 0, max: 100, default: 80, step: 0.1, unit: "%", group: "Oscillator", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamOsc1UnisonWidth" },
  { id: 33, name: "Osc1 Uni Bld", type: "float", min: 0, max: 100, default: 75, step: 0.1, unit: "%", group: "Oscillator", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamOsc1UnisonBlend" },
  // Osc2 Unison parameters - independent from Osc1
  { id: 34, name: "Osc2 Uni", type: "enum", min: 0, max: 7, default: 0, step: 1, unit: "", group: "Oscillator 2", shape: "ShapeLinear", shapeParameter: 0, enumValues: ["1", "2", "3", "4", "5", "6", "7", "8"], automatable: true, key: "kParamOsc2UnisonVoices" },
  { id: 35, name: "Osc2 Uni Det", type: "float", min: 0, max: 100, default: 25, step: 0.1, unit: "%", group: "Oscillator 2", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamOsc2UnisonDetune" },
  { id: 36, name: "Osc2 Uni Wid", type: "float", min: 0, max: 100, default: 80, step: 0.1, unit: "%", group: "Oscillator 2", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamOsc2UnisonWidth" },
  { id: 37, name: "Osc2 Uni Bld", type: "float", min: 0, max: 100, default: 75, step: 0.1, unit: "%", group: "Oscillator 2", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamOsc2UnisonBlend" },
  // Oscillator sync - classic analog sync sound
  { id: 38, name: "Osc Sync", type: "enum", min: 0, max: 1, default: 0, step: 1, unit: "", group: "Oscillator", shape: "ShapeLinear", shapeParameter: 0, enumValues: ["Off", "Hard"], automatable: true, key: "kParamOscSync" },
  // LFO1 - Low Frequency Oscillator 1 for modulation
  // Low/High system: LFO sweeps from Low to High for full control over direction and range
  // Tempo Sync: When not "Off", LFO syncs to host tempo using musical divisions
  // Rate at 0 Hz = static offset (LFO frozen, Low/High control DC offset)
  { id: 39, name: "LFO1 On", type: "bool", min: 0, max: 1, default: 1, step: 1, unit: "", group: "LFO1", shape: "ShapeLinear", shapeParameter: 0, enumValues: ["Off", "On"], automatable: true, key: "kParamLFO1Enable" },
  { id: 40, name: "LFO1 Rate", type: "float", min: 0.01, max: 20, default: 1, step: 0.01, unit: "Hz", group: "LFO1", shape: "ShapePowCurve", shapeParameter: 3, enumValues: null, automatable: true, key: "kParamLFO1Rate" },
  { id: 41, name: "LFO1 Sync", type: "enum", min: 0, max: 16, default: 0, step: 1, unit: "", group: "LFO1", shape: "ShapeLinear", shapeParameter: 0, enumValues: ["Off", "4/1", "2/1", "1/1", "1/2", "1/2D", "1/2T", "1/4", "1/4D", "1/4T", "1/8", "1/8D", "1/8T", "1/16", "1/16D", "1/16T", "1/32"], automatable: true, key: "kParamLFO1Sync" },
  { id: 42, name: "LFO1 Low", type: "float", min: -100, max: 100, default: -100, step: 1, unit: "%", group: "LFO1", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamLFO1Low" },
  { id: 43, name: "LFO1 High", type: "float", min: -100, max: 100, default: 100, step: 1, unit: "%", group: "LFO1", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamLFO1High" },
  { id: 44, name: "LFO1 Wave", type: "enum", min: 0, max: 5, default: 0, step: 1, unit: "", group: "LFO1", shape: "ShapeLinear", shapeParameter: 0, enumValues: ["Sine", "Triangle", "Saw Up", "Saw Down", "Square", "S&H"], automatable: true, key: "kParamLFO1Waveform" },
  { id: 45, name: "LFO1 Retrig", type: "enum", min: 0, max: 1, default: 0, step: 1, unit: "", group: "LFO1", shape: "ShapeLinear", shapeParameter: 0, enumValues: ["Free", "Retrig"], automatable: true, key: "kParamLFO1Retrigger" },
  { id: 46, name: "LFO1 Dest", type: "enum", min: 0, max: 14, default: 1, step: 1, unit: "", group: "LFO1", shape: "ShapeLinear", shapeParameter: 0, enumValues: ["Off", "Filter", "Pitch", "PW", "Amp", "FM", "WT Pos", "Osc1 Pitch", "Osc2 Pitch", "Osc1 PW", "Osc2 PW", "Osc1 FM", "Osc2 FM", "Osc1 WT", "Osc2 WT"], automatable: true, key: "kParamLFO1Destination" },
  // LFO2 - Low Frequency Oscillator 2 for modulation
  // Rate at 0 Hz = static offset (LFO frozen, Low/High control DC offset)
  { id: 47, name: "LFO2 On", type: "bool", min: 0, max: 1, default: 0, step: 1, unit: "", group: "LFO2", shape: "ShapeLinear", shapeParameter: 0, enumValues: ["Off", "On"], automatable: true, key: "kParamLFO2Enable" },
  { id: 48, name: "LFO2 Rate", type: "float", min: 0.01, max: 20, default: 0.5, step: 0.01, unit: "Hz", group: "LFO2", shape: "ShapePowCurve", shapeParameter: 3, enumValues: null, automatable: true, key: "kParamLFO2Rate" },
  { id: 49, name: "LFO2 Sync", type: "enum", min: 0, max: 16, default: 0, step: 1, unit: "", group: "LFO2", shape: "ShapeLinear", shapeParameter: 0, enumValues: ["Off", "4/1", "2/1", "1/1", "1/2", "1/2D", "1/2T", "1/4", "1/4D", "1/4T", "1/8", "1/8D", "1/8T", "1/16", "1/16D", "1/16T", "1/32"], automatable: true, key: "kParamLFO2Sync" },
  { id: 50, name: "LFO2 Low", type: "float", min: -100, max: 100, default: 0, step: 1, unit: "%", group: "LFO2", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamLFO2Low" },
  { id: 51, name: "LFO2 High", type: "float", min: -100, max: 100, default: 0, step: 1, unit: "%", group: "LFO2", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamLFO2High" },
  { id: 52, name: "LFO2 Wave", type: "enum", min: 0, max: 5, default: 0, step: 1, unit: "", group: "LFO2", shape: "ShapeLinear", shapeParameter: 0, enumValues: ["Sine", "Triangle", "Saw Up", "Saw Down", "Square", "S&H"], automatable: true, key: "kParamLFO2Waveform" },
  { id: 53, name: "LFO2 Retrig", type: "enum", min: 0, max: 1, default: 0, step: 1, unit: "", group: "LFO2", shape: "ShapeLinear", shapeParameter: 0, enumValues: ["Free", "Retrig"], automatable: true, key: "kParamLFO2Retrigger" },
  { id: 54, name: "LFO2 Dest", type: "enum", min: 0, max: 14, default: 0, step: 1, unit: "", group: "LFO2", shape: "ShapeLinear", shapeParameter: 0, enumValues: ["Off", "Filter", "Pitch", "PW", "Amp", "FM", "WT Pos", "Osc1 Pitch", "Osc2 Pitch", "Osc1 PW", "Osc2 PW", "Osc1 FM", "Osc2 FM", "Osc1 WT", "Osc2 WT"], automatable: true, key: "kParamLFO2Destination" },
  // Stereo Delay - simple delay with separate dry/wet levels
  { id: 55, name: "Delay On", type: "bool", min: 0, max: 1, default: 0, step: 1, unit: "", group: "Delay", shape: "ShapeLinear", shapeParameter: 0, enumValues: ["Off", "On"], automatable: true, key: "kParamDelayEnable" },
  { id: 56, name: "Delay Time", type: "float", min: 1, max: 2000, default: 250, step: 1, unit: "ms", group: "Delay", shape: "ShapePowCurve", shapeParameter: 2, enumValues: null, automatable: true, key: "kParamDelayTime" },
  { id: 57, name: "Delay Sync", type: "enum", min: 0, max: 14, default: 0, step: 1, unit: "", group: "Delay", shape: "ShapeLinear", shapeParameter: 0, enumValues: ["Off", "1/1", "1/2D", "1/2", "1/2T", "1/4D", "1/4", "1/4T", "1/8D", "1/8", "1/8T", "1/16D", "1/16", "1/16T", "1/32"], automatable: true, key: "kParamDelaySync" },
  { id: 58, name: "Delay Fdbk", type: "float", min: 0, max: 90, default: 30, step: 0.1, unit: "%", group: "Delay", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamDelayFeedback" },
  { id: 59, name: "Delay Dry", type: "float", min: 0, max: 100, default: 100, step: 0.1, unit: "%", group: "Delay", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamDelayDry" },
  { id: 60, name: "Delay Wet", type: "float", min: 0, max: 100, default: 0, step: 0.1, unit: "%", group: "Delay", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: true, key: "kParamDelayWet" },
  { id: 61, name: "Delay Mode", type: "enum", min: 0, max: 1, default: 0, step: 1, unit: "", group: "Delay", shape: "ShapeLinear", shapeParameter: 0, enumValues: ["Stereo", "Ping-Pong"], automatable: true, key: "kParamDelayMode" },
  // Voice count display (read-only)
  { id: 62, name: "Voices", type: "int", min: 0, max: 32, default: 0, step: 1, unit: "", group: "Master", shape: "ShapeLinear", shapeParameter: 0, enumValues: null, automatable: false, key: "kParamVoiceCount" },
  // Preset selection (uses regular parameter for reliable UI communication)
  { id: 63, name: "Preset", type: "enum", min: 0, max: 4, default: 0, step: 1, unit: "", group: "Master", shape: "ShapeLinear", shapeParameter: 0, enumValues: ["Init", "Classic Lead", "Massive Supersaw", "FM Bell Pad", "Wobble Bass"], automatable: true, key: "kParamPresetSelect" },
];

// Control tags (for SCMFD - meters, etc.)
export const controlTags: ControlTag[] = [
  { id: 0, key: "kCtrlTagMeter", senderType: "IPeakAvgSender", channels: 2 },
  { id: 1, key: "kCtrlTagWaveform", senderType: "IBufferSender", channels: 1 },
];

// Message tags (for SAMFD - visualizations and presets)
export const msgTags: MsgTag[] = [
  { id: 0, key: "kMsgTagRestorePreset" },
];

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
  // Filter parameters
  kParamFilterEnable: 8,
  kParamFilterCutoff: 9,
  kParamFilterResonance: 10,
  kParamFilterType: 11,
  kParamPulseWidth: 12,
  kParamFMRatio: 13,
  kParamFMFine: 14,
  kParamFMDepth: 15,
  kParamOsc1Level: 16,
  kParamOsc1Octave: 17,
  kParamOsc1Detune: 18,
  kParamOsc1Pan: 19,
  kParamOsc2Waveform: 20,
  kParamOsc2Octave: 21,
  kParamOsc2Detune: 22,
  kParamOsc2Level: 23,
  kParamOsc2Morph: 24,
  kParamOsc2PulseWidth: 25,
  kParamOsc2FMRatio: 26,
  kParamOsc2FMFine: 27,
  kParamOsc2FMDepth: 28,
  kParamOsc2Pan: 29,
  // Osc1 Unison parameters
  kParamOsc1UnisonVoices: 30,
  kParamOsc1UnisonDetune: 31,
  kParamOsc1UnisonWidth: 32,
  kParamOsc1UnisonBlend: 33,
  // Osc2 Unison parameters
  kParamOsc2UnisonVoices: 34,
  kParamOsc2UnisonDetune: 35,
  kParamOsc2UnisonWidth: 36,
  kParamOsc2UnisonBlend: 37,
  // Oscillator sync
  kParamOscSync: 38,
  // LFO1 parameters (Low/High system + Tempo Sync + Enable)
  kParamLFO1Enable: 39,
  kParamLFO1Rate: 40,
  kParamLFO1Sync: 41,
  kParamLFO1Low: 42,
  kParamLFO1High: 43,
  kParamLFO1Waveform: 44,
  kParamLFO1Retrigger: 45,
  kParamLFO1Destination: 46,
  // LFO2 parameters (Low/High system + Tempo Sync + Enable)
  kParamLFO2Enable: 47,
  kParamLFO2Rate: 48,
  kParamLFO2Sync: 49,
  kParamLFO2Low: 50,
  kParamLFO2High: 51,
  kParamLFO2Waveform: 52,
  kParamLFO2Retrigger: 53,
  kParamLFO2Destination: 54,
  // Stereo Delay parameters
  kParamDelayEnable: 55,
  kParamDelayTime: 56,
  kParamDelaySync: 57,
  kParamDelayFeedback: 58,
  kParamDelayDry: 59,
  kParamDelayWet: 60,
  kParamDelayMode: 61,
  kParamVoiceCount: 62,
  kParamPresetSelect: 63,
} as const;

export const EControlTags = {
  kCtrlTagMeter: 0,
  kCtrlTagWaveform: 1,
} as const;

export const EMsgTags = {
  kMsgTagRestorePreset: 0,
} as const;

// Type helpers
export type EParamsKey = keyof typeof EParams;
export type EControlTagsKey = keyof typeof EControlTags;
export type EMsgTagsKey = keyof typeof EMsgTags;
