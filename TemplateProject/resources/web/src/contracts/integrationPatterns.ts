/**
 * Integration Patterns - How to connect UI to the audio engine
 *
 * These patterns ARE REQUIRED for audio functionality to work.
 * Everything else (styling, layout, components) is up to you.
 */

/**
 * How to read/write parameters
 *
 * Parameters are the bridge between UI and DSP.
 * Use the useParameter hook for all parameter interactions.
 */
export const parameterPatterns = {
  /**
   * Using the useParameter hook (recommended)
   */
  useParameterHook: `
import { useParameter } from '../glue/hooks/useParameter';
import { EParams } from '../config/runtimeParameters';

function MyKnob() {
  const { value, setValue, beginChange, endChange } = useParameter(EParams.kParamGain);

  // For continuous controls (knob, slider):
  // - Call beginChange() on drag start
  // - Call setValue() during drag
  // - Call endChange() on drag end

  return (
    <MyKnobUI
      value={value}
      onDragStart={beginChange}
      onDrag={setValue}
      onDragEnd={endChange}
    />
  );
}`,

  /**
   * For instant controls (toggle, dropdown)
   */
  instantControl: `
import { useParameter } from '../glue/hooks/useParameter';
import { EParams } from '../config/runtimeParameters';

function MyToggle() {
  const { value, setValue, beginChange, endChange } = useParameter(EParams.kParamBypass);
  const isOn = value > 0.5;

  const handleToggle = () => {
    beginChange();
    setValue(isOn ? 0 : 1);
    endChange();
  };

  return <button onClick={handleToggle}>{isOn ? 'ON' : 'OFF'}</button>;
}`,
};

/**
 * How to send MIDI
 */
export const midiPatterns = {
  /**
   * Sending note on/off
   */
  sendNotes: `
import { sendNoteOn, sendNoteOff } from '../glue/iplugBridge/iplugBridge';

// Note on: noteNumber (0-127), velocity (0-127)
sendNoteOn(60, 127); // Middle C, full velocity

// Note off: noteNumber, velocity (usually 0)
sendNoteOff(60, 0);`,

  /**
   * Receiving MIDI from DSP (echo)
   */
  receiveMidi: `
import { useMidi } from '../glue/hooks/useMidi';

function MyPianoKeyboard() {
  const { activeNotes, isNoteActive } = useMidi();

  // activeNotes: Map<noteNumber, velocity>
  // isNoteActive(noteNumber): boolean

  return (
    <div>
      {[60, 62, 64, 65, 67].map(note => (
        <Key key={note} active={isNoteActive(note)} />
      ))}
    </div>
  );
}`,
};

/**
 * How to read meter data
 */
export const meterPatterns = {
  /**
   * Reading audio levels
   */
  readMeters: `
import { useMeter } from '../glue/hooks/useMeter';

function MyMeter() {
  const { peak, rms } = useMeter(0); // 0 = left, 1 = right

  // peak/rms are 0-1 linear values
  const peakDb = peak > 0 ? 20 * Math.log10(peak) : -Infinity;

  return <div>{peakDb.toFixed(1)} dB</div>;
}`,
};

/**
 * How to read arbitrary visualization data (spectrum, oscilloscope, envelope, etc.)
 */
export const visualizationPatterns = {
  /**
   * Reading arbitrary binary data from DSP
   */
  readArbitraryData: `
import { useArbitraryMessage } from '../glue/hooks/useArbitraryMessage';
import { EMsgTags } from '../config/runtimeParameters';

function MySpectrum() {
  const message = useArbitraryMessage(EMsgTags.kMsgTagSpectrum);

  if (!message) return <div>Waiting for data...</div>;

  // message.data is ArrayBuffer - convert to typed array
  const spectrum = new Float32Array(message.data);

  return (
    <canvas ref={canvasRef}>
      {/* Draw spectrum data */}
    </canvas>
  );
}`,
};

/**
 * Required context wrapper
 *
 * App must be wrapped in BridgeProvider for any of the above to work.
 */
export const contextWrapper = `
import { BridgeProvider } from '../glue/BridgeProvider';

export function App() {
  return (
    <BridgeProvider>
      {/* Your UI here - any components, any styles */}
    </BridgeProvider>
  );
}`;

/**
 * Available hooks summary
 */
export const availableHooks = {
  useParameter: 'Read/write parameter values with DAW automation support',
  useMidi: 'Read MIDI note state from DSP',
  useMeter: 'Read audio level meters (peak/rms)',
  useArbitraryMessage: 'Read raw binary data for custom visualizations',
};

/**
 * Parameter shapes - how normalized (0-1) maps to actual values
 *
 * Shape is defined in runtimeParameters.ts for each parameter.
 * The UI automatically handles conversion via normalizedToActual/actualToNormalized.
 */
export const parameterShapes = {
  ShapeLinear: 'Linear mapping: actual = min + normalized * (max - min)',
  ShapePowCurve: 'Power curve: actual = min + pow(normalized, shape) * (max - min)',
  ShapeExp: 'Exponential: actual = min * pow(max/min, normalized) - for frequency params',
};
