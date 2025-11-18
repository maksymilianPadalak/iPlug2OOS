/**
 * Simple Effect Plugin UI
 * Wide horizontal layout - no scrolling needed
 */

import React from 'react';
import { ParameterProvider } from './ParameterContext';
import { Knob } from './Knob';
import { Meter } from './Meter';
import { TestToneButton } from './TestToneButton';
import { AudioFileInput } from './AudioFileInput';
import { EParams } from '../config/constants';
import { initializeEnvironment } from '../utils/environment';
import { initializeWAM, toggleTestTone, loadAudioFile, playLoadedAudioFile, stopAudioFile } from '../audio/wam-controller';

type InputMode = 'mic' | 'file';

export function App() {
  const [audioStatus, setAudioStatus] = React.useState<'working' | 'not-working' | null>(null);
  const [inputMode, setInputMode] = React.useState<InputMode>('mic');
  const [testToneEnabled, setTestToneEnabled] = React.useState(false);
  const [audioFilePlaying, setAudioFilePlaying] = React.useState(false);
  const [currentFileName, setCurrentFileName] = React.useState<string | undefined>(undefined);

  // Initialize environment detection and auto-start WAM
  React.useEffect(() => {
    const env = initializeEnvironment();

    if (env === 'wam') {
      // Auto-start web audio in WAM mode
      initializeWAM().then((wamController) => {
        if (wamController) {
          window.TemplateProject2_WAM = wamController;
          setAudioStatus('working');
        } else {
          setAudioStatus('not-working');
        }
      }).catch((error) => {
        console.error('Error auto-initializing web audio:', error);
        setAudioStatus('not-working');
      });
    }
  }, []);

  // Handle input mode change
  const handleInputModeChange = React.useCallback((mode: InputMode) => {
    setInputMode(mode);
    
    // Stop the other input when switching modes
    if (mode === 'mic') {
      // Switching to mic - stop audio file
      if (audioFilePlaying) {
        stopAudioFile();
        setAudioFilePlaying(false);
      }
    } else {
      // Switching to file - stop microphone
      if (testToneEnabled) {
        toggleTestTone(false).catch((error) => {
          console.error('Error stopping microphone:', error);
        });
        setTestToneEnabled(false);
      }
    }
  }, [audioFilePlaying, testToneEnabled]);

  // Handle test tone toggle
  const handleTestToneToggle = React.useCallback(async () => {
    console.log('🎵 Test tone button clicked, current state:', testToneEnabled);
    
    // Stop audio file if playing
    if (audioFilePlaying) {
      stopAudioFile();
      setAudioFilePlaying(false);
    }
    
    const newState = !testToneEnabled;
    try {
      await toggleTestTone(newState);
      setTestToneEnabled(newState);
      console.log('✅ Test tone toggled successfully to:', newState);
    } catch (error) {
      console.error('❌ Error toggling test tone:', error);
    }
  }, [testToneEnabled, audioFilePlaying]);

  // Handle audio file selection (loads but doesn't play)
  const handleFileSelected = React.useCallback(async (file: File) => {
    // Stop playback if currently playing
    if (audioFilePlaying) {
      stopAudioFile();
      setAudioFilePlaying(false);
    }

    try {
      await loadAudioFile(file);
      setCurrentFileName(file.name);
      console.log('✅ Audio file loaded (ready to play):', file.name);
    } catch (error) {
      console.error('❌ Error loading audio file:', error);
      alert('Failed to load audio file. Please try another file.');
    }
  }, [audioFilePlaying]);

  // Handle play audio file
  const handlePlayAudioFile = React.useCallback(async () => {
    // Stop microphone if active
    if (testToneEnabled) {
      try {
        await toggleTestTone(false);
        setTestToneEnabled(false);
      } catch (error) {
        console.error('Error stopping microphone:', error);
      }
    }

    try {
      await playLoadedAudioFile();
      setAudioFilePlaying(true);
      console.log('✅ Audio file playback started');
    } catch (error) {
      console.error('❌ Error playing audio file:', error);
      if ((error as Error).message.includes('No audio file loaded')) {
        alert('Please select an audio file first.');
      } else {
        alert('Failed to play audio file.');
      }
    }
  }, [testToneEnabled]);

  // Handle stop audio file
  const handleStopAudioFile = React.useCallback(() => {
    stopAudioFile();
    setAudioFilePlaying(false);
    console.log('🛑 Audio file stopped');
  }, []);

  return (
    <ParameterProvider>
      <div className="w-full h-full bg-gradient-to-br from-stone-950 via-black to-cyan-950/20 flex flex-col">
        {/* Web Controls - Only visible in WAM mode - Fixed at top */}
        {audioStatus && (
          <div className="wam-only flex items-center justify-between p-3 border-b border-cyan-600/20">
            {/* Audio Input Controls - Only visible in WAM mode (web), not in AU */}
            <div className="flex flex-col items-center gap-2 px-4 py-2 border border-cyan-600/30 bg-black/40 rounded max-w-xs">
              <span className="text-cyan-300 text-xs font-bold uppercase tracking-wider whitespace-nowrap">
                Audio Input
              </span>

              {/* Input Mode Selector */}
              <div className="flex gap-2 p-1 bg-black/60 rounded border border-cyan-600/30 w-full">
                <button
                  onClick={() => handleInputModeChange('mic')}
                  className={`flex-1 px-3 py-1 rounded text-xs font-bold uppercase tracking-wider transition-all whitespace-nowrap ${
                    inputMode === 'mic'
                      ? 'bg-cyan-600 text-white'
                      : 'bg-transparent text-cyan-300/60 hover:text-cyan-300'
                  }`}
                >
                  🎤 MIC
                </button>
                <button
                  onClick={() => handleInputModeChange('file')}
                  className={`flex-1 px-3 py-1 rounded text-xs font-bold uppercase tracking-wider transition-all whitespace-nowrap ${
                    inputMode === 'file'
                      ? 'bg-cyan-600 text-white'
                      : 'bg-transparent text-cyan-300/60 hover:text-cyan-300'
                  }`}
                >
                  📁 FILE
                </button>
              </div>

              {/* Conditional Input Controls */}
              <div className="flex items-center justify-center w-full">
                {inputMode === 'file' ? (
                  <AudioFileInput
                    onFileSelected={handleFileSelected}
                    onPlay={handlePlayAudioFile}
                    onStop={handleStopAudioFile}
                    isPlaying={audioFilePlaying}
                    hasFile={!!currentFileName}
                    currentFileName={currentFileName}
                  />
                ) : (
                  <TestToneButton
                    isPlaying={testToneEnabled}
                    onToggle={handleTestToneToggle}
                  />
                )}
              </div>
            </div>

            {/* Audio Status */}
            <div className={`px-3 py-1 rounded text-xs font-bold uppercase tracking-wider whitespace-nowrap ${
              audioStatus === 'working' ? 'bg-emerald-500/20 text-emerald-400' : 'bg-rose-500/20 text-rose-400'
            }`}>
              {audioStatus === 'working' ? '✓ AUDIO READY' : '✗ AUDIO ERROR'}
            </div>
          </div>
        )}

        {/* Main content centered in remaining space */}
        <div className="flex-1 flex items-center justify-center p-6">

        {/* Plugin Content - Horizontal Layout */}
        <div id="plugin-body" className="w-full max-w-6xl p-8">
          {/* Title at top */}
          <div className="flex flex-col items-center gap-2 mb-8">
            <h1 className="text-4xl font-black uppercase tracking-tighter text-transparent bg-clip-text bg-gradient-to-r from-cyan-200 via-cyan-400 to-cyan-200">
              DELAY
            </h1>
            <div className="h-0.5 w-12 bg-gradient-to-r from-transparent via-cyan-500 to-transparent" />
            <span className="text-cyan-300/60 text-[10px] uppercase tracking-widest font-bold">
              Effect Plugin
            </span>
          </div>

          {/* Output Meters - Above knobs */}
          <div className="flex justify-center mb-6">
            <div className="flex flex-col items-center gap-3 px-6 py-4 border-2 border-cyan-600/30 bg-black/40 rounded min-w-[200px]">
              <span className="text-cyan-300 text-xs font-bold uppercase tracking-wider">
                Output
              </span>
              <div className="flex items-center justify-center gap-4">
                <div className="flex flex-col items-center gap-2">
                  <span className="text-cyan-200 text-xs font-bold uppercase">L</span>
                  <Meter channel={0} compact={true} />
                </div>
                <div className="flex flex-col items-center gap-2">
                  <span className="text-cyan-200 text-xs font-bold uppercase">R</span>
                  <Meter channel={1} compact={true} />
                </div>
              </div>
            </div>
          </div>

          {/* Main controls in a row - all sections have uniform height */}
          <div className="flex items-stretch justify-center gap-8">
            {/* Gain Knob - self-aligned */}
            <div className="flex flex-col items-center justify-center">
              <Knob paramIdx={EParams.kParamGain} label="GAIN" />
            </div>

            {/* Divider */}
            <div className="w-px bg-gradient-to-b from-transparent via-cyan-500/30 to-transparent self-stretch" />

            {/* Delay Controls - matched height */}
            <div className="flex flex-col items-center justify-center gap-3">
              <span className="text-cyan-300 text-xs font-bold uppercase tracking-wider">Delay</span>
              <div className="flex gap-4">
                <Knob paramIdx={EParams.kParamDelayTime} label="TIME" />
                <Knob paramIdx={EParams.kParamDelayFeedback} label="FEEDBACK" />
                <Knob paramIdx={EParams.kParamDelayDry} label="DRY" />
                <Knob paramIdx={EParams.kParamDelayWet} label="WET" />
              </div>
            </div>

          </div>
        </div>
        </div>
      </div>
    </ParameterProvider>
  );
}
