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
  kParamOsc1Detune = 14,
  kParamOsc2Detune = 15,
  kParamOsc3Detune = 16,
  kParamOsc1Octave = 17,
  kParamOsc2Octave = 18,
  kParamOsc3Octave = 19,
  kParamOsc1Wave = 20,
  kParamOsc2Wave = 21,
  kParamOsc3Wave = 22,
  // Filter
  kParamFilterCutoff = 23,
  kParamFilterResonance = 24,
  kParamFilterEnvAmount = 25,
  kParamFilterKeytrack = 26,
  kParamFilterAttack = 27,
  kParamFilterDecay = 28,
  kParamFilterSustain = 29,
  kParamFilterRelease = 30,
  // LFO2
  kParamLFO2RateHz = 31,
  kParamLFO2RateTempo = 32,
  kParamLFO2RateMode = 33,
  kParamLFO2Shape = 34,
  kParamLFO2Depth = 35,
  // Delay
  kParamDelayTime = 36,
  kParamDelayFeedback = 37,
  kParamDelayDry = 38,
  kParamDelayWet = 39,
  // Sync
  kParamOscSync = 40,
  kParamOscSyncRatio = 41,
  // Reverb
  kParamReverbRoomSize = 42,
  kParamReverbDamp = 43,
  kParamReverbWidth = 44,
  kParamReverbDry = 45,
  kParamReverbWet = 46,
  kNumParams = 47
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
  [EParams.kParamLFOShape]: "kParamLFOShape",
  [EParams.kParamLFORateHz]: "LFO Rate",
  [EParams.kParamLFORateTempo]: "kParamLFORateTempo",
  [EParams.kParamLFORateMode]: "LFO Sync",
  [EParams.kParamLFODepth]: "LFO Depth",
  // Oscillators
  [EParams.kParamOsc1Mix]: "Osc1 Mix",
  [EParams.kParamOsc2Mix]: "Osc2 Mix",
  [EParams.kParamOsc3Mix]: "Osc3 Mix",
  [EParams.kParamOsc1Detune]: "Osc1 Detune",
  [EParams.kParamOsc2Detune]: "Osc2 Detune",
  [EParams.kParamOsc3Detune]: "Osc3 Detune",
  [EParams.kParamOsc1Octave]: "Osc1 Octave",
  [EParams.kParamOsc2Octave]: "Osc2 Octave",
  [EParams.kParamOsc3Octave]: "Osc3 Octave",
  [EParams.kParamOsc1Wave]: "Osc1 Wave",
  [EParams.kParamOsc2Wave]: "Osc2 Wave",
  [EParams.kParamOsc3Wave]: "Osc3 Wave",
  // Filter
  [EParams.kParamFilterCutoff]: "Filter Cutoff",
  [EParams.kParamFilterResonance]: "Filter Resonance",
  [EParams.kParamFilterEnvAmount]: "Filter Env Amount",
  [EParams.kParamFilterKeytrack]: "Filter Keytrack",
  [EParams.kParamFilterAttack]: "Filter Attack",
  [EParams.kParamFilterDecay]: "Filter Decay",
  [EParams.kParamFilterSustain]: "Filter Sustain",
  [EParams.kParamFilterRelease]: "Filter Release",
  // LFO2
  [EParams.kParamLFO2RateHz]: "LFO2 Rate",
  [EParams.kParamLFO2RateTempo]: "kParamLFO2RateTempo",
  [EParams.kParamLFO2RateMode]: "LFO2 Sync",
  [EParams.kParamLFO2Shape]: "kParamLFO2Shape",
  [EParams.kParamLFO2Depth]: "LFO2 Depth",
  // Delay
  [EParams.kParamDelayTime]: "Delay Time",
  [EParams.kParamDelayFeedback]: "Delay Feedback",
  [EParams.kParamDelayDry]: "Delay Dry",
  [EParams.kParamDelayWet]: "Delay Wet",
  // Sync
  [EParams.kParamOscSync]: "Osc Sync",
  [EParams.kParamOscSyncRatio]: "Sync Ratio",
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
