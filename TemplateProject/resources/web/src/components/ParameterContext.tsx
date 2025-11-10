/**
 * Parameter Context - React state management for iPlug2 parameters
 * 
 * CRITICAL: This bridges C++ callbacks to React state.
 * Global callbacks (SPVFD, SCVFD) must be able to update React state.
 */

import React, { createContext, useContext, useState, useCallback, useRef, useEffect } from 'react';
import { EParams, EControlTags } from '../config/constants';
import { getDefaultNormalizedValues } from '../utils/parameter';

interface ParameterState {
  // Parameter values (normalized 0-1)
  paramValues: Map<EParams, number>;
  // Control values (normalized 0-1)
  controlValues: Map<EControlTags, number>;
  // Meter values (left/right peak/rms)
  meterValues: {
    left: { peak: number; rms: number };
    right: { peak: number; rms: number };
  };
  // LFO waveform buffer
  lfoWaveform: Float32Array;
  // Flag to prevent feedback loops
  updatingFromProcessor: boolean;
}

interface ParameterContextValue extends ParameterState {
  setParamValue: (paramIdx: EParams, value: number, fromProcessor?: boolean) => void;
  setControlValue: (ctrlTag: EControlTags, value: number) => void;
  setMeterValue: (channel: number, peak: number, rms: number) => void;
  updateLFOWaveform: (value: number) => void;
  getParamValue: (paramIdx: EParams) => number;
}

const ParameterContext = createContext<ParameterContextValue | null>(null);

// Global ref to store React state setters - accessible from C++ callbacks
let globalStateSetters: {
  setParamValue: (paramIdx: EParams, value: number, fromProcessor?: boolean) => void;
  setControlValue: (ctrlTag: EControlTags, value: number) => void;
  setMeterValue: (channel: number, peak: number, rms: number) => void;
  updateLFOWaveform: (value: number) => void;
} | null = null;

// Store updating flag to prevent feedback loops
// Using a simple variable since this is accessed from global callbacks
let updatingFromProcessor = false;

/**
 * Register C++ callbacks to update React state
 * This MUST be called after React mounts
 */
export function registerCallbacksWithReact(): void {
  // SPVFD: Send Parameter Value From Delegate
  window.SPVFD = (paramIdx: number, normalizedValue: number) => {
    if (globalStateSetters) {
      updatingFromProcessor = true;
      globalStateSetters.setParamValue(paramIdx as EParams, normalizedValue, true);
      // Reset flag after a microtask to allow React to process
      Promise.resolve().then(() => {
        updatingFromProcessor = false;
      });
    }
  };

  // SCVFD: Send Control Value From Delegate
  window.SCVFD = (ctrlTag: number, normalizedValue: number) => {
    if (globalStateSetters) {
      globalStateSetters.setControlValue(ctrlTag as EControlTags, normalizedValue);
    }
  };

  // SCMFD: Send Control Message From Delegate
  window.SCMFD = (ctrlTag: number, msgTag: number, dataSize: number, base64Data: string) => {
    if (ctrlTag === EControlTags.kCtrlTagMeter && globalStateSetters) {
      // Decode meter data
      try {
        const binaryString = atob(base64Data);
        const buffer = new ArrayBuffer(binaryString.length);
        const view = new Uint8Array(buffer);
        for (let i = 0; i < binaryString.length; i++) {
          view[i] = binaryString.charCodeAt(i);
        }
        const dataView = new DataView(buffer);
        const leftPeak = dataView.getFloat32(12, true);
        const leftRMS = dataView.getFloat32(16, true);
        const rightPeak = dataView.getFloat32(20, true);
        const rightRMS = dataView.getFloat32(24, true);
        
        globalStateSetters.setMeterValue(0, leftPeak, leftRMS);
        globalStateSetters.setMeterValue(1, rightPeak, rightRMS);
      } catch (e) {
        console.error('Error decoding meter data:', e);
      }
    }
  };

  // SAMFD: Send Arbitrary Message From Delegate
  window.SAMFD = (msgTag: number, dataSize: number, base64Data: string) => {
    console.log('SAMFD received:', msgTag, dataSize, base64Data);
  };

  // SMMFD: Send MIDI Message From Delegate
  window.SMMFD = (statusByte: number, dataByte1: number, dataByte2: number) => {
    console.log('SMMFD received:', statusByte, dataByte1, dataByte2);
  };

  // StartIdleTimer: Start periodic updates
  window.StartIdleTimer = () => {
    let idleTimerInterval: number | null = null;
    
    if (idleTimerInterval) {
      clearInterval(idleTimerInterval);
    }

    idleTimerInterval = window.setInterval(() => {
      if (window.TemplateProject_WAM && typeof window.TemplateProject_WAM.sendMessage === 'function') {
        window.TemplateProject_WAM.sendMessage('TICK', '', 0);
      }
    }, 16); // ~60fps
  };

  console.log('✅ C++ callbacks registered with React state');
}

/**
 * Check if currently updating from processor (to prevent feedback loops)
 */
export function isUpdatingFromProcessor(): boolean {
  return updatingFromProcessor;
}

/**
 * Parameter Provider Component
 */
export function ParameterProvider({ children }: { children: React.ReactNode }) {
  // Initialize with default values matching C++ DSP defaults
  const [paramValues, setParamValues] = useState<Map<EParams, number>>(getDefaultNormalizedValues());
  const [controlValues, setControlValues] = useState<Map<EControlTags, number>>(new Map());
  const [meterValues, setMeterValues] = useState({
    left: { peak: 0, rms: 0 },
    right: { peak: 0, rms: 0 },
  });
  
  // LFO waveform buffer (512 samples)
  const [lfoWaveform, setLfoWaveform] = useState<Float32Array>(new Float32Array(512).fill(0.5));
  const lfoWaveformIndexRef = useRef(0);

  // State setters with proper React updates
  const setParamValue = useCallback((paramIdx: EParams, value: number, fromProcessor = false) => {
    setParamValues(prev => {
      const next = new Map(prev);
      next.set(paramIdx, value);
      return next;
    });
  }, []);

  const setControlValue = useCallback((ctrlTag: EControlTags, value: number) => {
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

  const getParamValue = useCallback((paramIdx: EParams): number => {
    return paramValues.get(paramIdx) ?? 0;
  }, [paramValues]);

  // Store setters in global ref for C++ callbacks
  useEffect(() => {
    globalStateSetters = {
      setParamValue,
      setControlValue,
      setMeterValue,
      updateLFOWaveform,
    };

    // Register callbacks after React is ready
    registerCallbacksWithReact();

    return () => {
      globalStateSetters = null;
    };
  }, [setParamValue, setControlValue, setMeterValue, updateLFOWaveform]);

  const value: ParameterContextValue = {
    paramValues,
    controlValues,
    meterValues,
    lfoWaveform,
    updatingFromProcessor: false, // This is read-only, actual flag is module-level
    setParamValue,
    setControlValue,
    setMeterValue,
    updateLFOWaveform,
    getParamValue,
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

