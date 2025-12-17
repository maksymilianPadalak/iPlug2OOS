# Plugin UI Components

Minimal template for AI-generated plugin UIs.

## Structure

```
components/
├── controls/       # Knob, Dropdown
├── visualizations/ # Meter
├── layouts/        # Section
└── App.tsx         # Main app (wrap in BridgeProvider)
```

## Hooks

| Hook | Returns | Use For |
|------|---------|---------|
| `useParameter` | `{ value, setValue, beginChange, endChange }` | All parameter controls |
| `useMeter` | `{ peak, rms }` | Audio level meters |
| `useMidi` | `{ activeNotes, isNoteActive }` | MIDI display |

## Controls

All controls take `paramId` from `EParams` enum:

```tsx
import { Knob } from 'sharedUi/components/Knob';
import { Dropdown } from 'sharedUi/components/Dropdown';
import { EParams } from '@/config/runtimeParameters';

<Knob paramId={EParams.kParamGain} label="Gain" />
<Dropdown paramId={EParams.kParamWaveform} label="Wave" options={['Sine', 'Saw', 'Square']} />
```

## Files Reference

- `config/runtimeParameters.ts` - Parameter definitions (generated from C++)
- `uiManifest.ts` - Available components manifest
- `sharedUi` - Shared components and DSP communication layer
