/**
 * Step Sequencer Component
 *
 * 16-step drum sequencer with transport controls.
 * Uses Web Worker for timing to avoid main thread jank.
 * Futuristic holographic LED pad design.
 */

import { useState, useCallback, useEffect, useRef, useMemo } from 'react';

const NUM_STEPS = 16;
const DEFAULT_BPM = 120;

type DrumColor = 'orange' | 'cyan' | 'magenta' | 'green';

// Subtle color definitions - matching the dark plugin aesthetic
const COLORS: Record<DrumColor, {
  rgb: string;
  active: string;
  inactive: string;
  border: string;
  activeBorder: string;
  glow: string;
  text: string;
}> = {
  orange: {
    rgb: '251, 146, 60',
    active: 'rgba(251, 146, 60, 0.5)',
    inactive: 'rgba(251, 146, 60, 0.08)',
    border: 'rgba(251, 146, 60, 0.2)',
    activeBorder: 'rgba(251, 146, 60, 0.7)',
    glow: '0 0 12px rgba(251,146,60,0.5)',
    text: 'rgb(251, 146, 60)',
  },
  cyan: {
    rgb: '34, 211, 238',
    active: 'rgba(34, 211, 238, 0.5)',
    inactive: 'rgba(34, 211, 238, 0.08)',
    border: 'rgba(34, 211, 238, 0.2)',
    activeBorder: 'rgba(34, 211, 238, 0.7)',
    glow: '0 0 12px rgba(34,211,238,0.5)',
    text: 'rgb(34, 211, 238)',
  },
  magenta: {
    rgb: '236, 72, 153',
    active: 'rgba(236, 72, 153, 0.5)',
    inactive: 'rgba(236, 72, 153, 0.08)',
    border: 'rgba(236, 72, 153, 0.2)',
    activeBorder: 'rgba(236, 72, 153, 0.7)',
    glow: '0 0 12px rgba(236,72,153,0.5)',
    text: 'rgb(236, 72, 153)',
  },
  green: {
    rgb: '74, 222, 128',
    active: 'rgba(74, 222, 128, 0.5)',
    inactive: 'rgba(74, 222, 128, 0.08)',
    border: 'rgba(74, 222, 128, 0.2)',
    activeBorder: 'rgba(74, 222, 128, 0.7)',
    glow: '0 0 12px rgba(74,222,128,0.5)',
    text: 'rgb(74, 222, 128)',
  },
};

// Must match VoiceCard colors in PluginBody: Kick=orange, Snare=magenta, HiHat=green, Clap=orange, Tom=cyan, Rim=magenta
const VOICE_COLORS: DrumColor[] = ['orange', 'magenta', 'green', 'orange', 'cyan', 'magenta'];

// Simple CSS - minimal animations matching plugin style
const SEQUENCER_STYLES = `
  @keyframes seq-trigger {
    0% { filter: brightness(1.8); }
    100% { filter: brightness(1); }
  }

  .seq-step {
    cursor: pointer;
    transition: border-color 0.1s, background 0.1s;
  }

  .seq-step:hover {
    filter: brightness(1.3);
  }

  .seq-step-triggered {
    animation: seq-trigger 0.12s ease-out;
  }

  /* Hide default number input spinners */
  .seq-bpm-input::-webkit-outer-spin-button,
  .seq-bpm-input::-webkit-inner-spin-button {
    -webkit-appearance: none;
    margin: 0;
  }
  .seq-bpm-input {
    -moz-appearance: textfield;
  }
`;

// Inline worker code - runs on separate thread
const workerCode = `
const NUM_STEPS = 16;
const SCHEDULER_INTERVAL = 10;

let isPlaying = false;
let currentStep = 0;
let bpm = 120;
let pattern = [];
let drumNotes = [];
let nextStepTime = 0;
let intervalId = null;

function getStepDurationMs() {
  return (60 / bpm / 4) * 1000;
}

function scheduler() {
  if (!isPlaying) return;

  const now = performance.now();

  if (now >= nextStepTime) {
    const notesToTrigger = [];

    pattern.forEach((track, trackIndex) => {
      if (track[currentStep] && drumNotes[trackIndex] !== undefined) {
        notesToTrigger.push(drumNotes[trackIndex]);
      }
    });

    self.postMessage({
      type: 'tick',
      step: currentStep,
      notes: notesToTrigger,
    });

    currentStep = (currentStep + 1) % NUM_STEPS;
    nextStepTime += getStepDurationMs();

    if (nextStepTime < now) {
      nextStepTime = now + getStepDurationMs();
    }
  }
}

function start(initialBpm, initialPattern, notes) {
  bpm = initialBpm;
  pattern = initialPattern;
  drumNotes = notes;
  currentStep = 0;
  nextStepTime = performance.now();
  isPlaying = true;

  if (intervalId !== null) {
    clearInterval(intervalId);
  }

  intervalId = setInterval(scheduler, SCHEDULER_INTERVAL);
  scheduler();
}

function stop() {
  isPlaying = false;
  currentStep = 0;

  if (intervalId !== null) {
    clearInterval(intervalId);
    intervalId = null;
  }

  self.postMessage({ type: 'stopped' });
}

self.onmessage = (event) => {
  const message = event.data;

  switch (message.type) {
    case 'start':
      start(message.bpm, message.pattern, message.notes);
      break;
    case 'stop':
      stop();
      break;
    case 'updatePattern':
      pattern = message.pattern;
      break;
    case 'updateBpm':
      bpm = message.bpm;
      break;
    case 'updateNotes':
      drumNotes = message.notes;
      break;
  }
};
`;

function createWorker(): { worker: Worker; blobUrl: string } {
  const blob = new Blob([workerCode], { type: 'application/javascript' });
  const blobUrl = URL.createObjectURL(blob);
  return { worker: new Worker(blobUrl), blobUrl };
}

export type VoiceConfig = {
  name: string;
  midiNote: number;
};

export type StepSequencerProps = {
  voiceConfig: VoiceConfig[];
  onNoteOn: (noteNum: number, velocity: number) => void;
  onNoteOff: (noteNum: number, velocity: number) => void;
};

export function StepSequencer({ voiceConfig, onNoteOn, onNoteOff }: StepSequencerProps) {
  const tracks = useMemo(() =>
    voiceConfig.map((v, i) => ({
      name: v.name,
      note: v.midiNote,
      color: VOICE_COLORS[i % VOICE_COLORS.length],
    })),
    [voiceConfig]
  );

  const drumNotes = useMemo(() => voiceConfig.map(v => v.midiNote), [voiceConfig]);

  const [pattern, setPattern] = useState<boolean[][]>(() =>
    tracks.map(() => Array(NUM_STEPS).fill(false))
  );
  const [isPlaying, setIsPlaying] = useState(false);
  const [currentStep, setCurrentStep] = useState(0);
  const [bpm, setBpm] = useState(DEFAULT_BPM);

  const workerRef = useRef<Worker | null>(null);
  const blobUrlRef = useRef<string | null>(null);
  const patternRef = useRef(pattern);
  const bpmRef = useRef(bpm);
  const drumNotesRef = useRef(drumNotes);
  const onNoteOnRef = useRef(onNoteOn);
  const onNoteOffRef = useRef(onNoteOff);

  // Reset pattern when voiceConfig changes
  useEffect(() => {
    setPattern(tracks.map(() => Array(NUM_STEPS).fill(false)));
    drumNotesRef.current = drumNotes;
    workerRef.current?.postMessage({ type: 'updateNotes', notes: drumNotes });
  }, [tracks, drumNotes]);

  useEffect(() => {
    patternRef.current = pattern;
    workerRef.current?.postMessage({ type: 'updatePattern', pattern });
  }, [pattern]);

  useEffect(() => {
    bpmRef.current = bpm;
    workerRef.current?.postMessage({ type: 'updateBpm', bpm });
  }, [bpm]);

  useEffect(() => {
    onNoteOnRef.current = onNoteOn;
  }, [onNoteOn]);

  useEffect(() => {
    onNoteOffRef.current = onNoteOff;
  }, [onNoteOff]);

  // Initialize worker
  useEffect(() => {
    const { worker, blobUrl } = createWorker();
    workerRef.current = worker;
    blobUrlRef.current = blobUrl;

    worker.onmessage = (event) => {
      const message = event.data;

      if (message.type === 'tick') {
        setCurrentStep(message.step);

        message.notes.forEach((note: number) => {
          onNoteOnRef.current(note, 127);
          setTimeout(() => {
            onNoteOffRef.current(note, 0);
          }, 30);
        });
      } else if (message.type === 'stopped') {
        setCurrentStep(0);
      }
    };

    return () => {
      worker.terminate();
      if (blobUrlRef.current) {
        URL.revokeObjectURL(blobUrlRef.current);
      }
    };
  }, []);

  // Inject CSS
  useEffect(() => {
    const styleId = 'seq-styles-v2';
    if (!document.getElementById(styleId)) {
      const style = document.createElement('style');
      style.id = styleId;
      style.textContent = SEQUENCER_STYLES;
      document.head.appendChild(style);
    }
  }, []);

  const toggleStep = useCallback((trackIndex: number, stepIndex: number) => {
    setPattern(prev => {
      const newPattern = prev.map(row => [...row]);
      newPattern[trackIndex][stepIndex] = !newPattern[trackIndex][stepIndex];
      return newPattern;
    });
  }, []);

  const startPlayback = useCallback(() => {
    setIsPlaying(true);
    workerRef.current?.postMessage({
      type: 'start',
      bpm: bpmRef.current,
      pattern: patternRef.current,
      notes: drumNotesRef.current,
    });
  }, []);

  const stopPlayback = useCallback(() => {
    setIsPlaying(false);
    workerRef.current?.postMessage({ type: 'stop' });
  }, []);

  const togglePlay = useCallback(() => {
    if (isPlaying) {
      stopPlayback();
    } else {
      startPlayback();
    }
  }, [isPlaying, startPlayback, stopPlayback]);

  const clearPattern = useCallback(() => {
    setPattern(tracks.map(() => Array(NUM_STEPS).fill(false)));
  }, [tracks]);

  if (tracks.length === 0) {
    return null;
  }

  return (
    <div
      className="relative rounded-lg overflow-hidden"
      style={{
        backgroundColor: '#080508',
        backgroundImage: `
          linear-gradient(rgba(255,0,128,0.03) 1px, transparent 1px),
          linear-gradient(90deg, rgba(255,0,128,0.03) 1px, transparent 1px)
        `,
        backgroundSize: '24px 24px',
        boxShadow: 'inset 0 1px 2px rgba(255,0,128,0.05), inset 0 -1px 2px rgba(0,0,0,0.5)',
      }}
    >
      {/* Corner accents - matching plugin style */}
      <div className="absolute top-1.5 left-1.5 w-4 h-4 border-l border-t border-pink-500/40" />
      <div className="absolute top-1.5 right-1.5 w-4 h-4 border-r border-t border-pink-500/40" />
      <div className="absolute bottom-1.5 left-1.5 w-4 h-4 border-l border-b border-pink-500/40" />
      <div className="absolute bottom-1.5 right-1.5 w-4 h-4 border-r border-b border-pink-500/40" />

      {/* Header */}
      <div className="flex justify-between items-center pl-4 pr-8 py-3">
        <div className="flex items-center gap-2">
          <div className="flex gap-0.5">
            <div className="w-0.5 h-4 bg-pink-500 rounded-full" />
            <div className="w-0.5 h-3 bg-pink-500/60 rounded-full mt-0.5" />
          </div>
          <span className="text-xs font-bold uppercase tracking-[0.15em] text-pink-500">
            Step Sequencer
          </span>
        </div>

        {/* Transport */}
        <div className="flex items-center gap-3">
          <span className="text-[11px] text-pink-500/50 uppercase tracking-wider">BPM</span>
          <div className="flex items-stretch h-8">
            <input
              type="number"
              value={bpm}
              onChange={(e) => setBpm(Math.max(40, Math.min(300, parseInt(e.target.value) || DEFAULT_BPM)))}
              className="seq-bpm-input w-14 bg-black/50 border border-pink-500/30 rounded-l px-2 text-sm text-pink-200 font-mono text-center focus:outline-none focus:border-pink-500/60"
              min={40}
              max={300}
            />
            <div className="flex flex-col border border-l-0 border-pink-500/30 rounded-r overflow-hidden">
              <button
                onClick={() => setBpm(prev => Math.min(300, prev + 1))}
                className="flex-1 px-1.5 bg-black/50 text-pink-500/60 hover:bg-pink-500/20 hover:text-pink-400 transition-colors flex items-center justify-center"
              >
                <svg className="w-2.5 h-2.5" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                  <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2.5} d="M5 15l7-7 7 7" />
                </svg>
              </button>
              <button
                onClick={() => setBpm(prev => Math.max(40, prev - 1))}
                className="flex-1 px-1.5 bg-black/50 text-pink-500/60 hover:bg-pink-500/20 hover:text-pink-400 transition-colors border-t border-pink-500/30 flex items-center justify-center"
              >
                <svg className="w-2.5 h-2.5" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                  <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2.5} d="M19 9l-7 7-7-7" />
                </svg>
              </button>
            </div>
          </div>

          <button
            onClick={clearPattern}
            className="px-4 py-1.5 border border-pink-500/30 rounded text-pink-400/70 text-[11px] font-bold uppercase tracking-wider hover:border-pink-500/50 hover:text-pink-300 transition-colors"
          >
            Clear
          </button>

          <button
            onClick={togglePlay}
            className={`px-5 py-1.5 rounded text-[11px] font-bold uppercase tracking-wider transition-all ${
              isPlaying
                ? 'bg-pink-500 text-white shadow-[0_0_15px_rgba(236,72,153,0.5)]'
                : 'border border-pink-500/30 text-pink-400/70 hover:border-pink-500/50 hover:text-pink-300'
            }`}
          >
            {isPlaying ? 'Stop' : 'Play'}
          </button>
        </div>
      </div>

      {/* Grid */}
      <div className="px-4 pb-4">
        {/* Step numbers */}
        <div className="flex mb-1.5 ml-14 pr-4">
          {Array.from({ length: NUM_STEPS }, (_, i) => {
            const isPlayhead = isPlaying && currentStep === i;
            return (
              <div
                key={i}
                className={`flex-1 text-center text-[10px] font-mono transition-colors ${
                  isPlayhead ? 'text-pink-400' : 'text-pink-500/40'
                } ${i % 4 === 0 && i !== 0 ? 'ml-1.5' : ''}`}
                style={{ minWidth: 0 }}
              >
                {i + 1}
              </div>
            );
          })}
        </div>

        {/* Tracks */}
        <div className="space-y-1.5">
          {tracks.map((track, trackIndex) => {
            const color = COLORS[track.color];
            return (
              <div key={track.name} className="flex items-center">
                {/* Label */}
                <div className="w-14 text-right pr-2 flex-shrink-0">
                  <span
                    className="text-[11px] font-bold uppercase tracking-wide"
                    style={{ color: color.text }}
                  >
                    {track.name}
                  </span>
                </div>

                {/* Steps */}
                <div className="flex-1 flex pr-4">
                  {Array.from({ length: NUM_STEPS }, (_, stepIndex) => {
                    const isActive = pattern[trackIndex]?.[stepIndex] ?? false;
                    const isPlayhead = isPlaying && currentStep === stepIndex;
                    const isTriggered = isPlayhead && isActive;
                    const isBeatStart = stepIndex % 4 === 0 && stepIndex !== 0;

                    return (
                      <button
                        key={stepIndex}
                        onClick={() => toggleStep(trackIndex, stepIndex)}
                        className={`
                          seq-step flex-1 h-7 rounded-sm
                          ${isTriggered ? 'seq-step-triggered' : ''}
                          ${isBeatStart ? 'ml-1.5' : 'ml-0.5'}
                        `}
                        style={{
                          minWidth: 0,
                          background: isActive ? color.active : color.inactive,
                          border: `1px solid ${isActive ? color.activeBorder : color.border}`,
                          boxShadow: isActive ? color.glow : 'none',
                        }}
                      />
                    );
                  })}
                </div>
              </div>
            );
          })}
        </div>

        {/* Beat markers */}
        <div className="flex mt-2 ml-14 pr-4">
          {[1, 2, 3, 4].map((beat, i) => (
            <div
              key={beat}
              className={`flex-1 text-center text-[10px] font-mono text-pink-500/30 ${i > 0 ? 'ml-1.5' : ''}`}
              style={{ minWidth: 0 }}
            >
              {beat}
            </div>
          ))}
        </div>
      </div>
    </div>
  );
}
