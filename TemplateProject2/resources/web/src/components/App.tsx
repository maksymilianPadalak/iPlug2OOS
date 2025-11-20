/**
 * Main App Component - Effect Plugin
 * Orchestrates all UI sections for the effect plugin
 */

import React from 'react';
import { initializeEnvironment } from '../utils/environment';
import { initializeWAM, toggleTestTone, loadAudioFile, playLoadedAudioFile, stopAudioFile } from '../audio/wam-controller';

// System components
import { ParameterProvider } from './system/ParameterContext';

// Section components
import { WebControls } from './sections/WebControls';
import { PluginHeader } from './sections/PluginHeader';
import { OutputSection } from './sections/OutputSection';
import { EffectParametersSection } from './sections/EffectParametersSection';

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
        {/* Web Controls - WAM-only audio input controls */}
        <WebControls
          audioStatus={audioStatus}
          inputMode={inputMode}
          onInputModeChange={handleInputModeChange}
          testToneEnabled={testToneEnabled}
          onTestToneToggle={handleTestToneToggle}
          audioFilePlaying={audioFilePlaying}
          onFileSelected={handleFileSelected}
          onPlayAudioFile={handlePlayAudioFile}
          onStopAudioFile={handleStopAudioFile}
          currentFileName={currentFileName}
        />

        {/* Main content centered in remaining space */}
        <div className="flex-1 flex items-center justify-center p-6">
          {/* Plugin Content - Horizontal Layout */}
          <div id="plugin-body" className="w-full max-w-6xl p-8">
            {/* Plugin Title and Version */}
            <PluginHeader />

            {/* Output Level Meters */}
            <OutputSection />

            {/* Effect Parameters (Gain + Delay) */}
            <EffectParametersSection />
          </div>
        </div>
      </div>
    </ParameterProvider>
  );
}
