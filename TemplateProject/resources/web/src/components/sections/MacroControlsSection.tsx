import React, { useMemo } from "react";

import { EParams } from "../../config/constants";
import { useParameterValue } from "../../hooks/useParameterValue";
import { MasterSection } from "./MasterSection";

function formatActualValue(
  normalized: number,
  min: number | null | undefined,
  max: number | null | undefined,
) {
  if (typeof min === "number" && typeof max === "number") {
    return min + normalized * (max - min);
  }

  return normalized;
}

export function MacroControlsSection() {
  const gain = useParameterValue(EParams.kParamGain);

  const gainDisplay = useMemo(() => {
    const actual = formatActualValue(
      gain.value,
      gain.metadata?.minValue ?? 0,
      gain.metadata?.maxValue ?? 1,
    );
    const unit = gain.metadata?.unit ?? "";
    return `${actual.toFixed(1)}${unit ? ` ${unit}` : ""}`;
  }, [gain.value, gain.metadata?.minValue, gain.metadata?.maxValue, gain.metadata?.unit]);

  return (
    <section
      aria-label="Macro controls"
      className="rounded-xl border border-orange-900/40 bg-black/40 p-4 text-orange-100"
    >
      <header className="mb-4 flex items-center justify-between">
        <div className="text-xs font-semibold uppercase tracking-[0.3em] text-orange-400">
          Macro Controls
        </div>
        <p className="text-[11px] font-medium uppercase tracking-[0.3em] text-orange-200/70">
          Core tone shapers
        </p>
      </header>

      <div className="grid gap-6 md:grid-cols-[minmax(0,420px)_minmax(0,1fr)]">
        <MasterSection />

        <div className="flex flex-col gap-4 rounded-lg border border-orange-800/30 bg-white/5 p-4">
          <div className="flex flex-col gap-1">
            <div className="text-xs font-semibold uppercase tracking-[0.4em] text-orange-400/70">
              Gain Preview
            </div>
            <p className="text-sm text-orange-100/80">
              Demonstrates the `useParameterValue` hook wiring. AI-generated macros should follow the
              same pattern when binding to TemplateProject parameters.
            </p>
          </div>
          <div className="flex items-center gap-4">
            <input
              aria-label="Gain macro mock control"
              type="range"
              min={0}
              max={1}
              step={0.001}
              value={gain.value}
              onChange={(event) => gain.setValue(Number(event.currentTarget.value))}
              className="h-1 w-full cursor-pointer appearance-none rounded-full bg-orange-900/50 accent-orange-400"
            />
            <div className="text-sm font-semibold text-orange-200">{gainDisplay}</div>
          </div>
        </div>
      </div>
    </section>
  );
}

