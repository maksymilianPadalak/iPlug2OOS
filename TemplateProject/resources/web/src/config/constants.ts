/**
 * Parameter indices matching C++ TemplateProject.h EParams enum
 * AUTO-GENERATED - DO NOT EDIT MANUALLY
 * Generated from C++ source files using AI extraction
 */
export enum EParams {
  kParamGain = 0,
  kParamAttack = 1,
  kParamDecay = 2,
  kParamSustain = 3,
  kParamRelease = 4,
  // Oscillators
  kParamOsc1Mix = 5,
  kParamOsc2Mix = 6,
  kParamOsc3Mix = 7,
  kParamOsc4Mix = 8,
  kParamOsc1Detune = 9,
  kParamOsc2Detune = 10,
  kParamOsc3Detune = 11,
  kParamOsc4Detune = 12,
  kParamOsc1Octave = 13,
  kParamOsc2Octave = 14,
  kParamOsc3Octave = 15,
  kParamOsc4Octave = 16,
  kParamOsc1Wave = 17,
  kParamOsc2Wave = 18,
  kParamOsc3Wave = 19,
  kParamOsc4Wave = 20,
  // Reverb
  kParamReverbRoomSize = 21,
  kParamReverbDamp = 22,
  kParamReverbWidth = 23,
  kParamReverbDry = 24,
  kParamReverbWet = 25,
  kNumParams = 26
}

/**
 * Control tag indices matching C++ TemplateProject.h EControlTags enum
 */
export enum EControlTags {
  kCtrlTagMeter = 0,
  kCtrlTagLFOVis = 1,
  kCtrlTagScope = 2,
  kCtrlTagRTText = 3,
  kCtrlTagKeyboard = 4,
  kCtrlTagBender = 5,
  kNumCtrlTags = 6
}

/**
 * iPlug2 message types for UI-to-processor communication
 */
export const MessageTypes = {
  SPVFUI: "SPVFUI", // Send Parameter Value From UI
  BPCFUI: "BPCFUI", // Begin Parameter Change From UI
  EPCFUI: "EPCFUI", // End Parameter Change From UI
  SAMFUI: "SAMFUI", // Send Arbitrary Message From UI
  SMMFUI: "SMMFUI", // Send MIDI Message From UI
} as const;

export type MessageType = typeof MessageTypes[keyof typeof MessageTypes];

/**
 * iPlug2 message types for processor-to-UI communication
 */
export const CallbackTypes = {
  SPVFD: "SPVFD", // Send Parameter Value From Delegate
  SCVFD: "SCVFD", // Send Control Value From Delegate
  SCMFD: "SCMFD", // Send Control Message From Delegate
  SAMFD: "SAMFD", // Send Arbitrary Message From Delegate
  SMMFD: "SMMFD", // Send MIDI Message From Delegate
} as const;

/**
 * Parameter display names
 */
export const ParamNames: Record<EParams, string> = {
  [EParams.kParamGain]: "Gain",
  [EParams.kParamAttack]: "Attack",
  [EParams.kParamDecay]: "Decay",
  [EParams.kParamSustain]: "Sustain",
  [EParams.kParamRelease]: "Release",
  // Oscillators
  [EParams.kParamOsc1Mix]: "Osc1 Mix",
  [EParams.kParamOsc2Mix]: "Osc2 Mix",
  [EParams.kParamOsc3Mix]: "Osc3 Mix",
  [EParams.kParamOsc4Mix]: "Osc4 Mix",
  [EParams.kParamOsc1Detune]: "Osc1 Detune",
  [EParams.kParamOsc2Detune]: "Osc2 Detune",
  [EParams.kParamOsc3Detune]: "kParamOsc3Detune",
  [EParams.kParamOsc4Detune]: "Osc4 Detune",
  [EParams.kParamOsc1Octave]: "Osc1 Octave",
  [EParams.kParamOsc2Octave]: "Osc2 Octave",
  [EParams.kParamOsc3Octave]: "kParamOsc3Octave",
  [EParams.kParamOsc4Octave]: "Osc4 Octave",
  [EParams.kParamOsc1Wave]: "Osc1 Wave",
  [EParams.kParamOsc2Wave]: "Osc2 Wave",
  [EParams.kParamOsc3Wave]: "Osc3 Wave",
  [EParams.kParamOsc4Wave]: "Osc4 Wave",
  // Reverb
  [EParams.kParamReverbRoomSize]: "Reverb Room Size",
  [EParams.kParamReverbDamp]: "Reverb Damp",
  [EParams.kParamReverbWidth]: "Reverb Width",
  [EParams.kParamReverbDry]: "Reverb Dry",
  [EParams.kParamReverbWet]: "Reverb Wet",

  [EParams.kNumParams]: "",
};

/**
 * MIDI note names
 */
export const NoteNames = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'] as const;

/**
 * MIDI status bytes
 */
export const MidiStatus = {
  NoteOn: 0x90,
  NoteOff: 0x80,
} as const;
