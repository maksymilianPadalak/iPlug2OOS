/**
 * Main App Component
 * Orchestrates all UI sections for the audio plugin
 */

import React from 'react';
import { initializeWAM, setupMIDIDevices } from '../audio/wam-controller';
import { initializeEnvironment } from '../utils/environment';

// System components
import { ParameterProvider } from './system/ParameterContext';

// Section components
import { WebControls } from './sections/WebControls';
import { PluginHeader } from './sections/PluginHeader';
import { OutputMeters } from './sections/OutputMeters';
import { MasterSection } from './sections/MasterSection';
import { KeyboardSection } from './sections/KeyboardSection';

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
      <div className="min-h-screen w-full bg-neutral-950 py-8 px-4 text-orange-100">
        <div className="max-w-5xl mx-auto space-y-4">
          <WebControls audioStatus={audioStatus} />
          <div className="flex justify-center">
            <div
              id="plugin-body"
              className="w-[1100px] rounded-2xl border border-orange-900/40 bg-gradient-to-b from-stone-900 via-neutral-950 to-black shadow-[0_20px_60px_rgba(0,0,0,0.6)] p-8 space-y-8"
            >
              <PluginHeader />

              <div className="flex flex-col gap-8">
                <OutputMeters />
                <div className="flex justify-center">
                  <MasterSection />
                </div>
                <KeyboardSection />
              </div>
            </div>
          </div>
        </div>
      </div>
    </ParameterProvider>
  );
}
