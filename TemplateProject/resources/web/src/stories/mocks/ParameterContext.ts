/**
 * Mock Parameter Context for Storybook
 * Re-exports from MockParameterProvider with matching API
 */

import React, { createContext, useContext, useState, useCallback } from 'react';
import { EParams } from '../../config/runtimeParameters';

// Always return false in Storybook (no real processor)
export function isUpdatingFromProcessor(): boolean {
  return false;
}

interface ParameterContextValue {
  paramValues: Map<number, number>;
  controlValues: Map<number, number>;
  meterValues: {
    left: { peak: number; rms: number };
    right: { peak: number; rms: number };
  };
  lfoWaveform: Float32Array;
  activeNotes: Map<number, number>;
  lastMidiMessage: { statusByte: number; dataByte1: number; dataByte2: number; timestamp: number } | null;
  lastArbitraryMessage: { msgTag: number; dataSize: number; data: ArrayBuffer; timestamp: number } | null;
  updatingFromProcessor: boolean;
  setParamValue: (paramIdx: number, value: number, fromProcessor?: boolean) => void;
  setControlValue: (ctrlTag: number, value: number) => void;
  setMeterValue: (channel: number, peak: number, rms: number) => void;
  updateLFOWaveform: (value: number) => void;
  getParamValue: (paramIdx: number) => number;
  isNoteActive: (noteNumber: number) => boolean;
  getNoteVelocity: (noteNumber: number) => number;
}

const ParameterContext = createContext<ParameterContextValue | null>(null);

export function ParameterProvider({ children }: { children: React.ReactNode }) {
  const [paramValues, setParamValues] = useState<Map<number, number>>(() => {
    const map = new Map<number, number>();
    map.set(EParams.kParamGain, 0.75);
    return map;
  });

  const [controlValues, setControlValues] = useState<Map<number, number>>(new Map());
  const [meterValues] = useState({
    left: { peak: 0.5, rms: 0.3 },
    right: { peak: 0.5, rms: 0.3 },
  });
  const [lfoWaveform] = useState<Float32Array>(new Float32Array(512).fill(0.5));
  const [activeNotes] = useState<Map<number, number>>(new Map());

  const setParamValue = useCallback((paramIdx: number, value: number) => {
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

  const setMeterValue = useCallback(() => {}, []);
  const updateLFOWaveform = useCallback(() => {}, []);

  const getParamValue = useCallback((paramIdx: number): number => {
    return paramValues.get(paramIdx) ?? 0.5;
  }, [paramValues]);

  const isNoteActive = useCallback((noteNumber: number): boolean => {
    return activeNotes.has(noteNumber);
  }, [activeNotes]);

  const getNoteVelocity = useCallback((noteNumber: number): number => {
    return activeNotes.get(noteNumber) ?? 0;
  }, [activeNotes]);

  const value: ParameterContextValue = {
    paramValues,
    controlValues,
    meterValues,
    lfoWaveform,
    activeNotes,
    lastMidiMessage: null,
    lastArbitraryMessage: null,
    updatingFromProcessor: false,
    setParamValue,
    setControlValue,
    setMeterValue,
    updateLFOWaveform,
    getParamValue,
    isNoteActive,
    getNoteVelocity,
  };

  return React.createElement(
    ParameterContext.Provider,
    { value },
    children
  );
}

export function useParameters(): ParameterContextValue {
  const context = useContext(ParameterContext);
  if (!context) {
    throw new Error('useParameters must be used within ParameterProvider');
  }
  return context;
}


