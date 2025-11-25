import { useCallback, useMemo } from "react";

import { runtimeParameters } from "../config/runtimeParameters";
import { useParameters } from "../components/system/ParameterContext";

type ParameterMetadata = {
  id: number;
  key: string;
  name: string;
  min: number;
  max: number;
  default: number;
  step: number;
  unit: string;
  group: string;
  shape: string;
  shapeParameter: number;
  enumValues: string[] | null;
  automatable: boolean;
};

// Build lookup map from runtimeParameters
const metadataByParam = new Map<number, ParameterMetadata>(
  runtimeParameters.map((p) => [p.id, p]),
);

function getDefaultNormalizedValue(paramIdx: number): number {
  const metadata = metadataByParam.get(paramIdx);

  if (!metadata) {
    return 0;
  }

  const { min, max, default: defaultValue } = metadata;
  const span = max - min;

  if (span <= 0) {
    return 0;
  }

  return (defaultValue - min) / span;
}

export function useParameterValue(paramIdx: number) {
  const { paramValues, setParamValue } = useParameters();

  const metadata = useMemo(() => {
    const meta = metadataByParam.get(paramIdx);
    if (meta) {
      return meta;
    }

    // Fallback for unknown parameters
    return {
      id: paramIdx,
      key: `kParam${paramIdx}`,
      name: `Param ${paramIdx}`,
      min: 0,
      max: 1,
      default: 0,
      step: 0.01,
      unit: "",
      group: "",
      shape: "ShapeLinear",
      shapeParameter: 0,
      enumValues: null,
      automatable: true,
    } satisfies ParameterMetadata;
  }, [paramIdx]);

  const value = paramValues.get(paramIdx) ?? getDefaultNormalizedValue(paramIdx);

  const setValue = useCallback(
    (nextValue: number) => {
      const clamped = Math.max(0, Math.min(1, nextValue));
      setParamValue(paramIdx, clamped);
    },
    [paramIdx, setParamValue],
  );

  return {
    value,
    setValue,
    metadata,
  };
}
