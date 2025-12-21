# Plugin UI Components

Minimal template for AI-generated plugin UIs.

## Structure

All shared components are imported from `sharedUi/components/`:

```tsx
// Controls
import { Knob } from 'sharedUi/components/Knob';
import { Dropdown } from 'sharedUi/components/Dropdown';

// Visualizations
import { Meter } from 'sharedUi/components/Meter';
import { WaveformDisplay } from 'sharedUi/components/WaveformDisplay';

// Layouts
import { Section } from 'sharedUi/components/Section';
import { SubGroup } from 'sharedUi/components/SubGroup';

// Titles
import { Title } from 'sharedUi/components/Title';
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
