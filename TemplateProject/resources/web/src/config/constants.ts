/**
 * Parameter indices matching C++ TemplateProject.h EParams enum
 * AUTO-GENERATED - DO NOT EDIT MANUALLY
 * Generated from C++ source files using AI extraction
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
  // Oscillators
  kParamOsc1Mix = 11,
  kParamOsc2Mix = 12,
  kParamOsc3Mix = 13,
  kParamOsc4Mix = 14,
  kParamOsc1Detune = 15,
  kParamOsc2Detune = 16,
  kParamOsc3Detune = 17,
  kParamOsc4Detune = 18,
  kParamOsc1Octave = 19,
  kParamOsc2Octave = 20,
  kParamOsc3Octave = 21,
  kParamOsc4Octave = 22,
  kParamOsc1Wave = 23,
  kParamOsc2Wave = 24,
  kParamOsc3Wave = 25,
  kParamOsc4Wave = 26,
  // Filter
  kParamFilterCutoff = 27,
  kParamFilterResonance = 28,
  kParamFilterEnvAmount = 29,
  kParamFilterKeytrack = 30,
  kParamFilterAttack = 31,
  kParamFilterDecay = 32,
  kParamFilterSustain = 33,
  kParamFilterRelease = 34,
  // LFO2
  kParamLFO2RateHz = 35,
  kParamLFO2RateTempo = 36,
  kParamLFO2RateMode = 37,
  kParamLFO2Shape = 38,
  kParamLFO2Depth = 39,
  // Delay
  kParamDelayTime = 40,
  kParamDelayFeedback = 41,
  kParamDelayDry = 42,
  kParamDelayWet = 43,
  // Sync
  kParamOscSync = 44,
  kParamOscSyncRatio = 45,
  // Reverb
  kParamReverbRoomSize = 46,
  kParamReverbDamp = 47,
  kParamReverbWidth = 48,
  kParamReverbDry = 49,
  kParamReverbWet = 50,
  kNumParams = 51
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
  [EParams.kParamNoteGlideTime]: "kParamNoteGlideTime",
  [EParams.kParamAttack]: "Attack",
  [EParams.kParamDecay]: "Decay",
  [EParams.kParamSustain]: "Sustain",
  [EParams.kParamRelease]: "Release",
  [EParams.kParamLFOShape]: "kParamLFOShape",
  [EParams.kParamLFORateHz]: "kParamLFORateHz",
  [EParams.kParamLFORateTempo]: "kParamLFORateTempo",
  [EParams.kParamLFORateMode]: "kParamLFORateMode",
  [EParams.kParamLFODepth]: "kParamLFODepth",
  // Oscillators
  [EParams.kParamOsc1Mix]: "Osc1 Mix",
  [EParams.kParamOsc2Mix]: "Osc2 Mix",
  [EParams.kParamOsc3Mix]: "Osc3 Mix",
  [EParams.kParamOsc4Mix]: "Osc4 Mix",
  [EParams.kParamOsc1Detune]: "Osc1 Detune",
  [EParams.kParamOsc2Detune]: "Osc2 Detune",
  [EParams.kParamOsc3Detune]: "Osc3 Detune",
  [EParams.kParamOsc4Detune]: "Osc4 Detune",
  [EParams.kParamOsc1Octave]: "Osc1 Octave",
  [EParams.kParamOsc2Octave]: "Osc2 Octave",
  [EParams.kParamOsc3Octave]: "Osc3 Octave",
  [EParams.kParamOsc4Octave]: "Osc4 Octave",
  [EParams.kParamOsc1Wave]: "Osc1 Wave",
  [EParams.kParamOsc2Wave]: "Osc2 Wave",
  [EParams.kParamOsc3Wave]: "Osc3 Wave",
  [EParams.kParamOsc4Wave]: "Osc4 Wave",
  // Filter
  [EParams.kParamFilterCutoff]: "kParamFilterCutoff",
  [EParams.kParamFilterResonance]: "kParamFilterResonance",
  [EParams.kParamFilterEnvAmount]: "kParamFilterEnvAmount",
  [EParams.kParamFilterKeytrack]: "kParamFilterKeytrack",
  [EParams.kParamFilterAttack]: "kParamFilterAttack",
  [EParams.kParamFilterDecay]: "kParamFilterDecay",
  [EParams.kParamFilterSustain]: "kParamFilterSustain",
  [EParams.kParamFilterRelease]: "kParamFilterRelease",
  // LFO2
  [EParams.kParamLFO2RateHz]: "kParamLFO2RateHz",
  [EParams.kParamLFO2RateTempo]: "kParamLFO2RateTempo",
  [EParams.kParamLFO2RateMode]: "kParamLFO2RateMode",
  [EParams.kParamLFO2Shape]: "kParamLFO2Shape",
  [EParams.kParamLFO2Depth]: "kParamLFO2Depth",
  // Delay
  [EParams.kParamDelayTime]: "kParamDelayTime",
  [EParams.kParamDelayFeedback]: "kParamDelayFeedback",
  [EParams.kParamDelayDry]: "kParamDelayDry",
  [EParams.kParamDelayWet]: "kParamDelayWet",
  // Sync
  [EParams.kParamOscSync]: "kParamOscSync",
  [EParams.kParamOscSyncRatio]: "kParamOscSyncRatio",
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
