/**
 * WAM Controls component - Berlin Brutalism Style
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
    <div className="wam-only mb-6">
      <div id="buttons" className="flex flex-wrap gap-2 justify-center mb-4">
        <button
          type="button"
          id="startWebAudioButton"
          onClick={handleStartWebAudio}
          disabled={isInitializing || wamReady}
          className="brutal-button disabled:opacity-50 disabled:cursor-not-allowed"
        >
          {isInitializing ? 'INITIALIZING...' : wamReady ? '✓ READY' : 'START WEB AUDIO!'}
        </button>
        <select 
          id="midiInSelect" 
          disabled={!wamReady}
          className="brutal-button disabled:opacity-50 disabled:cursor-not-allowed"
        >
          <option value="default">MIDI INPUT</option>
        </select>
        <select 
          id="midiOutSelect" 
          disabled={!wamReady}
          className="brutal-button disabled:opacity-50 disabled:cursor-not-allowed"
        >
          <option value="default">MIDI OUTPUT</option>
        </select>
      </div>
      <div id="greyout" className="wam-only text-center">
        <div id="status" className="inline-block px-6 py-3 border-2 border-white bg-black text-white text-xs font-mono uppercase tracking-wider">
          {status}
        </div>
      </div>
    </div>
  );
}
