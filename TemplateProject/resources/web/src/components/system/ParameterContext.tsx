import React, { createContext, useContext, useState, useCallback, useRef } from "react";

import { useParameterBridge } from "../../glue/hooks/useParameterBridge";
import { getDefaultNormalizedValues } from "../../utils/parameter";

export { isUpdatingFromProcessor } from "../../glue/processorCallbacks/processorCallbacks";

// MIDI message types
type MidiMessage = {
  statusByte: number;
  dataByte1: number;
  dataByte2: number;
  timestamp: number;
};

// Arbitrary message from DSP
type ArbitraryMessage = {
  msgTag: number;
  dataSize: number;
  data: ArrayBuffer;
  timestamp: number;
};

interface ParameterState {
  // Parameter values (normalized 0-1)
  paramValues: Map<number, number>;
  // Control values (normalized 0-1)
  controlValues: Map<number, number>;
  // Meter values (left/right peak/rms)
  meterValues: {
    left: { peak: number; rms: number };
    right: { peak: number; rms: number };
  };
  // LFO waveform buffer
  lfoWaveform: Float32Array;
  // Active MIDI notes (note number -> velocity)
  activeNotes: Map<number, number>;
  // Last MIDI message received
  lastMidiMessage: MidiMessage | null;
  // Last arbitrary message received
  lastArbitraryMessage: ArbitraryMessage | null;
  // Flag to prevent feedback loops
  updatingFromProcessor: boolean;
}

interface ParameterContextValue extends ParameterState {
  setParamValue: (paramIdx: number, value: number, fromProcessor?: boolean) => void;
  setControlValue: (ctrlTag: number, value: number) => void;
  setMeterValue: (channel: number, peak: number, rms: number) => void;
  updateLFOWaveform: (value: number) => void;
  getParamValue: (paramIdx: number) => number;
  // Check if a note is currently active
  isNoteActive: (noteNumber: number) => boolean;
  // Get velocity of active note (0 if not active)
  getNoteVelocity: (noteNumber: number) => number;
}

const ParameterContext = createContext<ParameterContextValue | null>(null);

/**
 * Parameter Provider Component
 */
export function ParameterProvider({ children }: { children: React.ReactNode }) {
  // Initialize with default values matching C++ DSP defaults
  const [paramValues, setParamValues] = useState<Map<number, number>>(getDefaultNormalizedValues());
  const [controlValues, setControlValues] = useState<Map<number, number>>(new Map());
  const [meterValues, setMeterValues] = useState({
    left: { peak: 0, rms: 0 },
    right: { peak: 0, rms: 0 },
  });

  // LFO waveform buffer (512 samples)
  const [lfoWaveform, setLfoWaveform] = useState<Float32Array>(new Float32Array(512).fill(0.5));
  const lfoWaveformIndexRef = useRef(0);

  // MIDI state
  const [activeNotes, setActiveNotes] = useState<Map<number, number>>(new Map());
  const [lastMidiMessage, setLastMidiMessage] = useState<MidiMessage | null>(null);

  // Arbitrary message state
  const [lastArbitraryMessage, setLastArbitraryMessage] = useState<ArbitraryMessage | null>(null);

  // State setters with proper React updates
  const setParamValue = useCallback((paramIdx: number, value: number, _fromProcessor = false) => {
    setParamValues(prev => {
      const next = new Map(prev);
      next.set(paramIdx, value);
      return next;
    });
  }, []);

  const setControlValue = useCallback((ctrlTag: number, value: number) => {
    setControlValues(prev => {
      const next = new Map(prev);
      next.set(ctrlTag, value);
      return next;
    });
  }, []);

  const setMeterValue = useCallback((channel: number, peak: number, rms: number) => {
    setMeterValues(prev => ({
      ...prev,
      [channel === 0 ? 'left' : 'right']: { peak, rms },
    }));
  }, []);

  const updateLFOWaveform = useCallback((value: number) => {
    setLfoWaveform(prev => {
      const next = new Float32Array(prev);
      next[lfoWaveformIndexRef.current] = value;
      lfoWaveformIndexRef.current = (lfoWaveformIndexRef.current + 1) % prev.length;
      return next;
    });
  }, []);

  const getParamValue = useCallback((paramIdx: number): number => {
    return paramValues.get(paramIdx) ?? 0;
  }, [paramValues]);

  // MIDI helpers
  const isNoteActive = useCallback((noteNumber: number): boolean => {
    return activeNotes.has(noteNumber);
  }, [activeNotes]);

  const getNoteVelocity = useCallback((noteNumber: number): number => {
    return activeNotes.get(noteNumber) ?? 0;
  }, [activeNotes]);

  // Handle MIDI message from processor
  const handleMidiMessage = useCallback((statusByte: number, dataByte1: number, dataByte2: number) => {
    const timestamp = Date.now();
    setLastMidiMessage({ statusByte, dataByte1, dataByte2, timestamp });

    // Parse MIDI message type (high nibble of status byte)
    const messageType = statusByte & 0xF0;
    const noteNumber = dataByte1;
    const velocity = dataByte2;

    if (messageType === 0x90 && velocity > 0) {
      // Note On
      setActiveNotes(prev => {
        const next = new Map(prev);
        next.set(noteNumber, velocity);
        return next;
      });
    } else if (messageType === 0x80 || (messageType === 0x90 && velocity === 0)) {
      // Note Off
      setActiveNotes(prev => {
        const next = new Map(prev);
        next.delete(noteNumber);
        return next;
      });
    }
  }, []);

  // Handle arbitrary message from processor
  const handleArbitraryMessage = useCallback((msgTag: number, dataSize: number, data: ArrayBuffer) => {
    const timestamp = Date.now();
    setLastArbitraryMessage({ msgTag, dataSize, data, timestamp });
  }, []);

  useParameterBridge({
    onParameterValue: (paramIdx, normalizedValue) => {
      setParamValue(paramIdx, normalizedValue, true);
    },
    onControlValue: (ctrlTag, normalizedValue) => {
      setControlValue(ctrlTag, normalizedValue);
    },
    onMeterData: (channel, peak, rms) => {
      setMeterValue(channel, peak, rms);
    },
    onMidiMessage: handleMidiMessage,
    onArbitraryMessage: handleArbitraryMessage,
  });

  const value: ParameterContextValue = {
    paramValues,
    controlValues,
    meterValues,
    lfoWaveform,
    activeNotes,
    lastMidiMessage,
    lastArbitraryMessage,
    updatingFromProcessor: false, // This is read-only, actual flag is module-level
    setParamValue,
    setControlValue,
    setMeterValue,
    updateLFOWaveform,
    getParamValue,
    isNoteActive,
    getNoteVelocity,
  };

  return (
    <ParameterContext.Provider value={value}>
      {children}
    </ParameterContext.Provider>
  );
}

/**
 * Hook to access parameter context
 */
export function useParameters(): ParameterContextValue {
  const context = useContext(ParameterContext);
  if (!context) {
    throw new Error('useParameters must be used within ParameterProvider');
  }
  return context;
}
