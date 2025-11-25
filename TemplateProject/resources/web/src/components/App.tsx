/**
 * Main App Component - AI Plugin UI Base Template
 * 
 * This is the base template for AI-generated plugin UIs.
 * AI patches add sections at the marked injection point.
 */

import React from 'react';
import { initializeWAM, setupMIDIDevices } from '../audio/wam-controller';
import { initializeEnvironment } from '../utils/environment';

// System
import { ParameterProvider } from './system/ParameterContext';

// Layout
import { WebControls } from './sections/WebControls';
import { PluginHeader } from './sections/PluginHeader';
import { Section } from './layouts/Section';
import { KeyboardSection } from './sections/KeyboardSection';

// Controls
import { Knob } from './controls/Knob';
import { EParams } from '../config/runtimeParameters';

// AI_IMPORT_MARKER

export function App() {
  const [audioStatus, setAudioStatus] = React.useState<'working' | 'not-working' | null>(null);

  React.useEffect(() => {
    const env = initializeEnvironment();

    if (env === 'wam') {
      initializeWAM().then((wamController) => {
        if (wamController) {
          window.TemplateProject_WAM = wamController;
          setAudioStatus('working');
          setupMIDIDevices();
        } else {
          setAudioStatus('not-working');
        }
      }).catch((error) => {
        console.error('Error initializing audio:', error);
        setAudioStatus('not-working');
      });
    }
  }, []);

  return (
    <ParameterProvider>
      <div className="min-h-screen w-full bg-neutral-950 py-8 px-4 text-orange-100">
        <div className="mx-auto flex w-full max-w-5xl flex-col gap-4">
          <WebControls audioStatus={audioStatus} />

          <div
            id="plugin-body"
            className="rounded-2xl border border-orange-900/40 bg-gradient-to-b from-stone-900 via-neutral-950 to-black shadow-[0_20px_60px_rgba(0,0,0,0.6)] p-8"
          >
            <div className="flex flex-col gap-6">
              <PluginHeader />

              {/* Example - AI will replace this */}
              <Section title="Master" description="Main output">
                <Knob paramId={EParams.kParamGain} label="Gain" size="lg" />
              </Section>

              <Section title="Keyboard" description="MIDI input">
                <KeyboardSection />
              </Section>

              {/* AI_GENERATED_SECTIONS */}
            </div>
          </div>
        </div>
      </div>
    </ParameterProvider>
  );
}
