/**
 * Main App component - Simple 2 Oscillator Synth with Reverb (Tabbed Interface)
 */

import React, { useState } from 'react';
import { ParameterProvider } from './ParameterContext';
import { WAMControls } from './WAMControls';
import { Knob } from './Knob';
import { Dropdown } from './Dropdown';
import { Meter } from './Meter';
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

  const tabs = ['OSC', 'ENV', 'REVERB', 'MAIN'];

  return (
    <ParameterProvider>
      <div className="w-full h-full p-3 mx-auto max-w-6xl">
        {/* Header */}
        <div className="flex items-center justify-center mb-2">
          <h1 className="text-white font-brutal text-xl font-bold uppercase tracking-wider text-center">
            SIMPLE SYNTH
          </h1>
        </div>
        <div className="flex items-center justify-center mb-2">
          <p id="adapterStatus" className="wam-only text-yellow-400 text-xs font-mono uppercase tracking-wider text-center">
            Waiting for AudioWorklet initialization...
          </p>
        </div>

        <WAMControls />

        {/* Output Meters */}
        <div className="flex items-center justify-center gap-4 mb-2">
          <Meter channel={0} compact={true} />
          <Meter channel={1} compact={true} />
        </div>

        {/* Tabbed Interface */}
        <TabContainer tabs={tabs} activeTab={activeTab} onTabChange={setActiveTab}>
          {activeTab === 0 && <OscillatorsPage />}
          {activeTab === 1 && <EnvelopePage />}
          {activeTab === 2 && <ReverbPage />}
          {activeTab === 3 && <MainPage />}
        </TabContainer>

        {/* Keyboard Section - Always visible */}
        <div className="bg-black border-4 border-white p-3 mt-2">
          <PianoKeyboard />
        </div>
      </div>
    </ParameterProvider>
  );
}

function OscillatorsPage() {
  return (
    <div className="flex flex-col gap-4">
      <h2 className="text-white text-sm font-mono uppercase tracking-wider mb-2 text-center">OSCILLATORS</h2>
      <div className="flex gap-4 justify-center flex-wrap">
        <OscillatorControls oscNum={1} />
        <OscillatorControls oscNum={2} />
      </div>
    </div>
  );
}

function EnvelopePage() {
  return (
    <div className="flex flex-col gap-4">
      <h2 className="text-white text-sm font-mono uppercase tracking-wider mb-2 text-center">AMPLITUDE ENVELOPE</h2>
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

function ReverbPage() {
  return (
    <div className="flex flex-col gap-4">
      <h2 className="text-white text-sm font-mono uppercase tracking-wider mb-2 text-center">REVERB</h2>
      <div className="flex flex-col items-center gap-3">
        <div className="flex gap-2 justify-center flex-wrap">
          <Knob paramIdx={EParams.kParamReverbRoomSize} label="ROOM" />
          <Knob paramIdx={EParams.kParamReverbDamp} label="DAMP" />
          <Knob paramIdx={EParams.kParamReverbWidth} label="WIDTH" />
        </div>
        <div className="flex gap-2 justify-center">
          <Knob paramIdx={EParams.kParamReverbDry} label="DRY" step={0.01} />
          <Knob paramIdx={EParams.kParamReverbWet} label="WET" step={0.01} />
        </div>
      </div>
    </div>
  );
}

function MainPage() {
  return (
    <div className="flex flex-col gap-4">
      <h2 className="text-white text-sm font-mono uppercase tracking-wider mb-2 text-center">MAIN CONTROLS</h2>
      <div className="flex flex-col items-center gap-4">
        <div className="flex gap-2">
          <Knob paramIdx={EParams.kParamGain} label="GAIN" />
        </div>
      </div>
    </div>
  );
}

function OscillatorControls({ oscNum }: { oscNum: 1 | 2 }) {
  const mixParam = oscNum === 1 ? EParams.kParamOsc1Mix : EParams.kParamOsc2Mix;
  const detuneParam = oscNum === 1 ? EParams.kParamOsc1Detune : EParams.kParamOsc2Detune;
  const octaveParam = oscNum === 1 ? EParams.kParamOsc1Octave : EParams.kParamOsc2Octave;
  const waveParam = oscNum === 1 ? EParams.kParamOsc1Wave : EParams.kParamOsc2Wave;

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
