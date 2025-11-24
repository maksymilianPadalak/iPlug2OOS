import { useCallback, useMemo } from "react";

import { EParams, ParamNames } from "../../config/constants";
import parametersJson from "../../config/parameters.json";
import { useParameters } from "../../components/system/ParameterContext";

type ParameterManifestEntry = {
  paramIdx: string;
  name: string;
  defaultValue?: number | null;
  minValue?: number | null;
  maxValue?: number | null;
  step?: number | null;
  unit?: string | null;
  automatable?: boolean | null;
};

type ParameterManifest = {
  parameters: ParameterManifestEntry[];
};

type ParameterMetadata = {
  id: EParams;
  name: string;
  minValue?: number | null;
  maxValue?: number | null;
  defaultValue?: number | null;
  step?: number | null;
  unit?: string | null;
  automatable?: boolean | null;
};

const manifest = parametersJson as ParameterManifest;

const metadataByParam = new Map<EParams, ParameterMetadata>();

for (const entry of manifest.parameters) {
  const paramIdx = coerceParamIdx(entry.paramIdx);

  if (paramIdx === null || paramIdx === EParams.kNumParams) {
    continue;
  }

  metadataByParam.set(paramIdx, {
    id: paramIdx,
    name: entry.name ?? ParamNames[paramIdx] ?? `Param ${paramIdx}`,
    minValue: entry.minValue ?? null,
    maxValue: entry.maxValue ?? null,
    defaultValue: entry.defaultValue ?? null,
    unit: entry.unit ?? null,
    step: entry.step ?? null,
    automatable: entry.automatable ?? null,
  });
}

function coerceParamIdx(paramIdx: string): EParams | null {
  if (!paramIdx) {
    return null;
  }

  if (Object.prototype.hasOwnProperty.call(EParams, paramIdx)) {
    const enumValue = EParams[paramIdx as keyof typeof EParams];
    return typeof enumValue === "number" ? (enumValue as EParams) : null;
  }

  // parameters.json entries often omit the "kParam" prefix; attempt to prepend it.
  const prefixed = `kParam${paramIdx.charAt(0).toUpperCase()}${paramIdx.slice(1)}`;
  if (Object.prototype.hasOwnProperty.call(EParams, prefixed)) {
    const enumValue = EParams[prefixed as keyof typeof EParams];
    return typeof enumValue === "number" ? (enumValue as EParams) : null;
  }

  return null;
}

function getDefaultValue(paramIdx: EParams): number {
  const metadata = metadataByParam.get(paramIdx);

  if (
    metadata?.defaultValue !== undefined &&
    metadata.defaultValue !== null &&
    metadata.maxValue &&
    metadata.maxValue !== 0
  ) {
    const min = metadata.minValue ?? 0;
    const span = metadata.maxValue - min;
    if (span > 0) {
      return (metadata.defaultValue - min) / span;
    }
  }

  return 0;
}

export function useParameterValue(paramIdx: EParams) {
  const { paramValues, setParamValue } = useParameters();

  const metadata = useMemo(() => {
    if (metadataByParam.has(paramIdx)) {
      return metadataByParam.get(paramIdx) ?? null;
    }

    return {
      id: paramIdx,
      name: ParamNames[paramIdx] ?? `Param ${paramIdx}`,
    } satisfies ParameterMetadata;
  }, [paramIdx]);

  const value = paramValues.get(paramIdx) ?? getDefaultValue(paramIdx);

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

