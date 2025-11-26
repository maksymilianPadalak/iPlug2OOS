/**
 * Plugin App - Shell
 *
 * Contains glue code and fixed utilities (WebControls, Keyboard).
 * PluginBody contains all AI-modifiable content.
 */

import React from 'react';
import { initializeWAM, setupMIDIDevices } from '@/audio/wam-controller';
import { initializeEnvironment } from '@/utils/environment';

import { BridgeProvider } from '@/glue/BridgeProvider';
import { WebControls } from '@/components/sections/WebControls';
import { Section } from '@/components/layouts/Section';
import { KeyboardSection } from '@/components/sections/KeyboardSection';
import { PluginBody } from '@/components/PluginBody';

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
          <PluginBody />
          <Section title="Keyboard">
            <KeyboardSection />
          </Section>
        </div>
      </div>
    </BridgeProvider>
  );
}
