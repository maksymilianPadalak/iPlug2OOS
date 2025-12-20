/**
 * Plugin App - Shell
 *
 * Contains glue code and fixed utilities (WebControls, Keyboard).
 * PluginBody contains all AI-modifiable content.
 */

import React from 'react';
import { initializeWAM, setupMIDIDevices } from '@/audio/wam-controller';
import { initializeEnvironment } from '@/utils/environment';

import { BridgeProvider } from 'sharedUi/BridgeProvider';
import { RuntimeParametersProvider } from 'sharedUi/RuntimeParametersProvider';
import { WebControls } from 'sharedUi/components/WebControls';
import { KeyboardSection } from 'sharedUi/components/KeyboardSection';
import { useMidi } from 'sharedUi/hooks/useMidi';
import { sendNoteOn, sendNoteOff } from 'sharedUi/bridge';
import { controlTags, runtimeParameters } from '@/config/runtimeParameters';
import { PluginBody } from '@/components/PluginBody';

function KeyboardWrapper() {
  const { activeNotes } = useMidi();
  return (
    <KeyboardSection
      activeNotes={activeNotes}
      onNoteOn={sendNoteOn}
      onNoteOff={sendNoteOff}
    />
  );
}

export function App() {
  const [audioStatus, setAudioStatus] = React.useState<'working' | 'not-working' | null>(null);

  React.useEffect(() => {
    const env = initializeEnvironment();

    if (env === 'wam') {
      initializeWAM().then((wamController) => {
        if (wamController) {
          (window as any).Plugin_WAM = wamController;
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
    <RuntimeParametersProvider parameters={runtimeParameters}>
      <BridgeProvider controlTags={controlTags}>
        <div className="h-screen w-full bg-[#F5F0E6] flex flex-col overflow-hidden text-orange-100">
          {/* Fixed header */}
          <div className="flex-shrink-0 px-2 pt-2">
            <WebControls audioStatus={audioStatus} />
          </div>

          {/* Scrollable plugin body - fixed width, no horizontal scroll */}
          <div className="flex-1 overflow-y-auto overflow-x-hidden px-2 pt-4 pb-3 moderne-scroll flex justify-center">
            <div className="w-[950px] max-w-full">
              <PluginBody />
            </div>
          </div>

          {/* Fixed keyboard at bottom */}
          <div className="flex-shrink-0 px-2 pb-2 flex justify-center">
            <div className="w-[950px] max-w-full">
              <KeyboardWrapper />
            </div>
          </div>
        </div>
      </BridgeProvider>
    </RuntimeParametersProvider>
  );
}
