import React, { useState } from 'react';
import { ParameterProvider } from './ParameterContext';
import { Knob } from './Knob';
import { Dropdown } from './Dropdown';
import { Meter } from './Meter';
import { PianoKeyboard } from './PianoKeyboard';
import { TabContainer } from './Tabs';
import { EParams } from '../config/constants';
import { initializeEnvironment } from '../utils/environment';
import { initializeWAM, setupMIDIDevices } from '../audio/wam-controller';

export function App() {
  const [activeTab, setActiveTab] = useState(0);
  const [audioStatus, setAudioStatus] = React.useState<'working' | 'not-working' | null>(null);
  
  // Initialize environment detection and auto-start WAM
  React.useEffect(() => {
    const env = initializeEnvironment();
    
    if (env === 'wam') {
      // Auto-start web audio in WAM mode
      initializeWAM().then((wamController) => {
        if (wamController) {
          window.TemplateProject_WAM = wamController;
          setAudioStatus('working');
          // Setup MIDI devices immediately (no delay needed)
          setupMIDIDevices();
        } else {
          setAudioStatus('not-working');
        }
      }).catch((error) => {
        console.error('Error auto-initializing web audio:', error);
        setAudioStatus('not-working');
      });
    }
  }, []);

  const tabs = ['OSC', 'ENV', 'REVERB', 'MAIN'];

  return (
    <ParameterProvider>
      <div className="w-full p-3 mx-auto max-w-6xl">
        {/* Web Controls - Only visible in WAM mode */}
        <div className="wam-only mb-4">
          {/* Audio Status */}
          {audioStatus && (
            <div className="flex items-center justify-center mb-2">
              <p className={`text-xs font-mono uppercase tracking-wider ${
                audioStatus === 'working' ? 'text-green-500' : 'text-red-500'
              }`}>
                {audioStatus === 'working' ? '✓ AUDIO WORKING' : '✗ AUDIO NOT WORKING'}
              </p>
            </div>
          )}

          {/* MIDI Controls */}
          <div className="flex flex-col items-center mb-4">
            <h2 className="text-white text-xs font-mono uppercase tracking-wider mb-2">MIDI</h2>
            <div className="flex gap-3 justify-center">
              <div className="flex flex-col items-center gap-1">
                <label htmlFor="midiInSelect" className="text-white text-xs font-mono uppercase tracking-wider">
                  INPUT
                </label>
                <select 
                  id="midiInSelect" 
                  disabled={true}
                  className="bg-black border-4 border-white text-white px-4 py-2 font-mono text-xs uppercase tracking-wider disabled:opacity-50 disabled:cursor-not-allowed hover:bg-gray-900 focus:outline-none focus:bg-gray-900 cursor-pointer"
                >
                  <option value="default">MIDI INPUT</option>
                </select>
              </div>
              <div className="flex flex-col items-center gap-1">
                <label htmlFor="midiOutSelect" className="text-white text-xs font-mono uppercase tracking-wider">
                  OUTPUT
                </label>
                <select 
                  id="midiOutSelect" 
                  disabled={true}
                  className="bg-black border-4 border-white text-white px-4 py-2 font-mono text-xs uppercase tracking-wider disabled:opacity-50 disabled:cursor-not-allowed hover:bg-gray-900 focus:outline-none focus:bg-gray-900 cursor-pointer"
                >
                  <option value="default">MIDI OUTPUT</option>
                </select>
              </div>
            </div>
          </div>
        </div>

        {/* Horizontal Separator - Only visible in WAM mode */}
        <div className="wam-only border-t-2 border-white mb-4"></div>

        {/* Plugin Content - Wrapped in border */}
        <div className="border-4 border-white p-3 bg-gray-600">
          {/* Plugin Title */}
          <div className="flex items-center justify-center mb-2">
            <h1 className="text-white font-brutal text-xl font-bold uppercase tracking-wider text-center">
              SIMPLE SYNTH
            </h1>
          </div>

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
        <OscillatorControls oscNum={3} />
        <OscillatorControls oscNum={4} />
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

function OscillatorControls({ oscNum }: { oscNum: 1 | 2 | 3 | 4 }) {
  // Map oscillator number to parameter indices
  let mixParam = EParams.kParamOsc1Mix;
  let detuneParam = EParams.kParamOsc1Detune;
  let octaveParam = EParams.kParamOsc1Octave;
  let waveParam = EParams.kParamOsc1Wave;

  switch (oscNum) {
    case 1:
      mixParam = EParams.kParamOsc1Mix;
      detuneParam = EParams.kParamOsc1Detune;
      octaveParam = EParams.kParamOsc1Octave;
      waveParam = EParams.kParamOsc1Wave;
      break;
    case 2:
      mixParam = EParams.kParamOsc2Mix;
      detuneParam = EParams.kParamOsc2Detune;
      octaveParam = EParams.kParamOsc2Octave;
      waveParam = EParams.kParamOsc2Wave;
      break;
    case 3:
      mixParam = EParams.kParamOsc3Mix;
      detuneParam = EParams.kParamOsc3Detune;
      octaveParam = EParams.kParamOsc3Octave;
      waveParam = EParams.kParamOsc3Wave;
      break;
    case 4:
      mixParam = EParams.kParamOsc4Mix;
      detuneParam = EParams.kParamOsc4Detune;
      octaveParam = EParams.kParamOsc4Octave;
      waveParam = EParams.kParamOsc4Wave;
      break;
  }

  return (
    <div className="flex flex-col items-center gap-2 border-4 border-white p-3 bg-black">
      <div className="text-white text-sm font-mono uppercase font-bold">OSC {oscNum}</div>
      <Dropdown
        paramIdx={waveParam}
        label="WAVEFORM"
        options={["Sine", "Saw", "Square", "Triangle"]}
      />
      <Knob paramIdx={mixParam} label="MIX" step={0.01} />
      <Knob paramIdx={detuneParam} label="DETUNE" />
      <Knob paramIdx={octaveParam} label="OCTAVE" />
    </div>
  );
}
