/**
 * Parameter indices matching C++ TemplateProject.h EParams enum
 */
export enum EParams {
  kParamGain = 0,
  kParamNoteGlideTime = 1,
  kParamAttack = 2,
  kParamDecay = 3,
  kParamSustain = 4,
  kParamRelease = 5,
  kParamLFOShape = 6,
  kParamLFORateHz = 7,
  kParamLFORateTempo = 8,
  kParamLFORateMode = 9,
  kParamLFODepth = 10,
  kNumParams = 11
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
  [EParams.kParamNoteGlideTime]: "Note Glide Time",
  [EParams.kParamAttack]: "Attack",
  [EParams.kParamDecay]: "Decay",
  [EParams.kParamSustain]: "Sustain",
  [EParams.kParamRelease]: "Release",
  [EParams.kParamLFOShape]: "LFO Shape",
  [EParams.kParamLFORateHz]: "LFO Rate Hz",
  [EParams.kParamLFORateTempo]: "LFO Rate Tempo",
  [EParams.kParamLFORateMode]: "LFO Sync",
  [EParams.kParamLFODepth]: "LFO Depth",
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

