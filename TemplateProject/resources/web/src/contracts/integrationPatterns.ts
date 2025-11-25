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
 * Use these patterns to create interactive controls.
 */
export const parameterPatterns = {
  /**
   * Reading a parameter value
   */
  readParameter: `
import { useParameters } from './system/ParameterContext';
import { EParams } from '../config/runtimeParameters';

function MyComponent() {
  const { paramValues } = useParameters();
  const value = paramValues.get(EParams.kParamGain) ?? 0; // 0-1 normalized
  return <div>{value}</div>;
}`,

  /**
   * Writing a parameter value (updating from UI)
   */
  writeParameter: `
import { useParameters, isUpdatingFromProcessor } from './system/ParameterContext';
import { sendParameterValue } from '../glue/iplugBridge/iplugBridge';
import { EParams } from '../config/runtimeParameters';

function MyControl() {
  const { paramValues, setParamValue } = useParameters();
  const value = paramValues.get(EParams.kParamGain) ?? 0;

  const handleChange = (newValue: number) => {
    setParamValue(EParams.kParamGain, newValue);
    
    // Send to audio processor (skip if value came from processor)
    if (!isUpdatingFromProcessor()) {
      sendParameterValue(EParams.kParamGain, newValue);
    }
  };

  return <input type="range" value={value} onChange={e => handleChange(Number(e.target.value))} />;
}`,

  /**
   * Using the useParameterValue hook (simpler API)
   */
  useParameterValueHook: `
import { useParameterValue } from '../hooks/useParameterValue';
import { EParams } from '../config/runtimeParameters';

function MyKnob() {
  const { value, setValue } = useParameterValue(EParams.kParamGain);
  // value is 0-1 normalized, setValue handles the bridge communication
  return <MyKnobUI value={value} onChange={setValue} />;
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
};

/**
 * How to read meter data
 */
export const meterPatterns = {
  /**
   * Reading audio levels
   */
  readMeters: `
import { useParameters } from './system/ParameterContext';

function MyMeter() {
  const { meterValues } = useParameters();
  
  // meterValues.left.peak - left channel peak (0-1 linear)
  // meterValues.left.rms - left channel RMS
  // meterValues.right.peak - right channel peak
  // meterValues.right.rms - right channel RMS
  
  const leftDb = meterValues.left.peak > 0 
    ? 20 * Math.log10(meterValues.left.peak) 
    : -Infinity;
    
  return <div>{leftDb.toFixed(1)} dB</div>;
}`,
};

/**
 * How to read LFO waveform data
 */
export const lfoPatterns = {
  /**
   * Reading LFO waveform buffer
   */
  readLfoWaveform: `
import { useParameters } from './system/ParameterContext';

function MyLfoDisplay() {
  const { lfoWaveform } = useParameters();
  
  // lfoWaveform is Float32Array(512) with values 0-1
  // Values update in real-time from the DSP
  
  return (
    <canvas ref={canvasRef}>
      {/* Draw lfoWaveform data */}
    </canvas>
  );
}`,
};

/**
 * Required context wrapper
 * 
 * App must be wrapped in ParameterProvider for any of the above to work.
 */
export const contextWrapper = `
import { ParameterProvider } from './system/ParameterContext';

export function App() {
  return (
    <ParameterProvider>
      {/* Your UI here - any components, any styles */}
    </ParameterProvider>
  );
}`;

/**
 * EParams enum - parameter IDs defined in config/runtimeParameters.ts
 * 
 * This enum is AUTO-GENERATED from the C++ DSP code.
 * Reference it to know what parameters exist.
 */
export const parameterReference = `
// Check config/runtimeParameters.ts for the actual enum:
export enum EParams {
  kParamGain = 0,
  // ... more parameters added by AI during DSP generation
  kNumParams = N
}

// Use like:
<Knob paramId={EParams.kParamGain} />
`;

