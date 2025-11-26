/**
 * Hooks Manifest for AI Code Generation
 *
 * This manifest documents all available hooks for the plugin UI.
 * It is used by the AI to understand what hooks are available and how to use them.
 *
 * IMPORTANT: Keep this manifest in sync with the actual hook implementations.
 * When modifying hooks, update this manifest accordingly.
 */

export const HOOKS_MANIFEST = `
## Available Hooks

All imports use absolute paths with @/ alias.

### useParameter(paramIdx)
\`\`\`typescript
import { useParameter } from '@/glue/hooks/useParameter';
const { value, setValue, beginChange, endChange } = useParameter(paramIdx);
\`\`\`

Returns:
- \`value\`: number (0-1 normalized)
- \`setValue\`: (newValue: number) => void
- \`beginChange\`: () => void - Call on drag start for DAW automation
- \`endChange\`: () => void - Call on drag end for DAW automation

**Usage for continuous controls (knob/slider):**
\`\`\`typescript
onMouseDown: beginChange()
onMouseMove: setValue(newValue)
onMouseUp: endChange()
\`\`\`

**Usage for instant controls (toggle/dropdown):**
\`\`\`typescript
onChange: () => { beginChange(); setValue(newValue); endChange(); }
\`\`\`

### useMidi()
\`\`\`typescript
import { useMidi } from '@/glue/hooks/useMidi';
const { activeNotes, lastMessage, isNoteActive, getNoteVelocity } = useMidi();
\`\`\`

Returns:
- \`activeNotes\`: Set<number> - Currently active MIDI notes
- \`lastMessage\`: { status, data1, data2, timestamp } | null
- \`isNoteActive\`: (noteNumber: number) => boolean
- \`getNoteVelocity\`: (noteNumber: number) => number

### useMeter(channel)
\`\`\`typescript
import { useMeter } from '@/glue/hooks/useMeter';
const { peak, rms } = useMeter(0); // 0 = left, 1 = right
\`\`\`

Returns:
- \`peak\`: number - Peak level (0-1)
- \`rms\`: number - RMS level (0-1)

Per-channel subscriptions - left meter changes don't re-render right meter.

### useArbitraryMessage(msgTag)
\`\`\`typescript
import { useArbitraryMessage } from '@/glue/hooks/useArbitraryMessage';
const message = useArbitraryMessage(EMsgTags.kMsgTagLFOWaveform);
if (message) {
  const floatData = new Float32Array(message.data);
}
\`\`\`

Returns: \`{ data: ArrayBuffer, timestamp: number } | null\`

Use for LFO waveforms, spectrum data, envelope followers, etc.

## Utility Imports

### Parameter Display
\`\`\`typescript
import { normalizedToDisplay } from '@/utils/parameter';
const displayValue = normalizedToDisplay(paramIdx, value); // Returns "80 %" etc.
\`\`\`

### Parameter Metadata
\`\`\`typescript
import { runtimeParameters, EParams } from '@/config/runtimeParameters';
// Access min, max, unit, type, etc.
\`\`\`

## Note on PianoKeyboard
The PianoKeyboard component is handled separately and is NOT part of this UI contract.
It's a fixed utility component only visible in web/standalone mode for testing without a MIDI controller.
`.trim();
