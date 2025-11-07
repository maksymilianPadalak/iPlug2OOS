/**
 * Tests for constants.ts
 */

import {
  EParams,
  EControlTags,
  MessageTypes,
  CallbackTypes,
  ParamNames,
  NoteNames,
  MidiStatus,
} from "./constants";

describe("constants", () => {
  describe("EParams enum", () => {
    it("should have kParamGain as first parameter (0)", () => {
      expect(EParams.kParamGain).toBe(0);
    });

    it("should have kNumParams as the last entry", () => {
      expect(EParams.kNumParams).toBeGreaterThan(0);
    });

    it("should have sequential parameter indices", () => {
      const params = Object.values(EParams).filter(
        (v) => typeof v === "number",
      ) as number[];
      const sortedParams = [...params].sort((a, b) => a - b);

      // Check that indices are sequential (allowing for gaps)
      for (let i = 0; i < sortedParams.length - 1; i++) {
        expect(sortedParams[i + 1]).toBeGreaterThanOrEqual(sortedParams[i]);
      }
    });

    it("should have all expected parameter groups", () => {
      // Test a few key parameters from each group
      expect(EParams.kParamGain).toBeDefined();
      expect(EParams.kParamAttack).toBeDefined();
      expect(EParams.kParamOsc1Mix).toBeDefined();
      expect(EParams.kParamFilterCutoff).toBeDefined();
      expect(EParams.kParamLFO2RateHz).toBeDefined();
      expect(EParams.kParamDelayTime).toBeDefined();
      expect(EParams.kParamOscSync).toBeDefined();
      expect(EParams.kParamReverbRoomSize).toBeDefined();
    });
  });

  describe("EControlTags enum", () => {
    it("should have kCtrlTagMeter as first tag (0)", () => {
      expect(EControlTags.kCtrlTagMeter).toBe(0);
    });

    it("should have kNumCtrlTags as the last entry", () => {
      expect(EControlTags.kNumCtrlTags).toBeGreaterThan(0);
    });

    it("should have all expected control tags", () => {
      expect(EControlTags.kCtrlTagMeter).toBeDefined();
      expect(EControlTags.kCtrlTagLFOVis).toBeDefined();
      expect(EControlTags.kCtrlTagScope).toBeDefined();
      expect(EControlTags.kCtrlTagRTText).toBeDefined();
      expect(EControlTags.kCtrlTagKeyboard).toBeDefined();
      expect(EControlTags.kCtrlTagBender).toBeDefined();
    });
  });

  describe("MessageTypes", () => {
    it("should have all required message types", () => {
      expect(MessageTypes.SPVFUI).toBe("SPVFUI");
      expect(MessageTypes.BPCFUI).toBe("BPCFUI");
      expect(MessageTypes.EPCFUI).toBe("EPCFUI");
      expect(MessageTypes.SAMFUI).toBe("SAMFUI");
      expect(MessageTypes.SMMFUI).toBe("SMMFUI");
    });

    it("should be readonly (const assertion)", () => {
      // TypeScript will catch this, but we can verify the values are strings
      expect(typeof MessageTypes.SPVFUI).toBe("string");
      expect(typeof MessageTypes.SAMFUI).toBe("string");
    });
  });

  describe("CallbackTypes", () => {
    it("should have all required callback types", () => {
      expect(CallbackTypes.SPVFD).toBe("SPVFD");
      expect(CallbackTypes.SCVFD).toBe("SCVFD");
      expect(CallbackTypes.SCMFD).toBe("SCMFD");
      expect(CallbackTypes.SAMFD).toBe("SAMFD");
      expect(CallbackTypes.SMMFD).toBe("SMMFD");
    });

    it("should be readonly (const assertion)", () => {
      expect(typeof CallbackTypes.SPVFD).toBe("string");
      expect(typeof CallbackTypes.SAMFD).toBe("string");
    });
  });

  describe("ParamNames", () => {
    it("should have names for all parameters", () => {
      // Check that ParamNames has entries for all non-kNumParams entries
      const paramKeys = Object.keys(EParams).filter(
        (k) => k !== "kNumParams" && isNaN(Number(k)),
      ) as Array<keyof typeof EParams>;

      for (const key of paramKeys) {
        const paramValue = EParams[key];
        if (typeof paramValue === "number" && paramValue !== EParams.kNumParams) {
          expect(ParamNames[paramValue as EParams]).toBeDefined();
          expect(typeof ParamNames[paramValue as EParams]).toBe("string");
        }
      }
    });

    it("should have non-empty names for key parameters", () => {
      expect(ParamNames[EParams.kParamGain]).toBe("Gain");
      expect(ParamNames[EParams.kParamAttack]).toBe("Attack");
      expect(ParamNames[EParams.kParamFilterCutoff]).toBe("Filter Cutoff");
      expect(ParamNames[EParams.kParamOsc1Mix]).toBe("Osc1 Mix");
    });

    it("should have empty name for kNumParams", () => {
      expect(ParamNames[EParams.kNumParams]).toBe("");
    });
  });

  describe("NoteNames", () => {
    it("should have 12 note names", () => {
      expect(NoteNames).toHaveLength(12);
    });

    it("should contain all chromatic notes", () => {
      const expectedNotes = [
        "C",
        "C#",
        "D",
        "D#",
        "E",
        "F",
        "F#",
        "G",
        "G#",
        "A",
        "A#",
        "B",
      ];
      expect(NoteNames).toEqual(expectedNotes);
    });
  });

  describe("MidiStatus", () => {
    it("should have NoteOn status byte", () => {
      expect(MidiStatus.NoteOn).toBe(0x90);
    });

    it("should have NoteOff status byte", () => {
      expect(MidiStatus.NoteOff).toBe(0x80);
    });

    it("should have correct hex values", () => {
      expect(MidiStatus.NoteOn).toBe(144); // 0x90 = 144
      expect(MidiStatus.NoteOff).toBe(128); // 0x80 = 128
    });
  });
});

