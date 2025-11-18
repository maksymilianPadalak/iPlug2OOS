/**
 * Generate TypeScript conversion formulas from parameter metadata
 */

import type { ParameterMetadata } from "./schemas.js";

/**
 * Generate normalizedToActual conversion code for a parameter
 */
export function generateNormalizedToActual(
  param: ParameterMetadata,
): string {
  const { paramIdx, min, max, shape, shapeParam, type, enumValues, step } = param;

  // Handle enum types
  if (type === "enum" && enumValues) {
    const maxEnum = enumValues.length - 1;
    return `Math.round(value * ${maxEnum}); // 0-${maxEnum} enum (${enumValues.length} options)`;
  }

  // Handle boolean types
  if (type === "bool") {
    return `value > 0.5 ? 1.0 : 0.0; // boolean`;
  }

  // Handle integer types
  if (type === "int") {
    const range = max - min;
    return `Math.round(value * ${range} + ${min}); // ${min} to ${max}`;
  }

  // Handle power curve
  if (shape === "ShapePowCurve" && shapeParam != null) {
    const range = max - min;
    return `${min} + Math.pow(value, ${shapeParam}) * ${range}; // Power curve: ${min}-${max}ms, shape=${shapeParam}`;
  }

  // Handle exponential (frequency mapping)
  if (shape === "ShapeExp") {
    return `(function() {
      const minFreq = ${min};
      const maxFreq = ${max};
      const mAdd = Math.log(minFreq);
      const mMul = Math.log(maxFreq / minFreq);
      return Math.exp(mAdd + value * mMul);
    })(); // Exponential: ${min}-${max}Hz`;
  }

  // Handle centered ranges (negative to positive)
  if (min < 0 && max > 0) {
    const range = max - min;
    return `(value - 0.5) * ${range}; // ${min} to ${max}`;
  }

  // Handle stepped parameters with discrete values (like OscSyncRatio)
  // Check this before linear to handle discrete stepped parameters
  if (step != null && step > 0 && (max - min) / step <= 20) {
    // Small number of discrete steps - generate array lookup
    const numSteps = Math.round((max - min) / step) + 1;
    const values: number[] = [];
    for (let i = 0; i < numSteps; i++) {
      values.push(min + i * step);
    }
    const valuesStr = values.join(", ");
    return `(function() {
      const values = [${valuesStr}];
      const idx = Math.round(value * (values.length - 1));
      return values[idx];
    })(); // Discrete steps: ${valuesStr}`;
  }

  // Handle linear with offset
  if (min !== 0) {
    const range = max - min;
    return `${min} + value * ${range}; // ${min} to ${max}`;
  }

  // Simple linear
  return `value * ${max}; // 0 to ${max}`;
}

/**
 * Format a number for use in generated code (avoid .0 after decimals)
 */
function formatNumber(num: number): string {
  // If it's an integer, add .0 for float division
  if (Number.isInteger(num)) {
    return `${num}.0`;
  }
  // If it's already a decimal, return as-is
  return num.toString();
}

/**
 * Generate actualToNormalized conversion code (inverse)
 */
export function generateActualToNormalized(
  param: ParameterMetadata,
): string {
  const { paramIdx, min, max, shape, shapeParam, type, enumValues, step } = param;

  // Handle enum types
  if (type === "enum" && enumValues) {
    const maxEnum = enumValues.length - 1;
    return `actualValue / ${formatNumber(maxEnum)}; // 0-${maxEnum} enum -> 0-1`;
  }

  // Handle boolean types
  if (type === "bool") {
    return `actualValue > 0.5 ? 1.0 : 0.0; // boolean`;
  }

  // Handle integer types
  if (type === "int") {
    const range = max - min;
    return `(actualValue - ${min}) / ${formatNumber(range)}; // ${min} to ${max} -> 0 to 1`;
  }

  // Handle power curve (inverse)
  if (shape === "ShapePowCurve" && shapeParam != null) {
    const range = max - min;
    return `Math.pow((actualValue - ${min}) / ${range}, 1.0 / ${shapeParam}); // Inverse power curve: ${min}-${max}ms, shape=${shapeParam}`;
  }

  // Handle exponential (inverse)
  if (shape === "ShapeExp") {
    return `(function() {
      const minFreq = ${min};
      const maxFreq = ${max};
      const mAdd = Math.log(minFreq);
      const mMul = Math.log(maxFreq / minFreq);
      return (Math.log(actualValue) - mAdd) / mMul;
    })(); // Inverse exponential: ${min}-${max}Hz`;
  }

  // Handle centered ranges
  if (min < 0 && max > 0) {
    const range = max - min;
    return `(actualValue / ${formatNumber(range)}) + 0.5; // ${min} to ${max} -> 0 to 1`;
  }

  // Handle stepped parameters with discrete values (inverse)
  if (step != null && step > 0 && (max - min) / step <= 20) {
    const numSteps = Math.round((max - min) / step) + 1;
    const values: number[] = [];
    for (let i = 0; i < numSteps; i++) {
      values.push(min + i * step);
    }
    return `(function() {
      const values = [${values.join(", ")}];
      const idx = values.findIndex(v => Math.abs(v - actualValue) < 0.001);
      return idx >= 0 ? idx / (values.length - 1) : 0.5;
    })(); // Inverse discrete steps`;
  }

  // Handle linear with offset
  if (min !== 0) {
    const range = max - min;
    return `(actualValue - ${min}) / ${formatNumber(range)}; // ${min} to ${max} -> 0 to 1`;
  }

  // Simple linear
  return `actualValue / ${formatNumber(max)}; // 0 to ${max} -> 0 to 1`;
}

/**
 * Generate default normalized value calculation
 */
export function generateDefaultNormalized(
  param: ParameterMetadata,
): string {
  const { default: defaultValue, min, max, shape, shapeParam, type } = param;

  // For enums and ints, use actualToNormalized logic
  if (type === "enum" || type === "int") {
    const range = max - min;
    return `actualToNormalized(EParams.${param.paramIdx}, ${defaultValue})`;
  }

  // For power curve, calculate inverse
  if (shape === "ShapePowCurve" && shapeParam != null) {
    const range = max - min;
    return `Math.pow((${defaultValue} - ${min}) / ${range}, 1.0 / ${shapeParam})`;
  }

  // For exponential, calculate inverse
  if (shape === "ShapeExp") {
    const minFreq = min;
    const maxFreq = max;
    const mAdd = `Math.log(${minFreq})`;
    const mMul = `Math.log(${maxFreq} / ${minFreq})`;
    return `(Math.log(${defaultValue}) - ${mAdd}) / ${mMul}`;
  }

  // For centered ranges
  if (min < 0 && max > 0) {
    const range = max - min;
    return `(${defaultValue} / ${range}) + 0.5`;
  }

  // For linear with offset
  if (min !== 0) {
    const range = max - min;
    return `(${defaultValue} - ${min}) / ${range}`;
  }

  // Simple linear
  return `${defaultValue} / ${max}`;
}

