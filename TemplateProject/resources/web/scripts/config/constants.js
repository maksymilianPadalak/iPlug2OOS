/**
 * Parameter indices matching C++ TemplateProject.h EParams enum
 */
export var EParams;
(function (EParams) {
    EParams[EParams["kParamGain"] = 0] = "kParamGain";
    EParams[EParams["kParamNoteGlideTime"] = 1] = "kParamNoteGlideTime";
    EParams[EParams["kParamAttack"] = 2] = "kParamAttack";
    EParams[EParams["kParamDecay"] = 3] = "kParamDecay";
    EParams[EParams["kParamSustain"] = 4] = "kParamSustain";
    EParams[EParams["kParamRelease"] = 5] = "kParamRelease";
    EParams[EParams["kParamLFOShape"] = 6] = "kParamLFOShape";
    EParams[EParams["kParamLFORateHz"] = 7] = "kParamLFORateHz";
    EParams[EParams["kParamLFORateTempo"] = 8] = "kParamLFORateTempo";
    EParams[EParams["kParamLFORateMode"] = 9] = "kParamLFORateMode";
    EParams[EParams["kParamLFODepth"] = 10] = "kParamLFODepth";
    EParams[EParams["kNumParams"] = 11] = "kNumParams";
})(EParams || (EParams = {}));
/**
 * Control tag indices matching C++ TemplateProject.h EControlTags enum
 */
export var EControlTags;
(function (EControlTags) {
    EControlTags[EControlTags["kCtrlTagMeter"] = 0] = "kCtrlTagMeter";
    EControlTags[EControlTags["kCtrlTagLFOVis"] = 1] = "kCtrlTagLFOVis";
    EControlTags[EControlTags["kCtrlTagScope"] = 2] = "kCtrlTagScope";
    EControlTags[EControlTags["kCtrlTagRTText"] = 3] = "kCtrlTagRTText";
    EControlTags[EControlTags["kCtrlTagKeyboard"] = 4] = "kCtrlTagKeyboard";
    EControlTags[EControlTags["kCtrlTagBender"] = 5] = "kCtrlTagBender";
    EControlTags[EControlTags["kNumCtrlTags"] = 6] = "kNumCtrlTags";
})(EControlTags || (EControlTags = {}));
/**
 * iPlug2 message types for UI-to-processor communication
 */
export const MessageTypes = {
    SPVFUI: "SPVFUI", // Send Parameter Value From UI
    BPCFUI: "BPCFUI", // Begin Parameter Change From UI
    EPCFUI: "EPCFUI", // End Parameter Change From UI
    SAMFUI: "SAMFUI", // Send Arbitrary Message From UI
    SMMFUI: "SMMFUI", // Send MIDI Message From UI
};
/**
 * iPlug2 message types for processor-to-UI communication
 */
export const CallbackTypes = {
    SPVFD: "SPVFD", // Send Parameter Value From Delegate
    SCVFD: "SCVFD", // Send Control Value From Delegate
    SCMFD: "SCMFD", // Send Control Message From Delegate
    SAMFD: "SAMFD", // Send Arbitrary Message From Delegate
    SMMFD: "SMMFD", // Send MIDI Message From Delegate
};
/**
 * Parameter display names
 */
export const ParamNames = {
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
export const NoteNames = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];
/**
 * MIDI status bytes
 */
export const MidiStatus = {
    NoteOn: 0x90,
    NoteOff: 0x80,
};
//# sourceMappingURL=constants.js.map