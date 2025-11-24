import React, { createContext, useContext, useState, useCallback, useRef } from "react";

import { useParameterBridge } from "../../glue/hooks/useParameterBridge";
import { EParams, EControlTags } from "../../config/constants";
import { getDefaultNormalizedValues } from "../../utils/parameter";

export { isUpdatingFromProcessor } from "../../glue/processorCallbacks/processorCallbacks";

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

  useParameterBridge({
    onParameterValue: (paramIdx, normalizedValue) => {
      setParamValue(paramIdx as EParams, normalizedValue, true);
    },
    onControlValue: (ctrlTag, normalizedValue) => {
      setControlValue(ctrlTag as EControlTags, normalizedValue);
    },
    onMeterData: (channel, peak, rms) => {
      setMeterValue(channel, peak, rms);
    },
  });

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
