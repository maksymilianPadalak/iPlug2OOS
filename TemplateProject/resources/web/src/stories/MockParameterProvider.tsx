/**
 * Mock Parameter Provider for Storybook
 * Provides a standalone context without the iPlug2 bridge
 */

import React, { createContext, useContext, useState, useCallback } from 'react';
import { EParams, EControlTags } from '../config/constants';

interface ParameterContextValue {
  paramValues: Map<EParams, number>;
  controlValues: Map<EControlTags, number>;
  meterValues: {
    left: { peak: number; rms: number };
    right: { peak: number; rms: number };
  };
  lfoWaveform: Float32Array;
  updatingFromProcessor: boolean;
  setParamValue: (paramIdx: EParams, value: number, fromProcessor?: boolean) => void;
  setControlValue: (ctrlTag: EControlTags, value: number) => void;
  setMeterValue: (channel: number, peak: number, rms: number) => void;
  updateLFOWaveform: (value: number) => void;
  getParamValue: (paramIdx: EParams) => number;
}

const MockParameterContext = createContext<ParameterContextValue | null>(null);

export function MockParameterProvider({ children }: { children: React.ReactNode }) {
  const [paramValues, setParamValues] = useState<Map<EParams, number>>(() => {
    const map = new Map<EParams, number>();
    // Set some default values for demo
    map.set(EParams.kParamGain, 0.75);
    return map;
  });

  const [controlValues, setControlValues] = useState<Map<EControlTags, number>>(new Map());
  const [meterValues] = useState({
    left: { peak: 0.5, rms: 0.3 },
    right: { peak: 0.5, rms: 0.3 },
  });
  const [lfoWaveform] = useState<Float32Array>(new Float32Array(512).fill(0.5));

  const setParamValue = useCallback((paramIdx: EParams, value: number) => {
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

  const setMeterValue = useCallback(() => {}, []);
  const updateLFOWaveform = useCallback(() => {}, []);

  const getParamValue = useCallback((paramIdx: EParams): number => {
    return paramValues.get(paramIdx) ?? 0.5;
  }, [paramValues]);

  const value: ParameterContextValue = {
    paramValues,
    controlValues,
    meterValues,
    lfoWaveform,
    updatingFromProcessor: false,
    setParamValue,
    setControlValue,
    setMeterValue,
    updateLFOWaveform,
    getParamValue,
  };

  return (
    <MockParameterContext.Provider value={value}>
      {children}
    </MockParameterContext.Provider>
  );
}

// Override useParameters for storybook
export function useParametersMock(): ParameterContextValue {
  const context = useContext(MockParameterContext);
  if (!context) {
    throw new Error('useParametersMock must be used within MockParameterProvider');
  }
  return context;
}

