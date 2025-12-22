/**
 * Step Sequencer Component
 *
 * 16-step drum sequencer with transport controls.
 * Uses Web Worker for timing to avoid main thread jank.
 * Accepts voiceConfig prop for dynamic track configuration.
 */

import { useState, useCallback, useEffect, useRef, useMemo } from 'react';

const NUM_STEPS = 16;
const DEFAULT_BPM = 120;

type DrumColor = 'cyan' | 'magenta' | 'green' | 'orange';

const COLOR_STYLES: Record<DrumColor, { active: string; inactive: string; playing: string }> = {
  orange: {
    active: 'bg-orange-500 border-orange-400 shadow-[0_0_8px_rgba(255,165,0,0.6)]',
    inactive: 'bg-orange-950/30 border-orange-800/40 hover:bg-orange-900/40',
    playing: 'bg-orange-400 border-orange-300 shadow-[0_0_12px_rgba(255,165,0,0.8)]',
  },
  cyan: {
    active: 'bg-cyan-500 border-cyan-400 shadow-[0_0_8px_rgba(0,255,255,0.6)]',
    inactive: 'bg-cyan-950/30 border-cyan-800/40 hover:bg-cyan-900/40',
    playing: 'bg-cyan-400 border-cyan-300 shadow-[0_0_12px_rgba(0,255,255,0.8)]',
  },
  magenta: {
    active: 'bg-pink-500 border-pink-400 shadow-[0_0_8px_rgba(255,0,128,0.6)]',
    inactive: 'bg-pink-950/30 border-pink-800/40 hover:bg-pink-900/40',
    playing: 'bg-pink-400 border-pink-300 shadow-[0_0_12px_rgba(255,0,128,0.8)]',
  },
  green: {
    active: 'bg-green-500 border-green-400 shadow-[0_0_8px_rgba(0,255,100,0.6)]',
    inactive: 'bg-green-950/30 border-green-800/40 hover:bg-green-900/40',
    playing: 'bg-green-400 border-green-300 shadow-[0_0_12px_rgba(0,255,100,0.8)]',
  },
};

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

// Voice configuration type (matches runtimeParameters.ts - no color, UI assigns it)
export type VoiceConfig = {
  name: string;
  midiNote: number;
};

// Colors assigned by UI in order
const VOICE_COLORS: DrumColor[] = ['orange', 'cyan', 'magenta', 'green'];

export type StepSequencerProps = {
  voiceConfig: VoiceConfig[];
  onNoteOn: (noteNum: number, velocity: number) => void;
  onNoteOff: (noteNum: number, velocity: number) => void;
};

export function StepSequencer({ voiceConfig, onNoteOn, onNoteOff }: StepSequencerProps) {
  // Derive tracks from voiceConfig, assigning colors by index
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

  // Keep refs in sync
  useEffect(() => {
    patternRef.current = pattern;
    workerRef.current?.postMessage({ type: 'updatePattern', pattern });
  }, [pattern]);

  useEffect(() => {
    bpmRef.current = bpm;
    workerRef.current?.postMessage({ type: 'updateBpm', bpm });
  }, [bpm]);

  // Keep callback refs in sync (avoids worker recreation)
  useEffect(() => {
    onNoteOnRef.current = onNoteOn;
  }, [onNoteOn]);

  useEffect(() => {
    onNoteOffRef.current = onNoteOff;
  }, [onNoteOff]);

  // Initialize worker once on mount
  useEffect(() => {
    const { worker, blobUrl } = createWorker();
    workerRef.current = worker;
    blobUrlRef.current = blobUrl;

    // Handle messages from worker
    worker.onmessage = (event) => {
      const message = event.data;

      if (message.type === 'tick') {
        setCurrentStep(message.step);

        // Trigger all notes for this step (use refs to avoid stale closures)
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
      // Revoke blob URL to prevent memory leak
      if (blobUrlRef.current) {
        URL.revokeObjectURL(blobUrlRef.current);
      }
    };
  }, []); // Empty deps - worker created once on mount

  // Toggle a step in the pattern
  const toggleStep = useCallback((trackIndex: number, stepIndex: number) => {
    setPattern(prev => {
      const newPattern = prev.map(row => [...row]);
      newPattern[trackIndex][stepIndex] = !newPattern[trackIndex][stepIndex];
      return newPattern;
    });
  }, []);

  // Start playback
  const startPlayback = useCallback(() => {
    setIsPlaying(true);
    workerRef.current?.postMessage({
      type: 'start',
      bpm: bpmRef.current,
      pattern: patternRef.current,
      notes: drumNotesRef.current,
    });
  }, []);

  // Stop playback
  const stopPlayback = useCallback(() => {
    setIsPlaying(false);
    workerRef.current?.postMessage({ type: 'stop' });
  }, []);

  // Toggle play/stop
  const togglePlay = useCallback(() => {
    if (isPlaying) {
      stopPlayback();
    } else {
      startPlayback();
    }
  }, [isPlaying, startPlayback, stopPlayback]);

  // Clear pattern
  const clearPattern = useCallback(() => {
    setPattern(tracks.map(() => Array(NUM_STEPS).fill(false)));
  }, [tracks]);

  // Don't render if no voices configured
  if (tracks.length === 0) {
    return null;
  }

  return (
    <div className="bg-gradient-to-b from-[#1a1a1a] to-[#0a0a0a] rounded-2xl p-4 border border-pink-500/30">
      {/* Header with transport controls */}
      <div className="flex justify-between items-center mb-4">
        <div className="flex items-center gap-4">
          <div className="flex gap-0.5">
            <div className="w-0.5 h-4 bg-gradient-to-b from-pink-500 to-pink-700 rounded-full" />
            <div className="w-0.5 h-3 bg-gradient-to-b from-pink-500 to-pink-700 rounded-full mt-0.5" />
          </div>
          <h3 className="text-pink-200 text-xs font-bold uppercase tracking-[0.2em]">Step Sequencer</h3>
        </div>

        {/* Transport controls */}
        <div className="flex items-center gap-3">
          {/* BPM control */}
          <div className="flex items-center gap-2">
            <span className="text-pink-500/60 text-[10px] font-mono uppercase">BPM</span>
            <input
              type="number"
              value={bpm}
              onChange={(e) => setBpm(Math.max(40, Math.min(300, parseInt(e.target.value) || DEFAULT_BPM)))}
              className="w-14 bg-black/50 border border-pink-500/30 rounded px-2 py-1 text-xs text-pink-200 font-mono text-center"
              min={40}
              max={300}
            />
          </div>

          {/* Clear button */}
          <button
            onClick={clearPattern}
            className="px-3 py-1.5 bg-pink-950/50 border border-pink-500/30 rounded text-pink-300 text-xs font-bold uppercase tracking-wider hover:bg-pink-900/50 transition-colors"
          >
            Clear
          </button>

          {/* Play/Stop button */}
          <button
            onClick={togglePlay}
            className={`px-4 py-1.5 rounded text-xs font-bold uppercase tracking-wider transition-all ${
              isPlaying
                ? 'bg-pink-500 text-white shadow-[0_0_15px_rgba(255,0,128,0.5)]'
                : 'bg-pink-950/50 border border-pink-500/30 text-pink-300 hover:bg-pink-900/50'
            }`}
          >
            {isPlaying ? 'Stop' : 'Play'}
          </button>
        </div>
      </div>

      {/* Step indicators */}
      <div className="flex mb-2 ml-16">
        {Array.from({ length: NUM_STEPS }, (_, i) => (
          <div
            key={`indicator-${i}`}
            className={`w-6 h-1.5 mx-0.5 rounded-full transition-all ${
              currentStep === i && isPlaying
                ? 'bg-pink-400 shadow-[0_0_8px_rgba(255,0,128,0.8)]'
                : i % 4 === 0
                  ? 'bg-pink-800/60'
                  : 'bg-pink-950/40'
            }`}
          />
        ))}
      </div>

      {/* Sequencer grid */}
      <div className="space-y-1">
        {tracks.map((track, trackIndex) => {
          const colorStyle = COLOR_STYLES[track.color];
          return (
            <div key={track.name} className="flex items-center gap-2">
              {/* Track label */}
              <div className="w-14 text-right">
                <span className="text-gray-400 text-[10px] font-bold uppercase tracking-wider">
                  {track.name}
                </span>
              </div>

              {/* Steps */}
              <div className="flex gap-1">
                {Array.from({ length: NUM_STEPS }, (_, stepIndex) => {
                  const isActive = pattern[trackIndex]?.[stepIndex] ?? false;
                  const isCurrentlyPlaying = isPlaying && currentStep === stepIndex && isActive;
                  const isBeatStart = stepIndex % 4 === 0;

                  return (
                    <button
                      key={`${trackIndex}-${stepIndex}`}
                      onClick={() => toggleStep(trackIndex, stepIndex)}
                      className={`
                        w-6 h-8 rounded border transition-all
                        ${isCurrentlyPlaying
                          ? colorStyle.playing
                          : isActive
                            ? colorStyle.active
                            : colorStyle.inactive
                        }
                        ${isBeatStart ? 'ml-1' : ''}
                      `}
                    />
                  );
                })}
              </div>
            </div>
          );
        })}
      </div>

      {/* Beat markers */}
      <div className="flex mt-2 ml-16 text-[9px] text-pink-500/40 font-mono">
        {[1, 2, 3, 4].map((beat) => (
          <div key={beat} className="w-[116px] text-center">
            {beat}
          </div>
        ))}
      </div>
    </div>
  );
}
