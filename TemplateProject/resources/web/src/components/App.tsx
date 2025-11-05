/**
 * Main App component - Berlin Brutalism Style
 */

import React from 'react';
import { ParameterProvider, useParameters } from './ParameterContext';
import { WAMControls } from './WAMControls';
import { Slider } from './Slider';
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
      <div className="w-full max-w-7xl mx-auto p-4">
        {/* Header */}
        <h1 className="text-white font-brutal text-4xl font-bold uppercase tracking-widest mb-6 text-center">
          TEMPLATE SYNTH
        </h1>
        
        <p id="adapterStatus" className="wam-only text-yellow-400 text-center mb-4 text-xs font-mono uppercase tracking-wider">
          Waiting for AudioWorklet initialization... Click "Start web audio!" above to begin.
        </p>

        <WAMControls />

        {/* Control Panels Grid */}
        <div className="grid grid-cols-1 md:grid-cols-3 gap-4 mb-6">
          {/* ADSR Section */}
          <div className="brutal-panel">
            <h2 className="brutal-title">ADSR</h2>
            <div className="space-y-4">
              <Slider paramIdx={EParams.kParamAttack} label="A - ATTACK" />
              <Slider paramIdx={EParams.kParamDecay} label="D - DECAY" />
              <Slider paramIdx={EParams.kParamSustain} label="S - SUSTAIN" step={0.01} />
              <Slider paramIdx={EParams.kParamRelease} label="R - RELEASE" />
            </div>
          </div>

          {/* Main Controls Section */}
          <div className="brutal-panel">
            <h2 className="brutal-title">MAIN</h2>
            <div className="space-y-6">
              <Slider paramIdx={EParams.kParamGain} label="GAIN" />
              <Slider paramIdx={EParams.kParamNoteGlideTime} label="GLIDE" />
            </div>
          </div>

          {/* LFO Section */}
          <div className="brutal-panel">
            <h2 className="brutal-title">LFO</h2>
            <div className="space-y-4">
              <Dropdown
                paramIdx={EParams.kParamLFOShape}
                label="SHAPE"
                options={['Triangle', 'Square', 'Ramp Up', 'Ramp Down', 'Sine']}
              />
              <LFOSyncCheckbox />
              <LFORateControls />
              <Slider paramIdx={EParams.kParamLFODepth} label="DEPTH" />
              <LFOWaveform />
            </div>
          </div>
        </div>

        {/* Output Level Meter */}
        <div className="brutal-panel mb-6">
          <h2 className="brutal-title text-xl">OUTPUT LEVEL</h2>
          <div className="flex items-center gap-4">
            <div className="flex-1">
              <div className="flex justify-between mb-2">
                <span className="text-white text-xs font-mono uppercase">L</span>
                <span className="text-white text-xs font-mono uppercase">R</span>
              </div>
              <div className="flex gap-2 h-48">
                <Meter channel={0} />
                <Meter channel={1} />
              </div>
              <div className="flex justify-between mt-2">
                <span className="text-gray-400 text-xs font-mono">-60</span>
                <span className="text-gray-400 text-xs font-mono">-40</span>
                <span className="text-gray-400 text-xs font-mono">-20</span>
                <span className="text-gray-400 text-xs font-mono">0</span>
              </div>
            </div>
          </div>
        </div>

        {/* Keyboard Section */}
        <div className="brutal-panel">
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
      <Slider paramIdx={EParams.kParamLFORateHz} label="RATE HZ" />
    );
  }
}
