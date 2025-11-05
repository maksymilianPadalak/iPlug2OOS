/**
 * Main entry point for the web UI
 */

import { initializeEnvironment } from './utils/environment';
import { setupCallbacks } from './communication/callbacks';
import { initializeWAM, setupMIDIDevices } from './audio/wam-controller';
import { initKeyboard, setupKeyboardHandlers } from './audio/midi';
import { initLFOWaveform } from './ui/lfo-visualizer';
import { updateParam, updateParamEnum, toggleLFOSync } from './ui/parameters';
import { EParams } from './config/constants';

// Prevent tab key navigation and touch scrolling
document.addEventListener('keydown', (e) => {
  if (e.keyCode === 9) e.preventDefault();
});

document.addEventListener('touchmove', (e) => {
  e.preventDefault();
}, { passive: false });

// Initialize environment detection
const env = initializeEnvironment();

// Setup callbacks for processor-to-UI communication
setupCallbacks();

// Initialize UI components
if (document.readyState === 'loading') {
  document.addEventListener('DOMContentLoaded', initializeUI);
} else {
  initializeUI();
}

function initializeUI(): void {
  initKeyboard();
  initLFOWaveform();
  setupKeyboardHandlers();
}

// Expose functions to global scope for HTML onclick handlers
declare global {
  interface Window {
    startWebAudio: () => Promise<void>;
    updateParam: (paramIdx: number, normalizedValue: number) => void;
    updateParamEnum: (paramIdx: number, enumValue: number) => void;
    toggleLFOSync: () => void;
  }
}

/**
 * Start web audio (called from HTML button)
 */
window.startWebAudio = async (): Promise<void> => {
  const startButton = document.getElementById('startWebAudioButton') as HTMLButtonElement;
  if (startButton) {
    startButton.disabled = true;
  }

  try {
    const wamController = await initializeWAM();
    
    if (wamController) {
      window.TemplateProject_WAM = wamController;
      
      // Update status
      const statusEl = document.getElementById('adapterStatus');
      if (statusEl) {
        statusEl.textContent = '✓ AudioWorklet initialized and ready!';
        statusEl.style.color = '#00ff00';
      }

      // Setup MIDI devices
      setupMIDIDevices();

      // Enable start button (for re-init if needed)
      if (startButton) {
        startButton.setAttribute('disabled', 'true');
      }
    } else {
      throw new Error('Failed to initialize WAM controller');
    }
  } catch (error) {
    console.error('Error starting web audio:', error);
    const statusEl = document.getElementById('adapterStatus');
    if (statusEl) {
      statusEl.textContent = '✗ Error initializing audio. See console for details.';
      statusEl.style.color = '#ff0000';
    }
    if (startButton) {
      startButton.disabled = false;
    }
  }
};

// Expose parameter update functions
window.updateParam = (paramIdx: number, normalizedValue: number) => {
  updateParam(paramIdx as EParams, normalizedValue);
};

window.updateParamEnum = (paramIdx: number, enumValue: number) => {
  updateParamEnum(paramIdx as EParams, enumValue);
};

window.toggleLFOSync = toggleLFOSync;

// Handle websocket mode
if ((window as any).WEBSOCKET_MODE === true) {
  const buttonsEl = document.getElementById('buttons');
  if (buttonsEl) {
    buttonsEl.style.display = 'none';
  }
}

