/**
 * Plugin App - Shell (Effect Template)
 *
 * Contains glue code and fixed utilities (WebControls).
 * PluginBody contains all AI-modifiable content.
 * Note: Effects don't have a keyboard - they process audio input via file or mic.
 */

import React from 'react';
import { initializeWAM, loadAudioFile, playLoadedAudioFile, stopAudioFile } from '@/audio/wam-controller';
import { initializeEnvironment } from '@/utils/environment';

import { BridgeProvider } from 'sharedUi/BridgeProvider';
import { RuntimeParametersProvider } from 'sharedUi/RuntimeParametersProvider';
import { WebControls } from 'sharedUi/components/WebControls';
import { AudioFileInput } from 'sharedUi/components/AudioFileInput';
import { controlTags, runtimeParameters } from '@/config/runtimeParameters';
import { PluginBody } from '@/components/PluginBody';

export function App() {
  const [audioStatus, setAudioStatus] = React.useState<'working' | 'not-working' | null>(null);
  const [audioFilePlaying, setAudioFilePlaying] = React.useState(false);
  const [hasFile, setHasFile] = React.useState(false);
  const [currentFileName, setCurrentFileName] = React.useState<string | undefined>(undefined);

  React.useEffect(() => {
    const env = initializeEnvironment();

    if (env === 'wam') {
      initializeWAM().then((wamController) => {
        if (wamController) {
          window.Plugin_WAM = wamController;
          setAudioStatus('working');
        } else {
          setAudioStatus('not-working');
        }
      }).catch((error) => {
        console.error('Error initializing audio:', error);
        setAudioStatus('not-working');
      });
    }

    return () => {
      stopAudioFile();
    };
  }, []);

  const handleFileSelected = React.useCallback(async (file: File) => {
    if (audioFilePlaying) {
      stopAudioFile();
      setAudioFilePlaying(false);
    }

    try {
      await loadAudioFile(file);
      setHasFile(true);
      setCurrentFileName(file.name);
    } catch (error) {
      console.error('Error loading audio file:', error);
      alert('Failed to load audio file. Please try another file.');
    }
  }, [audioFilePlaying]);

  const handlePlayAudioFile = React.useCallback(async () => {
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
  }, []);

  const handleStopAudioFile = React.useCallback(() => {
    stopAudioFile();
    setAudioFilePlaying(false);
  }, []);

  return (
    <RuntimeParametersProvider parameters={runtimeParameters}>
      <BridgeProvider controlTags={controlTags}>
        <div
          className="h-screen w-full flex flex-col overflow-hidden text-orange-100 relative bg-transparent"
        >

          {/* Fixed header */}
          <div className="flex-shrink-0 px-2 pt-2 relative z-10">
            <WebControls audioStatus={audioStatus} />
          </div>

          {/* Scrollable plugin body - constrained to 1000x625 */}
          <div className="flex-1 overflow-y-auto overflow-x-hidden px-2 pt-4 pb-3 moderne-scroll flex justify-center relative z-10">
            <div
              className="w-[950px] h-[525px] flex-shrink-0 rounded-lg"
              style={{
                boxShadow: '0 0 40px rgba(255, 255, 255, 0.18), 0 0 80px rgba(255, 255, 255, 0.10), 0 0 120px rgba(255, 255, 255, 0.05)',
                border: '1px solid rgba(255, 255, 255, 0.15)',
              }}
            >
              <PluginBody />
            </div>
          </div>

          {/* Audio file input at bottom */}
          <div className="flex-shrink-0 px-2 pb-4 flex justify-center relative z-10">
            <div className="w-[950px] max-w-full">
              <div className="bg-white/5 backdrop-blur-sm border border-white/10 rounded-2xl p-4">
                <div className="flex items-center justify-between">
                  <div className="flex items-center gap-3">
                    <span className="text-white/80 text-xs font-bold uppercase tracking-wider">
                      Audio Input
                    </span>
                    <span className="text-white/40 text-[10px] uppercase tracking-wider">
                      Load an audio file to test the effect
                    </span>
                  </div>
                  <AudioFileInput
                    onFileSelected={handleFileSelected}
                    onPlay={handlePlayAudioFile}
                    onStop={handleStopAudioFile}
                    isPlaying={audioFilePlaying}
                    hasFile={hasFile}
                    currentFileName={currentFileName}
                  />
                </div>
              </div>
            </div>
          </div>
        </div>
      </BridgeProvider>
    </RuntimeParametersProvider>
  );
}
