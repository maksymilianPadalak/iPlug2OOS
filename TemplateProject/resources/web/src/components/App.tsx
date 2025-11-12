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
      <div className="w-full bg-black">
        {/* Web Controls - Only visible in WAM mode */}
        <div className="wam-only mb-2 max-w-7xl mx-auto">
          {/* Audio Status */}
          {audioStatus && (
            <div className="flex items-center justify-center mb-2">
              <p className={`text-xs font-bold uppercase tracking-widest ${
                audioStatus === 'working' ? 'text-emerald-400' : 'text-rose-400'
              }`}>
                {audioStatus === 'working' ? '✓ AUDIO WORKING' : '✗ AUDIO NOT WORKING'}
              </p>
            </div>
          )}

          {/* MIDI Controls */}
          <div className="flex flex-col items-center mb-2">
            <h2 className="text-orange-300 text-xs font-bold uppercase tracking-widest mb-2">MIDI</h2>
            <div className="flex gap-4 justify-center">
              <div className="flex flex-col items-center gap-1">
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
              <div className="flex flex-col items-center gap-1">
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
        <div className="wam-only border-t border-orange-900/50 mb-2 max-w-7xl mx-auto"></div>

        {/* Plugin Content - Main Container */}
        <div id="plugin-body" className="w-[1100px] mx-auto bg-gradient-to-br from-neutral-900 via-stone-900 to-neutral-950 border border-orange-800/30 rounded-lg shadow-2xl p-3">
          {/* Plugin Title */}
          <div className="flex items-center justify-between mb-2">
            <h1 className="text-2xl font-black uppercase tracking-tight">
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
          <div className="flex items-center justify-center gap-6 mb-3">
            <div className="flex items-center gap-2">
              <span className="text-orange-300 text-xs font-bold uppercase tracking-wider">L</span>
              <Meter channel={0} compact={true} />
            </div>
            <div className="flex items-center gap-2">
              <span className="text-orange-300 text-xs font-bold uppercase tracking-wider">R</span>
              <Meter channel={1} compact={true} />
            </div>
          </div>

          {/* All Parameters - New layout: waveforms top, knobs bottom, master right */}
          <div className="flex gap-6 mb-3">
            {/* Left side - Waveforms and knobs */}
            <div className="flex-1">
              {/* Waveforms and their knobs in columns */}
              <div className="flex gap-6 justify-center mb-6">
                {/* Each oscillator in its own column */}
                <div className="flex flex-col items-center gap-3">
                  <OscillatorWaveform oscNum={1} />
                  <OscillatorKnobs oscNum={1} />
                </div>
                <div className="flex flex-col items-center gap-3">
                  <OscillatorWaveform oscNum={2} />
                  <OscillatorKnobs oscNum={2} />
                </div>
                <div className="flex flex-col items-center gap-3">
                  <OscillatorWaveform oscNum={3} />
                  <OscillatorKnobs oscNum={3} />
                </div>
                <div className="flex flex-col items-center gap-3">
                  <OscillatorWaveform oscNum={4} />
                  <OscillatorKnobs oscNum={4} />
                </div>
              </div>

              {/* Envelope and Reverb centered below all oscillators */}
              <div className="flex gap-12 justify-center">
                {/* Envelope */}
                <div className="flex flex-col items-center gap-3">
                  <div className="text-orange-300 text-xs font-black uppercase tracking-widest">ENVELOPE</div>
                  <div className="flex gap-3">
                    <Knob paramIdx={EParams.kParamAttack} label="ATTACK" />
                    <Knob paramIdx={EParams.kParamDecay} label="DECAY" />
                    <Knob paramIdx={EParams.kParamSustain} label="SUSTAIN" step={0.01} />
                    <Knob paramIdx={EParams.kParamRelease} label="RELEASE" />
                  </div>
                </div>

                {/* Reverb */}
                <div className="flex flex-col items-center gap-3">
                  <div className="text-orange-300 text-xs font-black uppercase tracking-widest">REVERB</div>
                  <div className="flex gap-3">
                    <Knob paramIdx={EParams.kParamReverbRoomSize} label="ROOM" />
                    <Knob paramIdx={EParams.kParamReverbDamp} label="DAMP" />
                    <Knob paramIdx={EParams.kParamReverbWidth} label="WIDTH" />
                    <Knob paramIdx={EParams.kParamReverbDry} label="DRY" step={0.01} />
                    <Knob paramIdx={EParams.kParamReverbWet} label="WET" step={0.01} />
                  </div>
                </div>
              </div>
            </div>

            {/* Right side - Big main gain */}
            <div className="flex flex-col items-center justify-center pl-6">
              <h2 className="text-orange-300 text-xs font-black uppercase tracking-widest mb-3">MAIN</h2>
              <Knob paramIdx={EParams.kParamGain} label="GAIN" />
            </div>
          </div>

          {/* Keyboard Section - Always visible */}
          <div className="bg-gradient-to-b from-stone-800 to-stone-900 border border-orange-700/30 rounded-lg p-2 shadow-lg">
            <PianoKeyboard />
          </div>
        </div>
      </div>
    </ParameterProvider>
  );
}



// Component for just the waveform visualization with title and dropdown
function OscillatorWaveform({ oscNum }: { oscNum: 1 | 2 | 3 | 4 }) {
  let waveParam = EParams.kParamOsc1Wave;

  switch (oscNum) {
    case 1: waveParam = EParams.kParamOsc1Wave; break;
    case 2: waveParam = EParams.kParamOsc2Wave; break;
    case 3: waveParam = EParams.kParamOsc3Wave; break;
    case 4: waveParam = EParams.kParamOsc4Wave; break;
  }

  const { paramValues } = useParameters();
  const waveValue = paramValues.get(waveParam) ?? 0;
  const waveIndex = Math.round(waveValue * 3);

  return (
    <div className="flex flex-col items-center gap-2 w-[180px]">
      <div className="text-orange-300 text-xs font-black uppercase tracking-wider">OSC {oscNum}</div>
      <Dropdown
        paramIdx={waveParam}
        label="WAVEFORM"
        options={["Sine", "Saw", "Square", "Triangle"]}
      />
      <WaveformDisplay waveIndex={waveIndex} />
    </div>
  );
}

// Component for just the oscillator knobs
function OscillatorKnobs({ oscNum }: { oscNum: 1 | 2 | 3 | 4 }) {
  let mixParam = EParams.kParamOsc1Mix;
  let detuneParam = EParams.kParamOsc1Detune;
  let octaveParam = EParams.kParamOsc1Octave;

  switch (oscNum) {
    case 1:
      mixParam = EParams.kParamOsc1Mix;
      detuneParam = EParams.kParamOsc1Detune;
      octaveParam = EParams.kParamOsc1Octave;
      break;
    case 2:
      mixParam = EParams.kParamOsc2Mix;
      detuneParam = EParams.kParamOsc2Detune;
      octaveParam = EParams.kParamOsc2Octave;
      break;
    case 3:
      mixParam = EParams.kParamOsc3Mix;
      detuneParam = EParams.kParamOsc3Detune;
      octaveParam = EParams.kParamOsc3Octave;
      break;
    case 4:
      mixParam = EParams.kParamOsc4Mix;
      detuneParam = EParams.kParamOsc4Detune;
      octaveParam = EParams.kParamOsc4Octave;
      break;
  }

  return (
    <div className="flex gap-2 justify-center w-[180px]">
      <Knob paramIdx={mixParam} label="MIX" step={0.01} />
      <Knob paramIdx={detuneParam} label="DETUNE" />
      <Knob paramIdx={octaveParam} label="OCTAVE" />
    </div>
  );
}

function WaveformDisplay({ waveIndex }: { waveIndex: number }) {
  const [offset, setOffset] = React.useState(0);
  const width = 180;
  const height = 40;
  const padding = 8;
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
    <div className="w-full bg-black/40 border border-orange-700/30 rounded p-1 overflow-hidden">
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
