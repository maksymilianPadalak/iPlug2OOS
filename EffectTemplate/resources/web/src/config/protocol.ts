/**
 * iPlug2 Protocol Constants
 * These are framework-level constants that never change.
 */

// UI to Processor messages
export const MessageTypes = {
  SPVFUI: "SPVFUI", // Send Parameter Value From UI
  BPCFUI: "BPCFUI", // Begin Parameter Change From UI
  EPCFUI: "EPCFUI", // End Parameter Change From UI
  SAMFUI: "SAMFUI", // Send Arbitrary Message From UI
  SMMFUI: "SMMFUI", // Send MIDI Message From UI
  SKPFUI: "SKPFUI", // Send Key Press From UI
  SREQ: "SREQ",     // State Request - UI asks DSP for full state sync
} as const;

// Processor to UI callbacks
export const CallbackTypes = {
  SPVFD: "SPVFD",   // Send Parameter Value From Delegate
  SCVFD: "SCVFD",   // Send Control Value From Delegate
  SCMFD: "SCMFD",   // Send Control Message From Delegate
  SAMFD: "SAMFD",   // Send Arbitrary Message From Delegate
  SMMFD: "SMMFD",   // Send MIDI Message From Delegate
  SSTATE: "SSTATE", // Full state dump from DSP (response to SREQ)
} as const;

export type MessageType = (typeof MessageTypes)[keyof typeof MessageTypes];
export type CallbackType = (typeof CallbackTypes)[keyof typeof CallbackTypes];
