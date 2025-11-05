/**
 * Main App component - Berlin Brutalism Style with Tabs
 */

import React, { useState } from 'react';
import { ParameterProvider, useParameters } from './ParameterContext';
import { WAMControls } from './WAMControls';
import { Knob } from './Knob';
import { Dropdown } from './Dropdown';
import { Checkbox } from './Checkbox';
import { Meter } from './Meter';
import { LFOWaveform } from './LFOWaveform';
import { PianoKeyboard } from './PianoKeyboard';
import { TabContainer } from './Tabs';
import { EParams } from '../config/constants';
import { initializeEnvironment } from '../utils/environment';

export function App() {
  const [activeTab, setActiveTab] = useState(0);
  
  // Initialize environment detection
  React.useEffect(() => {
    initializeEnvironment();
  }, []);

  const tabs = ['OSC', 'FILTER', 'ENV', 'LFO', 'FX', 'MAIN'];

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

        {/* Tabbed Interface */}
        <TabContainer tabs={tabs} activeTab={activeTab} onTabChange={setActiveTab}>
          {activeTab === 0 && <OscillatorsPage />}
          {activeTab === 1 && <FilterPage />}
          {activeTab === 2 && <EnvelopePage />}
          {activeTab === 3 && <LFOPage />}
          {activeTab === 4 && <EffectsPage />}
          {activeTab === 5 && <MainPage />}
        </TabContainer>

        {/* Keyboard Section - Always visible */}
        <div className="brutal-panel p-3 mt-2">
          <PianoKeyboard />
        </div>
      </div>
    </ParameterProvider>
  );
}

function OscillatorsPage() {
  return (
    <div className="flex flex-col gap-4">
      <h2 className="text-white text-sm font-mono uppercase tracking-wider mb-2">OSCILLATORS</h2>
      <div className="flex gap-4 justify-center flex-wrap">
        <OscillatorControls oscNum={1} />
        <OscillatorControls oscNum={2} />
        <OscillatorControls oscNum={3} />
      </div>
    </div>
  );
}

function FilterPage() {
  return (
    <div className="flex flex-col gap-4">
      <h2 className="text-white text-sm font-mono uppercase tracking-wider mb-2">FILTER</h2>
      <div className="flex flex-col items-center gap-3">
        <div className="flex gap-2">
          <Knob paramIdx={EParams.kParamFilterCutoff} label="CUTOFF" />
          <Knob paramIdx={EParams.kParamFilterResonance} label="RES" />
        </div>
        <div className="flex gap-2">
          <Knob paramIdx={EParams.kParamFilterEnvAmount} label="ENV" />
          <Knob paramIdx={EParams.kParamFilterKeytrack} label="KEY" />
        </div>
        <div className="mt-2">
          <h3 className="text-white text-xs font-mono uppercase tracking-wider mb-2">FILTER ADSR</h3>
          <div className="flex gap-2">
            <Knob paramIdx={EParams.kParamFilterAttack} label="FA" />
            <Knob paramIdx={EParams.kParamFilterDecay} label="FD" />
            <Knob paramIdx={EParams.kParamFilterSustain} label="FS" step={0.01} />
            <Knob paramIdx={EParams.kParamFilterRelease} label="FR" />
          </div>
        </div>
      </div>
    </div>
  );
}

function EnvelopePage() {
  return (
    <div className="flex flex-col gap-4">
      <h2 className="text-white text-sm font-mono uppercase tracking-wider mb-2">AMPLITUDE ENVELOPE</h2>
      <div className="flex flex-col items-center gap-3">
        <div className="flex gap-2">
          <Knob paramIdx={EParams.kParamAttack} label="ATTACK" />
          <Knob paramIdx={EParams.kParamDecay} label="DECAY" />
          <Knob paramIdx={EParams.kParamSustain} label="SUSTAIN" step={0.01} />
          <Knob paramIdx={EParams.kParamRelease} label="RELEASE" />
        </div>
      </div>
    </div>
  );
}

function LFOPage() {
  return (
    <div className="flex flex-col gap-4">
      <h2 className="text-white text-sm font-mono uppercase tracking-wider mb-2">LOW FREQUENCY OSCILLATORS</h2>
      <div className="flex flex-col gap-6">
        {/* LFO1 */}
        <div className="flex flex-col items-center gap-2">
          <h3 className="text-white text-xs font-mono uppercase tracking-wider">LFO 1</h3>
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

        {/* LFO2 */}
        <div className="flex flex-col items-center gap-2">
          <h3 className="text-white text-xs font-mono uppercase tracking-wider">LFO 2</h3>
          <div className="flex flex-col items-center gap-2">
            <div className="flex gap-2 items-center">
              <Dropdown
                paramIdx={EParams.kParamLFO2Shape}
                label="SHAPE"
                options={['Triangle', 'Square', 'Ramp Up', 'Ramp Down', 'Sine']}
              />
              <div className="flex items-center mt-4">
                <LFO2SyncCheckbox />
              </div>
            </div>
            <div className="flex gap-2 items-center">
              <LFO2RateControls />
              <Knob paramIdx={EParams.kParamLFO2Depth} label="DEPTH" />
            </div>
          </div>
        </div>

        {/* LFO Waveform Visualization */}
        <div className="mt-2">
          <LFOWaveform />
        </div>
      </div>
    </div>
  );
}

function EffectsPage() {
  return (
    <div className="flex flex-col gap-4">
      <h2 className="text-white text-sm font-mono uppercase tracking-wider mb-2">EFFECTS</h2>
      <div className="flex flex-col gap-6">
        {/* Delay */}
        <div className="flex flex-col items-center gap-2">
          <h3 className="text-white text-xs font-mono uppercase tracking-wider">DELAY</h3>
          <div className="flex gap-2">
            <Knob paramIdx={EParams.kParamDelayTime} label="TIME" />
            <Knob paramIdx={EParams.kParamDelayFeedback} label="FEEDBACK" step={0.01} />
            <Knob paramIdx={EParams.kParamDelayDry} label="DRY" step={0.01} />
            <Knob paramIdx={EParams.kParamDelayWet} label="WET" step={0.01} />
          </div>
        </div>

        {/* Reverb */}
        <div className="flex flex-col items-center gap-2">
          <h3 className="text-white text-xs font-mono uppercase tracking-wider">REVERB</h3>
          <div className="flex gap-2">
            <Knob paramIdx={EParams.kParamReverbRoomSize} label="ROOM" />
            <Knob paramIdx={EParams.kParamReverbDamp} label="DAMP" />
            <Knob paramIdx={EParams.kParamReverbWidth} label="WIDTH" />
            <Knob paramIdx={EParams.kParamReverbDry} label="DRY" step={0.01} />
            <Knob paramIdx={EParams.kParamReverbWet} label="WET" step={0.01} />
          </div>
        </div>
      </div>
    </div>
  );
}

function MainPage() {
  return (
    <div className="flex flex-col gap-4">
      <h2 className="text-white text-sm font-mono uppercase tracking-wider mb-2">MAIN CONTROLS</h2>
      <div className="flex flex-col items-center gap-4">
        <div className="flex gap-2">
          <Knob paramIdx={EParams.kParamGain} label="GAIN" />
          <Knob paramIdx={EParams.kParamNoteGlideTime} label="GLIDE" />
        </div>
        
        <div className="flex flex-col items-center gap-2">
          <h3 className="text-white text-xs font-mono uppercase tracking-wider">OSCILLATOR SYNC</h3>
          <div className="flex flex-col items-center gap-2">
            <Checkbox paramIdx={EParams.kParamOscSync} label="ENABLE" />
            <Knob paramIdx={EParams.kParamOscSyncRatio} label="RATIO" />
          </div>
        </div>
      </div>
    </div>
  );
}

function OscillatorControls({ oscNum }: { oscNum: 1 | 2 | 3 }) {
  const mixParam = oscNum === 1 ? EParams.kParamOsc1Mix : oscNum === 2 ? EParams.kParamOsc2Mix : EParams.kParamOsc3Mix;
  const detuneParam = oscNum === 1 ? EParams.kParamOsc1Detune : oscNum === 2 ? EParams.kParamOsc2Detune : EParams.kParamOsc3Detune;
  const octaveParam = oscNum === 1 ? EParams.kParamOsc1Octave : oscNum === 2 ? EParams.kParamOsc2Octave : EParams.kParamOsc3Octave;
  const waveParam = oscNum === 1 ? EParams.kParamOsc1Wave : oscNum === 2 ? EParams.kParamOsc2Wave : EParams.kParamOsc3Wave;

  return (
    <div className="flex flex-col items-center gap-2">
      <div className="text-white text-sm font-mono uppercase font-bold">OSC {oscNum}</div>
      <Dropdown
        paramIdx={waveParam}
        label="WAVEFORM"
        options={['Sine', 'Saw', 'Square', 'Triangle']}
      />
      <Knob paramIdx={mixParam} label="MIX" step={0.01} />
      <Knob paramIdx={detuneParam} label="DETUNE" />
      <Knob paramIdx={octaveParam} label="OCTAVE" />
    </div>
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

function LFO2SyncCheckbox() {
  return (
    <Checkbox
      paramIdx={EParams.kParamLFO2RateMode}
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

function LFO2RateControls() {
  const { paramValues } = useParameters();
  const syncValue = paramValues.get(EParams.kParamLFO2RateMode) ?? 1.0;
  const syncEnabled = syncValue > 0.5;

  if (syncEnabled) {
    return (
      <Dropdown
        paramIdx={EParams.kParamLFO2RateTempo}
        label="RATE"
        options={['1/64', '1/32', '1/16T', '1/16', '1/16D', '1/8T', '1/8', '1/8D', '1/4', '1/4D', '1/2', '1/1', '2/1', '4/1', '8/1']}
      />
    );
  } else {
    return (
      <Knob paramIdx={EParams.kParamLFO2RateHz} label="RATE HZ" />
    );
  }
}
