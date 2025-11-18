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
          window.TemplateProject2_WAM = wamController;
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
            <h2 className="text-cyan-300 text-xs font-bold uppercase tracking-widest mb-2">MIDI</h2>
            <div className="flex gap-4 justify-center">
              <div className="flex flex-col items-center gap-1">
                <label htmlFor="midiInSelect" className="text-cyan-200 text-xs font-bold uppercase tracking-wider">
                  INPUT
                </label>
                <select
                  id="midiInSelect"
                  disabled={true}
                  className="bg-gradient-to-b from-blue-900 to-blue-950 border border-cyan-600/50 text-cyan-100 px-4 py-2 rounded font-bold text-xs uppercase tracking-wider disabled:opacity-50 disabled:cursor-not-allowed hover:from-blue-800 hover:to-blue-900 focus:outline-none focus:ring-2 focus:ring-cyan-500/50 cursor-pointer transition-all"
                >
                  <option value="default">MIDI INPUT</option>
                </select>
              </div>
              <div className="flex flex-col items-center gap-1">
                <label htmlFor="midiOutSelect" className="text-cyan-200 text-xs font-bold uppercase tracking-wider">
                  OUTPUT
                </label>
                <select
                  id="midiOutSelect"
                  disabled={true}
                  className="bg-gradient-to-b from-blue-900 to-blue-950 border border-cyan-600/50 text-cyan-100 px-4 py-2 rounded font-bold text-xs uppercase tracking-wider disabled:opacity-50 disabled:cursor-not-allowed hover:from-blue-800 hover:to-blue-900 focus:outline-none focus:ring-2 focus:ring-cyan-500/50 cursor-pointer transition-all"
                >
                  <option value="default">MIDI OUTPUT</option>
                </select>
              </div>
            </div>
          </div>
        </div>

        {/* Horizontal Separator - Only visible in WAM mode */}
        <div className="wam-only border-t border-blue-900/50 mb-2 max-w-7xl mx-auto"></div>

        {/* Plugin Content - Main Container */}
        <div id="plugin-body" className="w-[1100px] mx-auto bg-gradient-to-br from-neutral-900 via-stone-900 to-neutral-950 border border-cyan-800/30 rounded-lg shadow-2xl p-3">
          {/* Plugin Title */}
          <div className="flex items-center justify-between mb-2">
            <h1 className="text-2xl font-black uppercase tracking-tight">
              <span className="bg-gradient-to-r from-cyan-400 via-blue-500 to-indigo-500 text-transparent bg-clip-text">
                Simple
              </span>
              <span className="bg-gradient-to-r from-indigo-500 via-blue-500 to-cyan-500 text-transparent bg-clip-text">
                Synth
              </span>
            </h1>
            <div className="flex items-center gap-3">
              <span className="text-cyan-400/60 text-xs font-bold uppercase tracking-wider">Version 1.0</span>
            </div>
          </div>

          {/* Output Meters */}
          <div className="flex items-center justify-center gap-6 mb-3">
            <div className="flex items-center gap-2">
              <span className="text-cyan-300 text-xs font-bold uppercase tracking-wider">L</span>
              <Meter channel={0} compact={true} />
            </div>
            <div className="flex items-center gap-2">
              <span className="text-cyan-300 text-xs font-bold uppercase tracking-wider">R</span>
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
                  <div className="text-cyan-300 text-xs font-black uppercase tracking-widest">ENVELOPE</div>
                  <div className="flex gap-3">
                    <Knob paramIdx={EParams.kParamAttack} label="ATTACK" />
                    <Knob paramIdx={EParams.kParamDecay} label="DECAY" />
                    <Knob paramIdx={EParams.kParamSustain} label="SUSTAIN" step={0.01} />
                    <Knob paramIdx={EParams.kParamRelease} label="RELEASE" />
                  </div>
                </div>

                {/* Reverb */}
                <div className="flex flex-col items-center gap-3">
                  <div className="text-cyan-300 text-xs font-black uppercase tracking-widest">REVERB</div>
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
            <div className="flex flex-col items-center justify-center pl-2">
              <h2 className="text-cyan-300 text-xs font-black uppercase tracking-widest mb-3">MAIN</h2>
              <Knob paramIdx={EParams.kParamGain} label="GAIN" />
            </div>
          </div>

          {/* Keyboard Section - Always visible */}
          <div className="bg-gradient-to-b from-stone-800 to-stone-900 border border-cyan-700/30 rounded-lg p-2 shadow-lg">
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

  // Map oscillator to its knob primary color so the waveform matches the knob
  const primaryColor = React.useMemo(() => {
    switch (oscNum) {
      case 1: return '#06b6d4'; // Cyan/teal for oscillator 1 (changed from orange)
      case 2: return '#34d399'; // green
      case 3: return '#a78bfa'; // purple
      case 4: return '#f472b6'; // pink
      default: return '#06b6d4'; // cyan
    }
  }, [oscNum]);

  return (
    <div className="flex flex-col items-center gap-2 w-[180px]">
      <div className="text-cyan-300 text-xs font-black uppercase tracking-wider">OSC {oscNum}</div>
      <Dropdown
        paramIdx={waveParam}
        label="WAVEFORM"
        options={["Sine", "Saw", "Square", "Triangle"]}
      />
      <WaveformDisplay waveIndex={waveIndex} color={primaryColor} />
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

function WaveformDisplay({ waveIndex, color }: { waveIndex: number; color: string }) {
  const [offset, setOffset] = React.useState(0);
  const width = 180;
  const height = 40;
  const padding = 8;
  const centerY = height / 2;

  // unique filter id to avoid collisions
  const filterIdRef = React.useRef(`glow-${Math.random().toString(36).slice(2, 9)}`);
  const filterId = filterIdRef.current;

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

  const pathD = React.useMemo(() => generatePath(offset), [offset, waveIndex]);

  return (
    <div className="w-full bg-black/40 border border-cyan-700/30 rounded p-1 overflow-hidden">
      <svg width={width} height={height} viewBox={`0 0 ${width} ${height}`} className="w-full">
        {/* Grid lines */}
        <line x1={padding} y1={centerY} x2={width - padding} y2={centerY} stroke="#ffffff10" strokeWidth={1} />

        {/* Waveform with glow effect */}
        <defs>
          <filter id={filterId} x="-50%" y="-50%" width="200%" height="200%">
            <feGaussianBlur stdDeviation="3" result="coloredBlur" />
            <feMerge>
              <feMergeNode in="coloredBlur" />
              <feMergeNode in="SourceGraphic" />
            </feMerge>
          </filter>
        </defs>

        {/* blurred thicker stroke for glow */}
        <path
          d={pathD}
          fill="none"
          stroke={color}
          strokeWidth={4.5}
          strokeLinecap="round"
          strokeLinejoin="round"
          opacity={0.35}
          filter={`url(#${filterId})`}
        />

        <path
          d={pathD}
          fill="none"
          stroke={color}
          strokeWidth={2.5}
          strokeLinecap="round"
          strokeLinejoin="round"
        />
      </svg>
    </div>
  );
}
