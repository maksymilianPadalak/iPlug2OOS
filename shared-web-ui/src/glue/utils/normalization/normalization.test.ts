import { describe, it, expect } from 'vitest';
import { fromNormalized, toNormalized, toNormalizedStep, snapToStep } from './normalization';

describe('fromNormalized', () => {
  describe('ShapeLinear', () => {
    it('should convert 0 to min', () => {
      expect(fromNormalized(0, 0, 100)).toBe(0);
      expect(fromNormalized(0, 20, 200)).toBe(20);
    });

    it('should convert 1 to max', () => {
      expect(fromNormalized(1, 0, 100)).toBe(100);
      expect(fromNormalized(1, 20, 200)).toBe(200);
    });

    it('should convert 0.5 to midpoint', () => {
      expect(fromNormalized(0.5, 0, 100)).toBe(50);
      expect(fromNormalized(0.5, 20, 200)).toBe(110);
    });

    it('should use ShapeLinear as default', () => {
      expect(fromNormalized(0.5, 0, 100)).toBe(50);
    });
  });

  describe('ShapePowCurve', () => {
    it('should apply power curve with exponent 2', () => {
      // 0.5^2 * 100 = 25
      expect(fromNormalized(0.5, 0, 100, 'ShapePowCurve', 2)).toBe(25);
    });

    it('should convert endpoints correctly', () => {
      expect(fromNormalized(0, 0, 100, 'ShapePowCurve', 2)).toBe(0);
      expect(fromNormalized(1, 0, 100, 'ShapePowCurve', 2)).toBe(100);
    });
  });

  describe('ShapeExp', () => {
    it('should convert endpoints correctly', () => {
      expect(fromNormalized(0, 20, 20000, 'ShapeExp')).toBeCloseTo(20, 5);
      expect(fromNormalized(1, 20, 20000, 'ShapeExp')).toBeCloseTo(20000, 0);
    });

    it('should handle min <= 0 by using safe minimum', () => {
      // Should not throw, uses 0.00000001 as safe min
      expect(fromNormalized(0, 0, 100, 'ShapeExp')).toBeCloseTo(0.00000001, 8);
    });
  });
});

describe('toNormalized', () => {
  describe('ShapeLinear', () => {
    it('should convert min to 0', () => {
      expect(toNormalized(0, 0, 100)).toBe(0);
      expect(toNormalized(20, 20, 200)).toBe(0);
    });

    it('should convert max to 1', () => {
      expect(toNormalized(100, 0, 100)).toBe(1);
      expect(toNormalized(200, 20, 200)).toBe(1);
    });

    it('should convert midpoint to 0.5', () => {
      expect(toNormalized(50, 0, 100)).toBe(0.5);
      expect(toNormalized(110, 20, 200)).toBe(0.5);
    });

    it('should return 0 when range is 0', () => {
      expect(toNormalized(50, 50, 50)).toBe(0);
    });
  });

  describe('roundtrip', () => {
    it('should be inverse of fromNormalized for ShapeLinear', () => {
      const normalized = 0.7;
      const actual = fromNormalized(normalized, 0, 100, 'ShapeLinear');
      expect(toNormalized(actual, 0, 100, 'ShapeLinear')).toBeCloseTo(normalized, 10);
    });

    it('should be inverse of fromNormalized for ShapePowCurve', () => {
      const normalized = 0.7;
      const actual = fromNormalized(normalized, 0, 100, 'ShapePowCurve', 2);
      expect(toNormalized(actual, 0, 100, 'ShapePowCurve', 2)).toBeCloseTo(normalized, 10);
    });

    it('should be inverse of fromNormalized for ShapeExp', () => {
      const normalized = 0.7;
      const actual = fromNormalized(normalized, 20, 20000, 'ShapeExp');
      expect(toNormalized(actual, 20, 20000, 'ShapeExp')).toBeCloseTo(normalized, 10);
    });
  });
});

describe('toNormalizedStep', () => {
  it('should convert step to normalized range', () => {
    // step 1 in range 0-100 = 0.01 normalized
    expect(toNormalizedStep(1, 0, 100)).toBe(0.01);
  });

  it('should handle decimal steps', () => {
    // step 0.1 in range 0-100 = 0.001 normalized
    expect(toNormalizedStep(0.1, 0, 100)).toBe(0.001);
  });

  it('should handle non-zero min', () => {
    // step 10 in range 100-200 = 0.1 normalized
    expect(toNormalizedStep(10, 100, 200)).toBe(0.1);
  });

  it('should return 0 when range is 0', () => {
    expect(toNormalizedStep(1, 50, 50)).toBe(0);
  });
});

describe('snapToStep', () => {
  it('should snap to nearest step', () => {
    // step 0.1, value 0.12 should snap to 0.1
    expect(snapToStep(0.12, 0.1)).toBeCloseTo(0.1, 10);
    // step 0.1, value 0.18 should snap to 0.2
    expect(snapToStep(0.18, 0.1)).toBeCloseTo(0.2, 10);
  });

  it('should clamp to 0-1 range', () => {
    expect(snapToStep(-0.1, 0.1)).toBe(0);
    expect(snapToStep(1.1, 0.1)).toBe(1);
  });

  it('should return value unchanged when step is 0', () => {
    expect(snapToStep(0.123, 0)).toBe(0.123);
  });

  it('should return value unchanged when step is negative', () => {
    expect(snapToStep(0.123, -0.1)).toBe(0.123);
  });

  it('should handle very small steps', () => {
    // step 0.001, value 0.1234 should snap to 0.123
    expect(snapToStep(0.1234, 0.001)).toBeCloseTo(0.123, 10);
  });
});
