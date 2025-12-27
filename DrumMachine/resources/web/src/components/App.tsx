/**
 * Plugin App - Shell
 *
 * Contains glue code and fixed utilities (WebControls, StepSequencer).
 * PluginBody contains all AI-modifiable content.
 */

import React from 'react';
import { initializeWAM, setupMIDIDevices } from '@/audio/wam-controller';
import { initializeEnvironment } from '@/utils/environment';

import { BridgeProvider } from 'sharedUi/BridgeProvider';
import { RuntimeParametersProvider } from 'sharedUi/RuntimeParametersProvider';
import { WebControls } from 'sharedUi/components/WebControls';
import { StepSequencer } from 'sharedUi/components/StepSequencer';
import { sendNoteOn, sendNoteOff } from 'sharedUi/bridge';
import { trigger } from 'sharedUi/state/triggerStore';
import { controlTags, runtimeParameters, voiceConfig } from '@/config/runtimeParameters';
import { PluginBody } from '@/components/PluginBody';

// Wrap sendNoteOn to also update trigger store for UI visualization
function handleNoteOn(noteNumber: number, velocity: number) {
  trigger(noteNumber);
  sendNoteOn(noteNumber, velocity);
}

function handleNoteOff(noteNumber: number, velocity: number) {
  // No-op for release - the shared trigger auto-releases after 100ms
  sendNoteOff(noteNumber, velocity);
}

function StepSequencerWrapper() {
  return (
    <StepSequencer
      voiceConfig={voiceConfig}
      onNoteOn={handleNoteOn}
      onNoteOff={handleNoteOff}
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
        <div
          className="h-screen w-full flex flex-col overflow-hidden text-orange-100 relative bg-transparent"
        >

          {/* Fixed header - full width */}
          <div className="flex-shrink-0 pt-2 relative z-10">
            <WebControls audioStatus={audioStatus} />
          </div>

          {/* Scrollable plugin body - fixed width, no horizontal scroll */}
          <div className="flex-1 overflow-y-auto overflow-x-hidden px-2 pt-4 pb-3 moderne-scroll flex justify-center relative z-10">
            <div
              className="w-[950px] max-w-full rounded-lg"
              style={{
                boxShadow: '0 0 40px rgba(255, 255, 255, 0.18), 0 0 80px rgba(255, 255, 255, 0.10), 0 0 120px rgba(255, 255, 255, 0.05)',
                border: '1px solid rgba(255, 255, 255, 0.15)',
              }}
            >
              <PluginBody />
            </div>
          </div>

          {/* Fixed step sequencer at bottom - centered */}
          <div className="flex-shrink-0 pb-2 relative z-10 flex justify-center">
            <StepSequencerWrapper />
          </div>
        </div>
      </BridgeProvider>
    </RuntimeParametersProvider>
  );
}
