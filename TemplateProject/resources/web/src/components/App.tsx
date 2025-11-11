import React from 'react';
import { ParameterProvider, useParameters } from './ParameterContext';
import { Knob } from './Knob';
import { Dropdown } from './Dropdown';
import { Meter } from './Meter';
import { PianoKeyboard } from './PianoKeyboard';
import { EParams } from '../config/constants';
import { initializeEnvironment } from '../utils/environment';
import { initializeWAM, setupMIDIDevices } from '../audio/wam-controller';

export function App() {
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

  return (
    <ParameterProvider>
      <div className="w-full min-h-screen bg-black p-6">
        {/* Web Controls - Only visible in WAM mode */}
        <div className="wam-only mb-6 max-w-7xl mx-auto">
          {/* Audio Status */}
          {audioStatus && (
            <div className="flex items-center justify-center mb-3">
              <p className={`text-xs font-bold uppercase tracking-widest ${
                audioStatus === 'working' ? 'text-emerald-400' : 'text-rose-400'
              }`}>
                {audioStatus === 'working' ? '✓ AUDIO WORKING' : '✗ AUDIO NOT WORKING'}
              </p>
            </div>
          )}

          {/* MIDI Controls */}
          <div className="flex flex-col items-center mb-4">
            <h2 className="text-orange-300 text-xs font-bold uppercase tracking-widest mb-3">MIDI</h2>
            <div className="flex gap-4 justify-center">
              <div className="flex flex-col items-center gap-2">
                <label htmlFor="midiInSelect" className="text-orange-200 text-xs font-bold uppercase tracking-wider">
                  INPUT
                </label>
                <select
                  id="midiInSelect"
                  disabled={true}
                  className="bg-gradient-to-b from-orange-900 to-orange-950 border border-orange-600/50 text-orange-100 px-4 py-2 rounded font-bold text-xs uppercase tracking-wider disabled:opacity-50 disabled:cursor-not-allowed hover:from-orange-800 hover:to-orange-900 focus:outline-none focus:ring-2 focus:ring-orange-500/50 cursor-pointer transition-all"
                >
                  <option value="default">MIDI INPUT</option>
                </select>
              </div>
              <div className="flex flex-col items-center gap-2">
                <label htmlFor="midiOutSelect" className="text-orange-200 text-xs font-bold uppercase tracking-wider">
                  OUTPUT
                </label>
                <select
                  id="midiOutSelect"
                  disabled={true}
                  className="bg-gradient-to-b from-orange-900 to-orange-950 border border-orange-600/50 text-orange-100 px-4 py-2 rounded font-bold text-xs uppercase tracking-wider disabled:opacity-50 disabled:cursor-not-allowed hover:from-orange-800 hover:to-orange-900 focus:outline-none focus:ring-2 focus:ring-orange-500/50 cursor-pointer transition-all"
                >
                  <option value="default">MIDI OUTPUT</option>
                </select>
              </div>
            </div>
          </div>
        </div>

        {/* Horizontal Separator - Only visible in WAM mode */}
        <div className="wam-only border-t border-orange-900/50 mb-6 max-w-7xl mx-auto"></div>

        {/* Plugin Content - Main Container */}
        <div id="plugin-body" className="max-w-7xl mx-auto bg-gradient-to-br from-neutral-900 via-stone-900 to-neutral-950 border border-orange-800/30 rounded-lg shadow-2xl p-8">
          {/* Plugin Title */}
          <div className="flex items-center justify-between mb-6">
            <h1 className="text-5xl font-black uppercase tracking-tight">
              <span className="bg-gradient-to-r from-orange-400 via-orange-500 to-red-500 text-transparent bg-clip-text">
                Simple
              </span>
              <span className="bg-gradient-to-r from-red-500 via-rose-500 to-orange-500 text-transparent bg-clip-text">
                Synth
              </span>
            </h1>
            <div className="flex items-center gap-3">
              <span className="text-orange-400/60 text-xs font-bold uppercase tracking-wider">Version 1.0</span>
            </div>
          </div>

          {/* Output Meters */}
          <div className="flex items-center justify-center gap-6 mb-8">
            <div className="flex items-center gap-2">
              <span className="text-orange-300 text-xs font-bold uppercase tracking-wider">L</span>
              <Meter channel={0} compact={true} />
            </div>
            <div className="flex items-center gap-2">
              <span className="text-orange-300 text-xs font-bold uppercase tracking-wider">R</span>
              <Meter channel={1} compact={true} />
            </div>
          </div>

          {/* All Parameters - Modern grid layout */}
          <div className="grid grid-cols-1 lg:grid-cols-3 gap-5 mb-6">
            {/* Left Column - Oscillators (spans 2 columns) */}
            <div className="lg:col-span-2">
              <OscillatorsPage />
            </div>

            {/* Right Column - Envelope, Reverb, and Main Output stacked */}
            <div className="flex flex-col gap-5">
              <EnvelopePage />
              <ReverbPage />
              <MainPage />
            </div>
          </div>

          {/* Keyboard Section - Always visible */}
          <div className="bg-gradient-to-b from-stone-800 to-stone-900 border border-orange-700/30 rounded-lg p-4 shadow-lg">
            <PianoKeyboard />
          </div>
        </div>
      </div>
    </ParameterProvider>
  );
}

function OscillatorsPage() {
  return (
    <div className="bg-gradient-to-br from-orange-950/40 to-red-950/30 border border-orange-700/40 rounded-lg p-5 shadow-lg backdrop-blur-sm">
      <h2 className="text-orange-300 text-sm font-black uppercase tracking-widest mb-5 text-center">OSCILLATORS</h2>
      <div className="grid grid-cols-2 gap-4">
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
    <div className="bg-gradient-to-br from-orange-950/40 to-amber-950/30 border border-orange-700/40 rounded-lg p-5 shadow-lg backdrop-blur-sm">
      <h2 className="text-orange-300 text-sm font-black uppercase tracking-widest mb-5 text-center">ENVELOPE</h2>
      <div className="grid grid-cols-2 gap-3">
        <Knob paramIdx={EParams.kParamAttack} label="ATTACK" />
        <Knob paramIdx={EParams.kParamDecay} label="DECAY" />
        <Knob paramIdx={EParams.kParamSustain} label="SUSTAIN" step={0.01} />
        <Knob paramIdx={EParams.kParamRelease} label="RELEASE" />
      </div>
    </div>
  );
}

function ReverbPage() {
  return (
    <div className="bg-gradient-to-br from-red-950/40 to-orange-950/30 border border-orange-700/40 rounded-lg p-5 shadow-lg backdrop-blur-sm">
      <h2 className="text-orange-300 text-sm font-black uppercase tracking-widest mb-5 text-center">REVERB</h2>
      <div className="grid grid-cols-2 gap-3">
        <Knob paramIdx={EParams.kParamReverbRoomSize} label="ROOM" />
        <Knob paramIdx={EParams.kParamReverbDamp} label="DAMP" />
        <Knob paramIdx={EParams.kParamReverbWidth} label="WIDTH" />
        <Knob paramIdx={EParams.kParamReverbDry} label="DRY" step={0.01} />
        <Knob paramIdx={EParams.kParamReverbWet} label="WET" step={0.01} />
      </div>
    </div>
  );
}

function MainPage() {
  return (
    <div className="bg-gradient-to-br from-orange-950/40 to-stone-950/30 border border-orange-700/40 rounded-lg p-5 shadow-lg backdrop-blur-sm">
      <h2 className="text-orange-300 text-sm font-black uppercase tracking-widest mb-5 text-center">MAIN OUTPUT</h2>
      <div className="flex justify-center">
        <Knob paramIdx={EParams.kParamGain} label="GAIN" />
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

  const { paramValues } = useParameters();
  const waveValue = paramValues.get(waveParam) ?? 0;
  const waveIndex = Math.round(waveValue * 3); // 0=Sine, 1=Saw, 2=Square, 3=Triangle

  return (
    <div className="flex flex-col items-center gap-4 bg-gradient-to-b from-stone-900/50 to-neutral-900/50 border border-orange-600/20 rounded-lg p-4">
      <div className="text-orange-400 text-sm font-black uppercase tracking-wider">OSC {oscNum}</div>

      {/* Dropdown on top */}
      <Dropdown
        paramIdx={waveParam}
        label="WAVEFORM"
        options={["Sine", "Saw", "Square", "Triangle"]}
      />

      {/* Knobs arranged horizontally */}
      <div className="flex gap-4 justify-center">
        <Knob paramIdx={mixParam} label="MIX" step={0.01} />
        <Knob paramIdx={detuneParam} label="DETUNE" />
        <Knob paramIdx={octaveParam} label="OCTAVE" />
      </div>

      {/* Waveform visualization */}
      <WaveformDisplay waveIndex={waveIndex} />
    </div>
  );
}

function WaveformDisplay({ waveIndex }: { waveIndex: number }) {
  const [offset, setOffset] = React.useState(0);
  const width = 200;
  const height = 60;
  const padding = 10;
  const centerY = height / 2;

  // Animate the waveform scrolling
  React.useEffect(() => {
    let animationFrame: number;
    let lastTime = Date.now();

    const animate = () => {
      const now = Date.now();
      const delta = (now - lastTime) / 1000; // Convert to seconds
      lastTime = now;

      setOffset((prev) => (prev + delta * 0.5) % 1); // Scroll speed
      animationFrame = requestAnimationFrame(animate);
    };

    animationFrame = requestAnimationFrame(animate);
    return () => cancelAnimationFrame(animationFrame);
  }, []);

  // Generate path data for different waveforms
  const generatePath = (phaseOffset: number) => {
    const points: string[] = [];
    const cycles = 2;
    const resolution = 100;

    for (let i = 0; i <= resolution; i++) {
      const x = padding + (i / resolution) * (width - padding * 2);
      const t = ((i / resolution) + phaseOffset) * cycles * Math.PI * 2;
      let y = 0;

      switch (waveIndex) {
        case 0: // Sine
          y = Math.sin(t);
          break;
        case 1: // Saw
          y = 2 * ((t / (Math.PI * 2)) % 1) - 1;
          break;
        case 2: // Square
          y = Math.sin(t) >= 0 ? 1 : -1;
          break;
        case 3: // Triangle
          const normalized = (t / (Math.PI * 2)) % 1;
          y = normalized < 0.5 ? 4 * normalized - 1 : -4 * normalized + 3;
          break;
      }

      const scaledY = centerY - y * (height / 2 - padding);
      points.push(i === 0 ? `M ${x} ${scaledY}` : `L ${x} ${scaledY}`);
    }

    return points.join(' ');
  };

  return (
    <div className="w-full bg-black/40 border border-orange-700/30 rounded p-2 overflow-hidden">
      <svg width={width} height={height} viewBox={`0 0 ${width} ${height}`} className="w-full">
        {/* Grid lines */}
        <line x1={padding} y1={centerY} x2={width - padding} y2={centerY} stroke="#ffffff10" strokeWidth={1} />

        {/* Waveform with glow effect */}
        <defs>
          <filter id="glow">
            <feGaussianBlur stdDeviation="2" result="coloredBlur"/>
            <feMerge>
              <feMergeNode in="coloredBlur"/>
              <feMergeNode in="SourceGraphic"/>
            </feMerge>
          </filter>
        </defs>

        <path
          d={generatePath(offset)}
          fill="none"
          stroke="#fb923c"
          strokeWidth={2.5}
          strokeLinecap="round"
          strokeLinejoin="round"
          filter="url(#glow)"
        />
      </svg>
    </div>
  );
}
