/**
 * WAM (Web Audio Module) controller initialization and management
 */

import { WAMController } from '../types/wam';
import { detectEnvironment } from '../utils/environment';
import { sendParameterValue, sendParameterEnum } from '../communication/iplug-bridge';
import { EParams } from '../config/constants';

/**
 * Initialize WAM controller
 */
export async function initializeWAM(): Promise<WAMController | null> {
  const actx = new AudioContext();
  
  // Safari unlock
  if (actx.state === 'suspended') {
    const unlock = () => {
      actx.resume().then(() => {
        document.body.removeEventListener('touchstart', unlock);
        document.body.removeEventListener('touchend', unlock);
        document.body.removeEventListener('mousedown', unlock);
        document.body.removeEventListener('keydown', unlock);
      });
    };
    document.body.addEventListener('touchstart', unlock);
    document.body.addEventListener('touchend', unlock);
    document.body.addEventListener('mousedown', unlock);
    document.body.addEventListener('keydown', unlock);
  }

  if (!window.AWPF) {
    console.error('AudioWorklet polyfill not loaded');
    return null;
  }

  await window.AWPF.polyfill(actx);

  // Load WAM controller scripts (only if not already loaded)
  console.log('🔍 Checking for WAM scripts...');
  console.log('WAMController exists:', !!(window as any).WAMController);
  console.log('TemplateProjectController exists:', !!(window as any).TemplateProjectController);
  
  try {
    if (!(window as any).WAMController) {
      console.log('📥 Loading wam-controller.js...');
      await loadScript('scripts/wam-controller.js');
    } else {
      console.log('✅ wam-controller.js already loaded');
    }
    
    if (!(window as any).TemplateProjectController) {
      console.log('📥 Loading TemplateProject-awn.js...');
      await loadScript('scripts/TemplateProject-awn.js');
    } else {
      console.log('✅ TemplateProject-awn.js already loaded');
    }
  } catch (error) {
    console.error('❌ Error loading WAM scripts:', error);
    return null;
  }

  if (window.AWPF.isAudioWorkletPolyfilled) {
    console.log('AudioWorklet NOT Supported');
  } else {
    console.log('AudioWorklet Supported');
  }

  if (!window.TemplateProjectController) {
    console.error('TemplateProjectController not found after loading scripts');
    console.error('Available globals:', Object.keys(window).filter(k => k.includes('Template') || k.includes('WAM')));
    return null;
  }

  const inputBuses = [2];
  const outputBuses = [2];
  const options: any = {
    numberOfInputs: inputBuses.length,
    inputChannelCount: inputBuses,
    numberOfOutputs: outputBuses.length,
    outputChannelCount: outputBuses,
    processorOptions: { inputChannelCount: inputBuses }
  };

  await window.TemplateProjectController.importScripts(actx);
  const wamController = new window.TemplateProjectController(actx, options);

  // Initialize WebView-to-WAM adapter
  if (window.initWebViewWAMAdapter) {
    window.initWebViewWAMAdapter(wamController);
    console.log('WebView-to-WAM adapter initialized');
  }

  // Setup message handlers
  if (window.setupWAMMessageHandlers) {
    window.setupWAMMessageHandlers(wamController);
  }

  // Connect audio
  const nInputs = window.AWPF.isAudioWorkletPolyfilled 
    ? wamController.input?.numberOfInputs ?? 0
    : wamController.numberOfInputs ?? 0;

  if (nInputs > 0) {
    try {
      const stream = await navigator.mediaDevices.getUserMedia({ audio: true, video: false });
      const audioSource = actx.createMediaStreamSource(stream);
      
      if (window.AWPF.isAudioWorkletPolyfilled && wamController.input) {
        audioSource.connect(wamController.input);
      } else {
        audioSource.connect(wamController as unknown as AudioNode);
      }
      
      wamController.connect(actx.destination);
    } catch (err) {
      console.error('Error initializing user media stream:', err);
      wamController.connect(actx.destination);
    }
  } else {
    wamController.connect(actx.destination);
  }

  // Initialize default parameter values
  initializeDefaultParameters();

  return wamController;
}

/**
 * Load a script dynamically (only if not already loaded)
 */
function loadScript(src: string): Promise<void> {
  return new Promise((resolve, reject) => {
    // Check if script is already loaded by checking for global variables FIRST
    if (src.includes('wam-controller.js') && (window as any).WAMController) {
      console.log(`✅ ${src} already loaded (WAMController found)`);
      resolve();
      return;
    }
    if (src.includes('TemplateProject-awn.js') && (window as any).TemplateProjectController) {
      console.log(`✅ ${src} already loaded (TemplateProjectController found)`);
      resolve();
      return;
    }

    // Check if script tag already exists
    const existingScript = document.querySelector(`script[src="${src}"]`);
    if (existingScript) {
      console.log(`⏳ ${src} script tag exists, waiting for it to load...`);
      // Wait for it to load
      let attempts = 0;
      const maxAttempts = 100; // 5 seconds max (50ms * 100)
      const checkInterval = setInterval(() => {
        attempts++;
        if (src.includes('wam-controller.js') && (window as any).WAMController) {
          clearInterval(checkInterval);
          console.log(`✅ ${src} loaded successfully`);
          resolve();
        } else if (src.includes('TemplateProject-awn.js') && (window as any).TemplateProjectController) {
          clearInterval(checkInterval);
          console.log(`✅ ${src} loaded successfully`);
          resolve();
        } else if (attempts >= maxAttempts) {
          clearInterval(checkInterval);
          console.error(`❌ ${src} did not load within timeout`);
          reject(new Error(`Script ${src} did not load within timeout`));
        }
      }, 50);
      return;
    }

    console.log(`📥 Loading ${src}...`);
    const script = document.createElement('script');
    script.src = src;
    script.onload = () => {
      // Double-check the global is available
      if (src.includes('wam-controller.js') && !(window as any).WAMController) {
        reject(new Error(`Script ${src} loaded but WAMController not found`));
        return;
      }
      if (src.includes('TemplateProject-awn.js') && !(window as any).TemplateProjectController) {
        reject(new Error(`Script ${src} loaded but TemplateProjectController not found`));
        return;
      }
      console.log(`✅ ${src} loaded successfully`);
      resolve();
    };
    script.onerror = () => {
      console.error(`❌ Failed to load script: ${src}`);
      reject(new Error(`Failed to load script: ${src}`));
    };
    document.head.appendChild(script);
  });
}

/**
 * Initialize default parameter values
 */
function initializeDefaultParameters(): void {
  sendParameterValue(EParams.kParamGain, 1.0); // 100%
  sendParameterValue(EParams.kParamAttack, 0.01); // ~10ms
  sendParameterValue(EParams.kParamDecay, 0.01); // ~10ms
  sendParameterValue(EParams.kParamSustain, 0.5); // 50%
  sendParameterValue(EParams.kParamRelease, 0.01); // ~10ms
  sendParameterValue(EParams.kParamOsc1Mix, 1.0); // 100%
  sendParameterValue(EParams.kParamOsc2Mix, 0.0); // 0%
  sendParameterEnum(EParams.kParamOsc1Wave, 0); // Sine
  sendParameterEnum(EParams.kParamOsc2Wave, 0); // Sine
  sendParameterValue(EParams.kParamReverbDry, 1.0); // 100%
  sendParameterValue(EParams.kParamReverbWet, 0.0); // 0%
}

/**
 * Setup MIDI input/output dropdowns
 */
export function setupMIDIDevices(): void {
  if (!navigator.requestMIDIAccess) return;

  navigator.requestMIDIAccess().then((midiIF) => {
    setupMIDIInput(midiIF);
    setupMIDIOutput(midiIF);
  });
}

function setupMIDIInput(midiIF: MIDIAccess): void {
  const select = document.getElementById('midiInSelect') as HTMLSelectElement;
  if (!select) return;

  select.options.length = 0;

  for (const input of midiIF.inputs.values()) {
    const option = new Option(input.name || 'Unnamed');
    (option as any).port = input;
    select.appendChild(option);
  }

  select.onchange = (e) => {
    const target = e.target as HTMLSelectElement;
    const port = (target.options[target.selectedIndex] as any).port;
    if (window.TemplateProject_WAM) {
      (window.TemplateProject_WAM as any).midiIn = port;
    }
  };

  if (select.options.length > 0) {
    select.removeAttribute('disabled');
    const changeEvent = { target: select } as any;
    if (select.onchange) {
      select.onchange(changeEvent);
    }
  }
}

function setupMIDIOutput(midiIF: MIDIAccess): void {
  const select = document.getElementById('midiOutSelect') as HTMLSelectElement;
  if (!select) return;

  select.options.length = 0;

  for (const output of midiIF.outputs.values()) {
    const option = new Option(output.name || 'Unnamed');
    (option as any).port = output;
    select.appendChild(option);
  }

  select.onchange = (e) => {
    const target = e.target as HTMLSelectElement;
    const port = (target.options[target.selectedIndex] as any).port;
    if (window.TemplateProject_WAM) {
      (window.TemplateProject_WAM as any).midiOut = port;
    }
  };

  if (select.options.length > 0) {
    select.removeAttribute('disabled');
    const changeEvent = { target: select } as any;
    if (select.onchange) {
      select.onchange(changeEvent);
    }
  }
}

