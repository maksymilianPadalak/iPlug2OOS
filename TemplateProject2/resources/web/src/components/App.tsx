/**
 * Main App Component - Dreamy Psychedelic Reverb / Space Delay
 */

import React from 'react';
import { initializeEnvironment } from '../utils/environment';
import { initializeWAM, toggleTestTone, loadAudioFile, playLoadedAudioFile, stopAudioFile } from '../audio/wam-controller';

import { ParameterProvider } from './system/ParameterContext';
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

  React.useEffect(() => {
    const env = initializeEnvironment();

    if (env === 'wam') {
      initializeWAM()
        .then((wamController) => {
          if (wamController) {
            window.TemplateProject2_WAM = wamController;
            setAudioStatus('working');
          } else {
            setAudioStatus('not-working');
          }
        })
        .catch((error) => {
          console.error('Error auto-initializing web audio:', error);
          setAudioStatus('not-working');
        });
    }
  }, []);

  const [testToneGuard, setTestToneGuard] = React.useState(false);

  const handleInputModeChange = React.useCallback(
    (mode: InputMode) => {
      setInputMode(mode);
      if (mode === 'mic') {
        if (audioFilePlaying) {
          stopAudioFile();
          setAudioFilePlaying(false);
        }
      } else {
        if (testToneEnabled) {
          toggleTestTone(false)
            .catch((error) => {
              console.error('Error stopping microphone:', error);
            })
            .finally(() => setTestToneEnabled(false));
        }
      }
    },
    [audioFilePlaying, testToneEnabled]
  );

  const handleTestToneToggle = React.useCallback(async () => {
    if (testToneGuard) return;
    setTestToneGuard(true);

    if (audioFilePlaying) {
      stopAudioFile();
      setAudioFilePlaying(false);
    }

    const newState = !testToneEnabled;
    try {
      await toggleTestTone(newState);
      setTestToneEnabled(newState);
    } catch (error) {
      console.error('Error toggling test tone:', error);
    } finally {
      setTestToneGuard(false);
    }
  }, [audioFilePlaying, testToneEnabled, testToneGuard]);

  const handleFileSelected = React.useCallback(
    async (file: File) => {
      if (audioFilePlaying) {
        stopAudioFile();
        setAudioFilePlaying(false);
      }

      try {
        await loadAudioFile(file);
        setCurrentFileName(file.name);
      } catch (error) {
        console.error('Error loading audio file:', error);
        alert('Failed to load audio file. Please try another file.');
      }
    },
    [audioFilePlaying]
  );

  const handlePlayAudioFile = React.useCallback(async () => {
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
    } catch (error) {
      console.error('Error playing audio file:', error);
      if ((error as Error).message.includes('No audio file loaded')) {
        alert('Please select an audio file first.');
      } else {
        alert('Failed to play audio file.');
      }
    }
  }, [testToneEnabled]);

  const handleStopAudioFile = React.useCallback(() => {
    stopAudioFile();
    setAudioFilePlaying(false);
  }, []);

  return (
    <ParameterProvider>
      <div className="w-full h-full bg-[#020617] flex flex-col">
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

        <div className="flex-1 flex items-center justify-center p-4">
          <div
            id="plugin-body"
            className="w-full max-w-5xl p-6 md:p-7 border border-cyan-700/40 bg-gradient-to-br from-black via-slate-950 to-cyan-950/40 shadow-[0_0_80px_rgba(34,211,238,0.4)] flex flex-col gap-4"
          >
            <PluginHeader />

            <div className="flex flex-col gap-4">
              <OutputSection />
              <EffectParametersSection />
            </div>
          </div>
        </div>
      </div>
    </ParameterProvider>
  );
}
