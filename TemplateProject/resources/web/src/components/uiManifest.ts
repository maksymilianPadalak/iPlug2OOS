type UiSectionSpec = {
  id: "macro_controls" | "modulation" | "effects" | "performance" | "routing";
  title: string;
  description: string;
  entryPoint: string;
  recommendedComponents: string[];
  notes?: string;
};

export const uiSectionsManifest: UiSectionSpec[] = [
  {
    id: "macro_controls",
    title: "Macro Controls",
    description:
      "Primary gain/macro surface. Houses high-impact knobs, XY pads, and macro assignments.",
    entryPoint: "sections/MacroControlsSection.tsx",
    recommendedComponents: ["MasterSection", "useParameterValue", "Knob"],
    notes: "Bind each macro to explicit EParams IDs so the AI can reason about routing.",
  },
  {
    id: "modulation",
    title: "Modulation",
    description: "LFOs, envelopes, modulation matrix, performance macros.",
    entryPoint: "sections/ModulationSection.tsx",
    recommendedComponents: ["Tabs", "useParameterValue"],
  },
  {
    id: "effects",
    title: "Effects",
    description: "Filter, delay, reverb, distortion, and spatial processing blocks.",
    entryPoint: "sections/EffectsSection.tsx",
    recommendedComponents: ["Knob", "Slider"],
  },
  {
    id: "performance",
    title: "Performance",
    description: "Live play surface (keyboard, pads, macros) + capture controls.",
    entryPoint: "sections/PerformanceSection.tsx",
    recommendedComponents: ["KeyboardSection", "useParameterValue"],
  },
  {
    id: "routing",
    title: "Routing & Output",
    description: "Meters, automation statuses, multichannel routing, DAW integration.",
    entryPoint: "sections/RoutingSection.tsx",
    recommendedComponents: ["OutputMeters"],
  },
];

