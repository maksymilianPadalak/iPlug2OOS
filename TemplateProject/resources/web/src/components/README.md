# Plugin UI Components

Minimal template for AI-generated plugin UIs. Keep it simple.

## Structure

```
components/
├── controls/       # Knob, Slider, Dropdown, Toggle
├── visualizations/ # Meter, PianoKeyboard
├── layouts/        # Section, Tabs
├── sections/       # PluginHeader, WebControls, KeyboardSection
└── App.tsx         # Main app (wrap in BridgeProvider)
```

## Hooks

| Hook | Returns | Use For |
|------|---------|---------|
| `useParameter` | `{ value, setValue, beginChange, endChange }` | All parameter controls |
| `useMeter` | `{ peak, rms }` | Audio level meters |
| `useMidi` | `{ activeNotes, isNoteActive }` | Piano keyboard, MIDI display |
| `useArbitraryMessage` | `{ data: ArrayBuffer }` | Custom visualizations (spectrum, scope) |

## Controls

All controls take `paramId` from `EParams` enum:

```tsx
import { Knob } from './controls/Knob';
import { EParams } from '../config/runtimeParameters';

<Knob paramId={EParams.kParamGain} label="Gain" size="lg" />
```

## Adding New Parameters

1. Add to C++ `EParams` enum
2. Add to `runtimeParameters.ts` (auto-generated)
3. Use in UI with any control component

## Files Reference

- `config/runtimeParameters.ts` - Parameter definitions (generated from C++)
- `contracts/integrationPatterns.ts` - Code patterns for AI
- `uiManifest.ts` - Available components manifest
- `glue/` - DSP communication layer (don't modify)
