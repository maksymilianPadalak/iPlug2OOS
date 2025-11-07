/**
 * Tests for parameter.ts conversion functions
 */

import { EParams } from "../config/constants";
import {
  normalizedToActual,
  actualToNormalized,
  normalizedToDisplay,
  getDefaultNormalizedValues,
} from "./parameter";

describe("parameter conversion functions", () => {
  describe("normalizedToActual", () => {
    it("should clamp values to [0, 1] range", () => {
      expect(normalizedToActual(EParams.kParamGain, -0.5)).toBe(0);
      expect(normalizedToActual(EParams.kParamGain, 1.5)).toBe(100);
    });

    describe("percentage parameters (0-100)", () => {
      it("should convert Gain correctly", () => {
        expect(normalizedToActual(EParams.kParamGain, 0)).toBe(0);
        expect(normalizedToActual(EParams.kParamGain, 0.5)).toBe(50);
        expect(normalizedToActual(EParams.kParamGain, 1)).toBe(100);
      });

      it("should convert Sustain correctly", () => {
        expect(normalizedToActual(EParams.kParamSustain, 0)).toBe(0);
        expect(normalizedToActual(EParams.kParamSustain, 0.75)).toBe(75);
        expect(normalizedToActual(EParams.kParamSustain, 1)).toBe(100);
      });

      it("should convert LFO Depth correctly", () => {
        expect(normalizedToActual(EParams.kParamLFODepth, 0)).toBe(0);
        expect(normalizedToActual(EParams.kParamLFODepth, 1)).toBe(100);
      });
    });

    describe("milliseconds parameters", () => {
      it("should convert NoteGlideTime correctly (linear 0-30ms)", () => {
        expect(normalizedToActual(EParams.kParamNoteGlideTime, 0)).toBe(0);
        expect(normalizedToActual(EParams.kParamNoteGlideTime, 0.5)).toBe(15);
        expect(normalizedToActual(EParams.kParamNoteGlideTime, 1)).toBe(30);
      });
    });

    describe("power curve parameters (Attack/Decay)", () => {
      it("should convert Attack correctly (power curve 1-1000ms, shape=3)", () => {
        const result0 = normalizedToActual(EParams.kParamAttack, 0);
        const result05 = normalizedToActual(EParams.kParamAttack, 0.5);
        const result1 = normalizedToActual(EParams.kParamAttack, 1);

        expect(result0).toBeCloseTo(1, 1); // Min value
        expect(result05).toBeGreaterThan(1);
        expect(result05).toBeLessThan(1000);
        expect(result1).toBeCloseTo(1000, 1); // Max value

        // Power curve should be non-linear
        const midLow = normalizedToActual(EParams.kParamAttack, 0.25);
        const midHigh = normalizedToActual(EParams.kParamAttack, 0.75);
        const diffLow = midLow - result0;
        const diffHigh = result1 - midHigh;
        // Lower half should have smaller range than upper half (power curve)
        expect(diffLow).toBeLessThan(diffHigh);
      });

      it("should convert Decay correctly", () => {
        expect(normalizedToActual(EParams.kParamDecay, 0)).toBeCloseTo(1, 1);
        expect(normalizedToActual(EParams.kParamDecay, 1)).toBeCloseTo(1000, 1);
      });
    });

    describe("exponential frequency parameters", () => {
      it("should convert LFO Rate Hz correctly (exponential 0.01-40Hz)", () => {
        const result0 = normalizedToActual(EParams.kParamLFORateHz, 0);
        const result05 = normalizedToActual(EParams.kParamLFORateHz, 0.5);
        const result1 = normalizedToActual(EParams.kParamLFORateHz, 1);

        expect(result0).toBeCloseTo(0.01, 2);
        expect(result05).toBeGreaterThan(0.01);
        expect(result05).toBeLessThan(40);
        expect(result1).toBeCloseTo(40, 1);

        // Exponential should be non-linear
        const midLow = normalizedToActual(EParams.kParamLFORateHz, 0.25);
        const midHigh = normalizedToActual(EParams.kParamLFORateHz, 0.75);
        const diffLow = midLow - result0;
        const diffHigh = result1 - midHigh;
        // Lower half should have smaller range than upper half (exponential)
        expect(diffLow).toBeLessThan(diffHigh);
      });

      it("should convert Filter Cutoff correctly (exponential 20-20000Hz)", () => {
        const result0 = normalizedToActual(EParams.kParamFilterCutoff, 0);
        const result1 = normalizedToActual(EParams.kParamFilterCutoff, 1);

        expect(result0).toBeCloseTo(20, 1);
        expect(result1).toBeCloseTo(20000, 0);
      });
    });

    describe("boolean parameters", () => {
      it("should convert LFO Sync correctly (boolean)", () => {
        expect(normalizedToActual(EParams.kParamLFORateMode, 0)).toBe(0);
        expect(normalizedToActual(EParams.kParamLFORateMode, 0.4)).toBe(0);
        expect(normalizedToActual(EParams.kParamLFORateMode, 0.5)).toBe(0);
        expect(normalizedToActual(EParams.kParamLFORateMode, 0.51)).toBe(1);
        expect(normalizedToActual(EParams.kParamLFORateMode, 1)).toBe(1);
      });
    });

    describe("centered range parameters", () => {
      it("should convert Osc Detune correctly (-50 to 50 cents)", () => {
        expect(normalizedToActual(EParams.kParamOsc1Detune, 0)).toBe(-50);
        expect(normalizedToActual(EParams.kParamOsc1Detune, 0.5)).toBe(0);
        expect(normalizedToActual(EParams.kParamOsc1Detune, 1)).toBe(50);
      });
    });

    describe("integer range parameters", () => {
      it("should convert Osc Octave correctly (-2 to 2)", () => {
        const result0 = normalizedToActual(EParams.kParamOsc1Octave, 0);
        const result05 = normalizedToActual(EParams.kParamOsc1Octave, 0.5);
        const result1 = normalizedToActual(EParams.kParamOsc1Octave, 1);

        expect(result0).toBe(-2);
        expect(result05).toBe(0);
        expect(result1).toBe(2);
        expect(Number.isInteger(result0)).toBe(true);
        expect(Number.isInteger(result05)).toBe(true);
        expect(Number.isInteger(result1)).toBe(true);
      });

      it("should convert Osc Wave correctly (0-3 enum)", () => {
        const result0 = normalizedToActual(EParams.kParamOsc1Wave, 0);
        const result05 = normalizedToActual(EParams.kParamOsc1Wave, 0.5);
        const result1 = normalizedToActual(EParams.kParamOsc1Wave, 1);

        expect(result0).toBe(0);
        expect(result05).toBeGreaterThanOrEqual(1);
        expect(result05).toBeLessThanOrEqual(2);
        expect(result1).toBe(3);
        expect(Number.isInteger(result0)).toBe(true);
        expect(Number.isInteger(result1)).toBe(true);
      });
    });

    describe("default case", () => {
      it("should return value as-is for unknown parameters", () => {
        // Use kNumParams which shouldn't have a conversion
        const result = normalizedToActual(EParams.kNumParams as any, 0.5);
        expect(result).toBe(0.5);
      });
    });
  });

  describe("actualToNormalized", () => {
    it("should convert percentage parameters correctly", () => {
      expect(actualToNormalized(EParams.kParamGain, 0)).toBe(0);
      expect(actualToNormalized(EParams.kParamGain, 50)).toBe(0.5);
      expect(actualToNormalized(EParams.kParamGain, 100)).toBe(1);
    });

    it("should convert milliseconds parameters correctly", () => {
      expect(actualToNormalized(EParams.kParamNoteGlideTime, 0)).toBe(0);
      expect(actualToNormalized(EParams.kParamNoteGlideTime, 15)).toBe(0.5);
      expect(actualToNormalized(EParams.kParamNoteGlideTime, 30)).toBe(1);
    });

    it("should convert power curve parameters correctly", () => {
      const normalized = actualToNormalized(EParams.kParamAttack, 10);
      expect(normalized).toBeGreaterThanOrEqual(0);
      expect(normalized).toBeLessThanOrEqual(1);

      // Round trip test
      const actual = normalizedToActual(EParams.kParamAttack, normalized);
      expect(actual).toBeCloseTo(10, 1);
    });

    it("should convert exponential frequency parameters correctly", () => {
      const normalized = actualToNormalized(EParams.kParamLFORateHz, 1);
      expect(normalized).toBeGreaterThanOrEqual(0);
      expect(normalized).toBeLessThanOrEqual(1);

      // Round trip test
      const actual = normalizedToActual(EParams.kParamLFORateHz, normalized);
      expect(actual).toBeCloseTo(1, 1);
    });

    it("should convert boolean parameters correctly", () => {
      expect(actualToNormalized(EParams.kParamLFORateMode, 0)).toBe(0);
      expect(actualToNormalized(EParams.kParamLFORateMode, 1)).toBe(1);
    });

    it("should convert centered range parameters correctly", () => {
      expect(actualToNormalized(EParams.kParamOsc1Detune, -50)).toBe(0);
      expect(actualToNormalized(EParams.kParamOsc1Detune, 0)).toBe(0.5);
      expect(actualToNormalized(EParams.kParamOsc1Detune, 50)).toBe(1);
    });
  });

  describe("round-trip conversion", () => {
    it("should be reversible for percentage parameters", () => {
      const normalized = 0.75;
      const actual = normalizedToActual(EParams.kParamGain, normalized);
      const backToNormalized = actualToNormalized(EParams.kParamGain, actual);
      expect(backToNormalized).toBeCloseTo(normalized, 5);
    });

    it("should be reversible for power curve parameters", () => {
      const normalized = 0.5;
      const actual = normalizedToActual(EParams.kParamAttack, normalized);
      const backToNormalized = actualToNormalized(EParams.kParamAttack, actual);
      expect(backToNormalized).toBeCloseTo(normalized, 3);
    });

    it("should be reversible for exponential frequency parameters", () => {
      const normalized = 0.5;
      const actual = normalizedToActual(EParams.kParamLFORateHz, normalized);
      const backToNormalized = actualToNormalized(
        EParams.kParamLFORateHz,
        actual,
      );
      expect(backToNormalized).toBeCloseTo(normalized, 3);
    });

    it("should be reversible for filter cutoff", () => {
      const normalized = 0.5;
      const actual = normalizedToActual(EParams.kParamFilterCutoff, normalized);
      const backToNormalized = actualToNormalized(
        EParams.kParamFilterCutoff,
        actual,
      );
      expect(backToNormalized).toBeCloseTo(normalized, 3);
    });
  });

  describe("normalizedToDisplay", () => {
    it("should format percentage parameters correctly", () => {
      const display = normalizedToDisplay(EParams.kParamGain, 0.5);
      expect(display).toContain("50");
      expect(display).toContain("%");
    });

    it("should format milliseconds parameters correctly", () => {
      const display = normalizedToDisplay(EParams.kParamNoteGlideTime, 0.5);
      expect(display).toContain("ms");
      expect(display).toMatch(/\d+\.\d+\s*ms/);
    });

    it("should format frequency parameters correctly", () => {
      const display = normalizedToDisplay(EParams.kParamLFORateHz, 0.5);
      expect(display).toContain("Hz");
      expect(display).toMatch(/\d+\.\d+\s*Hz/);
    });

    it("should return a string", () => {
      const display = normalizedToDisplay(EParams.kParamGain, 0.5);
      expect(typeof display).toBe("string");
      expect(display.length).toBeGreaterThan(0);
    });
  });

  describe("getDefaultNormalizedValues", () => {
    it("should return a Map", () => {
      const defaults = getDefaultNormalizedValues();
      expect(defaults).toBeInstanceOf(Map);
    });

    it("should have default values for key parameters", () => {
      const defaults = getDefaultNormalizedValues();

      // Gain default: 100 (normalized = 1.0)
      expect(defaults.get(EParams.kParamGain)).toBeCloseTo(1.0, 3);

      // Sustain default: 50 (normalized = 0.5)
      expect(defaults.get(EParams.kParamSustain)).toBeCloseTo(0.5, 3);

      // Attack default: 10ms (power curve)
      const attackDefault = defaults.get(EParams.kParamAttack);
      expect(attackDefault).toBeGreaterThan(0);
      expect(attackDefault).toBeLessThan(1);
    });

    it("should have normalized values in [0, 1] range", () => {
      const defaults = getDefaultNormalizedValues();
      for (const [param, value] of defaults.entries()) {
        expect(value).toBeGreaterThanOrEqual(0);
        expect(value).toBeLessThanOrEqual(1);
      }
    });

    it("should match C++ defaults when converted back", () => {
      const defaults = getDefaultNormalizedValues();

      // Gain: 100 -> normalized 1.0 -> actual 100
      const gainNorm = defaults.get(EParams.kParamGain)!;
      const gainActual = normalizedToActual(EParams.kParamGain, gainNorm);
      expect(gainActual).toBeCloseTo(100, 1);

      // Sustain: 50 -> normalized 0.5 -> actual 50
      const sustainNorm = defaults.get(EParams.kParamSustain)!;
      const sustainActual = normalizedToActual(EParams.kParamSustain, sustainNorm);
      expect(sustainActual).toBeCloseTo(50, 1);

      // Attack: 10ms -> normalized -> actual ~10ms
      const attackNorm = defaults.get(EParams.kParamAttack)!;
      const attackActual = normalizedToActual(EParams.kParamAttack, attackNorm);
      expect(attackActual).toBeCloseTo(10, 1);
    });
  });

  describe("edge cases", () => {
    it("should handle boundary values correctly", () => {
      // Min boundary
      expect(normalizedToActual(EParams.kParamGain, 0)).toBe(0);
      expect(actualToNormalized(EParams.kParamGain, 0)).toBe(0);

      // Max boundary
      expect(normalizedToActual(EParams.kParamGain, 1)).toBe(100);
      expect(actualToNormalized(EParams.kParamGain, 100)).toBe(1);
    });

    it("should handle very small values", () => {
      const small = 0.0001;
      const actual = normalizedToActual(EParams.kParamGain, small);
      expect(actual).toBeGreaterThanOrEqual(0);
      expect(actual).toBeLessThan(1);
    });

    it("should handle values very close to 1", () => {
      const large = 0.9999;
      const actual = normalizedToActual(EParams.kParamGain, large);
      expect(actual).toBeLessThanOrEqual(100);
      expect(actual).toBeGreaterThan(99);
    });
  });
});

