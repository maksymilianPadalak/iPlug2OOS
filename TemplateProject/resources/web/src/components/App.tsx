/**
 * Plugin App - Minimal Base
 *
 * Single Gain knob. Build custom components on top of this.
 */

import React from 'react';
import { initializeWAM, setupMIDIDevices } from '../audio/wam-controller';
import { initializeEnvironment } from '../utils/environment';

import { BridgeProvider } from '../glue/BridgeProvider';
import { WebControls } from './sections/WebControls';
import { PluginHeader } from './sections/PluginHeader';
import { Section } from './layouts/Section';
import { KeyboardSection } from './sections/KeyboardSection';
import { Knob } from './controls/Knob';
import { EParams } from '../config/runtimeParameters';

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
    <BridgeProvider>
      <div className="min-h-screen w-full bg-neutral-950 py-4 px-2 text-orange-100">
        <div className="mx-auto flex w-full max-w-4xl flex-col gap-3">
          <WebControls audioStatus={audioStatus} />

          <div
            id="plugin-body"
            className="rounded-2xl border border-orange-900/40 bg-gradient-to-b from-stone-900 via-neutral-950 to-black shadow-[0_20px_60px_rgba(0,0,0,0.6)] p-4"
          >
            <div className="flex flex-col gap-4">
              <PluginHeader />

              <Section title="Master">
                <div className="flex items-center justify-center h-24">
                  <Knob paramId={EParams.kParamGain} label="Gain" size="lg" />
                </div>
              </Section>

              <Section title="Keyboard">
                <KeyboardSection />
              </Section>
            </div>
          </div>
        </div>
      </div>
    </BridgeProvider>
  );
}
