import manifestJson from "./parameters.json";
import { EParams } from "./constants";

type ParameterManifestEntry = {
  paramIdx: string;
  name: string;
  index: number;
};

type ParameterManifest = {
  parameters: ParameterManifestEntry[];
};

const manifest = manifestJson as ParameterManifest;

const lookupByIdx = new Map(
  manifest.parameters.map((entry) => [entry.paramIdx, entry]),
);

const lookupByName = new Map(
  manifest.parameters.map((entry) => [normalize(entry.name), entry]),
);

function normalize(value: string): string {
  return value.toLowerCase().replace(/[^a-z0-9]+/g, "");
}

export function resolveParamIdx(
  candidates: string[],
): EParams | null {
  for (const candidate of candidates) {
    const normalized = normalize(candidate);
    const entry =
      lookupByIdx.get(candidate) ||
      lookupByIdx.get(`kParam${candidate}`) ||
      lookupByName.get(normalized);

    if (entry) {
      const key = entry.paramIdx as keyof typeof EParams;
      if (key in EParams) {
        return EParams[key];
      }
    }
  }

  return null;
}

