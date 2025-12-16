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

import { BridgeProvider } from '@/glue/BridgeProvider';
import { WebControls } from '@/components/staticComponents/WebControls';
import { PluginBody } from '@/components/PluginBody';
import { AudioFileInput } from '@/components/inputs/AudioFileInput';

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
    <BridgeProvider>
      <div className="h-screen w-full bg-[#F5F0E6] flex flex-col overflow-hidden text-orange-100">
        {/* Fixed header */}
        <div className="flex-shrink-0 px-2 pt-2">
          <WebControls audioStatus={audioStatus} />
        </div>

        {/* Scrollable plugin body - fixed width, no horizontal scroll */}
        <div className="flex-1 overflow-y-auto overflow-x-hidden px-2 pt-4 pb-3 moderne-scroll">
          <div className="w-[1100px] max-w-full mx-auto">
            <PluginBody />
          </div>
        </div>

        {/* Audio file input at bottom (instead of keyboard) */}
        <div className="flex-shrink-0 px-2 pb-4">
          <div className="w-[1100px] max-w-full mx-auto">
            <div className="bg-gradient-to-br from-[#F5E6D3] via-[#EDE0CC] to-[#E8D4B8] rounded-2xl p-4 shadow-md">
              <div className="flex items-center justify-between">
                <div className="flex items-center gap-3">
                  <span className="text-[#2a2a2a] text-xs font-bold uppercase tracking-wider">
                    Audio Input
                  </span>
                  <span className="text-[#666] text-[10px] uppercase tracking-wider">
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
  );
}
