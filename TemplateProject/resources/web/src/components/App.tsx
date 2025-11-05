/**
 * Main App component - Berlin Brutalism Style
 */

import React from 'react';
import { ParameterProvider, useParameters } from './ParameterContext';
import { WAMControls } from './WAMControls';
import { Knob } from './Knob';
import { Dropdown } from './Dropdown';
import { Checkbox } from './Checkbox';
import { Meter } from './Meter';
import { LFOWaveform } from './LFOWaveform';
import { PianoKeyboard } from './PianoKeyboard';
import { EParams } from '../config/constants';
import { initializeEnvironment } from '../utils/environment';

export function App() {
  // Initialize environment detection
  React.useEffect(() => {
    initializeEnvironment();
  }, []);

  return (
    <ParameterProvider>
      <div className="w-full h-full p-3">
        {/* Compact Header */}
        <div className="flex items-center justify-between mb-2">
          <h1 className="text-white font-brutal text-xl font-bold uppercase tracking-wider">
            TEMPLATE SYNTH
          </h1>
          <p id="adapterStatus" className="wam-only text-yellow-400 text-xs font-mono uppercase tracking-wider">
            Waiting for AudioWorklet initialization...
          </p>
        </div>

        <WAMControls />

        {/* Output Meters - Top */}
        <div className="flex items-center justify-center gap-4 mb-2">
          <Meter channel={0} compact={true} />
          <Meter channel={1} compact={true} />
        </div>

        {/* Main Horizontal Layout */}
        <div className="brutal-panel p-3">
          <div className="flex items-start gap-4">
            {/* ADSR Section */}
            <div className="flex flex-col items-center gap-2">
              <h3 className="text-white text-xs font-mono uppercase tracking-wider">ADSR</h3>
              <div className="flex gap-2">
                <Knob paramIdx={EParams.kParamAttack} label="A" />
                <Knob paramIdx={EParams.kParamDecay} label="D" />
                <Knob paramIdx={EParams.kParamSustain} label="S" step={0.01} />
                <Knob paramIdx={EParams.kParamRelease} label="R" />
              </div>
            </div>

            {/* Main Controls */}
            <div className="flex flex-col items-center gap-2">
              <h3 className="text-white text-xs font-mono uppercase tracking-wider">MAIN</h3>
              <div className="flex gap-2">
                <Knob paramIdx={EParams.kParamGain} label="GAIN" />
                <Knob paramIdx={EParams.kParamNoteGlideTime} label="GLIDE" />
              </div>
            </div>

            {/* LFO Section */}
            <div className="flex flex-col items-center gap-2">
              <h3 className="text-white text-xs font-mono uppercase tracking-wider">LFO</h3>
              <div className="flex flex-col items-center gap-2">
                <div className="flex gap-2 items-center">
                  <Dropdown
                    paramIdx={EParams.kParamLFOShape}
                    label="SHAPE"
                    options={['Triangle', 'Square', 'Ramp Up', 'Ramp Down', 'Sine']}
                  />
                  <div className="flex items-center mt-4">
                    <LFOSyncCheckbox />
                  </div>
                </div>
                <div className="flex gap-2 items-center">
                  <LFORateControls />
                  <Knob paramIdx={EParams.kParamLFODepth} label="DEPTH" />
                </div>
              </div>
            </div>
          </div>

          {/* LFO Waveform - Compact */}
          <div className="mt-3">
            <LFOWaveform />
          </div>
        </div>

        {/* Keyboard Section - Compact */}
        <div className="brutal-panel p-3 mt-2">
          <PianoKeyboard />
        </div>
      </div>
    </ParameterProvider>
  );
}

function LFOSyncCheckbox() {
  return (
    <Checkbox
      paramIdx={EParams.kParamLFORateMode}
      label="SYNC"
    />
  );
}

function LFORateControls() {
  const { paramValues } = useParameters();
  const syncValue = paramValues.get(EParams.kParamLFORateMode) ?? 1.0;
  const syncEnabled = syncValue > 0.5;

  if (syncEnabled) {
    return (
      <Dropdown
        paramIdx={EParams.kParamLFORateTempo}
        label="RATE"
        options={['1/64', '1/32', '1/16T', '1/16', '1/16D', '1/8T', '1/8', '1/8D', '1/4', '1/4D', '1/2', '1/1', '2/1', '4/1', '8/1']}
      />
    );
  } else {
    return (
      <Knob paramIdx={EParams.kParamLFORateHz} label="RATE HZ" />
    );
  }
}
