# TemplateProject Web Components

This folder holds the reference UI scaffold for the TemplateProject plugin. The layout is intentionally simple so AI agents can reason about it quickly.

## Sections

- `MacroControlsSection` – Core tone controls, macros, gain readouts.
- `ModulationSection` – Placeholder for LFOs/envelopes/routing.
- `EffectsSection` – Placeholder for filters, FX stacks, spatial tools.
- `PerformanceSection` – Live widgets (keyboard, pads, macros).
- `RoutingSection` – Output meters, automation, channel routing.

Each section lives in `sections/<SectionName>/` with a single export. Avoid nested sub-components unless they are truly reusable across sections.

## Hooks

- `useParameterValue` exposes `{ value, setValue, metadata }` for any `EParams` entry, pulling metadata from `config/parameters.json`. Macro sections should prefer this over reading context maps directly.

## Authoring Guidance

- **No tests:** The TemplateProject UI is AI-generated; skip Jest files under this tree.
- **Document intent:** Use short comments or copy inside sections to describe missing widgets so the agent knows what to add.
- **Keep layout flat:** `App.tsx` should import sections directly (one layer of composition) to remain easy to parse.

Future enhancements (shadcn components, richer manifests) can build on this structure without rewriting the core layout.*** End Patch

