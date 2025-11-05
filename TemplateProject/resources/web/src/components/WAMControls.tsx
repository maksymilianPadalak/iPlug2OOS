/**
 * WAM Controls component (only visible in browser mode)
 */

import React, { useState } from 'react';
import { initializeWAM, setupMIDIDevices } from '../audio/wam-controller';

export function WAMControls() {
  const [status, setStatus] = useState('Ready - Click \'Start web audio!\' to begin');
  const [isInitializing, setIsInitializing] = useState(false);
  const [wamReady, setWamReady] = useState(false);

  const handleStartWebAudio = async () => {
    setIsInitializing(true);
    setStatus('Initializing...');

    try {
      const wamController = await initializeWAM();
      
      if (wamController) {
        window.TemplateProject_WAM = wamController;
        setStatus('✓ AudioWorklet initialized and ready!');
        setWamReady(true);
        setIsInitializing(false);
        
        // Setup MIDI devices after a short delay to ensure everything is ready
        setTimeout(() => {
          setupMIDIDevices();
        }, 100);
      } else {
        throw new Error('Failed to initialize WAM controller');
      }
    } catch (error) {
      console.error('Error starting web audio:', error);
      setStatus('✗ Error initializing audio. See console for details.');
      setIsInitializing(false);
    }
  };

  return (
    <div className="wam-only">
      <div id="buttons" style={{ marginBottom: '10px' }}>
        <button
          type="button"
          id="startWebAudioButton"
          onClick={handleStartWebAudio}
          disabled={isInitializing || wamReady}
        >
          {isInitializing ? 'Initializing...' : wamReady ? '✓ Ready' : 'Start web audio!'}
        </button>
        <select id="midiInSelect" disabled={!wamReady}>
          <option value="default">Midi input</option>
        </select>
        <select id="midiOutSelect" disabled={!wamReady}>
          <option value="default">Midi output</option>
        </select>
        <progress value={0} max={100} id="progress" />
      </div>
      <div id="greyout" className="wam-only">
        <div id="status">{status}</div>
      </div>
    </div>
  );
}


