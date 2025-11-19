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
import { OscillatorSection } from './sections/OscillatorSection';
import { EnvelopeSection } from './sections/EnvelopeSection';
import { ReverbSection } from './sections/ReverbSection';
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
      <div className="w-full bg-black">
        {/* Web Controls - WAM-only MIDI and audio status */}
        <WebControls audioStatus={audioStatus} />

        {/* Plugin Content - Main Container */}
        <div id="plugin-body" className="w-[1100px] mx-auto bg-gradient-to-br from-neutral-900 via-stone-900 to-neutral-950 border border-orange-800/30 rounded-lg shadow-2xl p-3">
          {/* Plugin Title and Version */}
          <PluginHeader />

          {/* Output Level Meters */}
          <OutputMeters />

          {/* Main Controls Area */}
          <div className="flex gap-6 mb-3">
            {/* Left side - Oscillators, Envelope, and Reverb */}
            <div className="flex-1">
              {/* Oscillators */}
              <OscillatorSection />

              {/* Envelope and Reverb */}
              <div className="flex gap-12 justify-center">
                <EnvelopeSection />
                <ReverbSection />
              </div>
            </div>

            {/* Right side - Master gain */}
            <MasterSection />
          </div>

          {/* Keyboard Section */}
          <KeyboardSection />
        </div>
      </div>
    </ParameterProvider>
  );
}
