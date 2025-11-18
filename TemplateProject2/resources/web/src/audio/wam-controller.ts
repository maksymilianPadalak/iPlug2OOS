/**
 * WAM (Web Audio Module) controller initialization and management
 */

import { WAMController } from '../types/wam';
import { detectEnvironment } from '../utils/environment';
import { sendParameterValue, sendParameterEnum } from '../communication/iplug-bridge';
import { EParams } from '../config/constants';
import { TestToneGenerator } from './test-tone';

// Global references for test tone management
let globalTestTone: TestToneGenerator | null = null;
let globalWAMController: (WAMController & { audioContext: AudioContext; mediaStream?: MediaStream; audioSource?: MediaStreamAudioSourceNode }) | null = null;

/**
 * Initialize WAM controller
 */
export async function initializeWAM(): Promise<(WAMController & { audioContext: AudioContext }) | null> {
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
  console.log('TemplateProject2Controller exists:', !!(window as any).TemplateProject2Controller);
  
  try {
    if (!(window as any).WAMController) {
      console.log('📥 Loading wam-controller.js...');
      await loadScript('scripts/wam-controller.js');
    } else {
      console.log('✅ wam-controller.js already loaded');
    }
    
    if (!(window as any).TemplateProject2Controller) {
      console.log('📥 Loading TemplateProject2-awn.js...');
      await loadScript('scripts/TemplateProject2-awn.js');
    } else {
      console.log('✅ TemplateProject2-awn.js already loaded');
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

  if (!window.TemplateProject2Controller) {
    console.error('TemplateProject2Controller not found after loading scripts');
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

  await window.TemplateProject2Controller.importScripts(actx);
  console.log('📦 TemplateProject2Controller.importScripts completed');

  const wamController = new window.TemplateProject2Controller(actx, options);
  console.log('🎛️  WAM Controller created:', wamController);
  console.log('   Controller type:', wamController.constructor.name);
  console.log('   Has connect method:', typeof wamController.connect === 'function');
  console.log('   Has disconnect method:', typeof (wamController as any).disconnect === 'function');
  console.log('   numberOfInputs:', wamController.numberOfInputs);
  console.log('   numberOfOutputs:', wamController.numberOfOutputs);
  console.log('   channelCount:', (wamController as any).channelCount);
  console.log('   channelCountMode:', (wamController as any).channelCountMode);
  console.log('   channelInterpretation:', (wamController as any).channelInterpretation);

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
  console.log('🔊 Connecting WAM controller to audio destination...');
  console.log('   AudioContext state:', actx.state);
  console.log('   AudioWorklet polyfilled:', window.AWPF.isAudioWorkletPolyfilled);

  const nInputs = window.AWPF.isAudioWorkletPolyfilled
    ? wamController.input?.numberOfInputs ?? 0
    : wamController.numberOfInputs ?? 0;

  console.log('   WAM controller inputs:', nInputs);

  // Store references for test tone management
  let mediaStream: MediaStream | null = null;
  let audioSource: MediaStreamAudioSourceNode | null = null;

  if (nInputs > 0) {
    try {
      console.log('🎤 Requesting microphone access...');
      console.log('   nInputs:', nInputs);
      console.log('   navigator.mediaDevices exists:', !!navigator.mediaDevices);
      console.log('   getUserMedia exists:', !!navigator.mediaDevices?.getUserMedia);
      console.log('   window.isSecureContext:', window.isSecureContext);
      console.log('   window.location.protocol:', window.location.protocol);

      mediaStream = await navigator.mediaDevices.getUserMedia({ audio: true, video: false });
      console.log('✅ Microphone access granted:', !!mediaStream);
      audioSource = actx.createMediaStreamSource(mediaStream);

      if (window.AWPF.isAudioWorkletPolyfilled && wamController.input) {
        console.log('   Connecting microphone to WAM input (polyfilled mode)');
        audioSource.connect(wamController.input);
      } else {
        console.log('   Connecting microphone to WAM directly (native mode)');
        audioSource.connect(wamController as unknown as AudioNode);
      }

      // Store references on controller for test tone management
      (wamController as any).mediaStream = mediaStream;
      (wamController as any).audioSource = audioSource;

      console.log('   Connecting WAM controller to destination...');
      console.log('   actx.destination:', actx.destination);
      console.log('   actx.destination type:', actx.destination.constructor.name);

      // Verify WAM controller is an AudioNode
      console.log('   WAM is AudioNode:', wamController instanceof AudioNode);
      console.log('   WAM context:', (wamController as any).context);

      wamController.connect(actx.destination);
      console.log('✅ WAM controller connected to audio destination successfully');
      console.log('   Full audio path: Microphone → WAM Controller Input → WAM DSP Processor → AudioContext.destination → Speakers');

      // Verify connection state
      console.log('   Verifying audio graph connections:');
      console.log('   - Microphone source exists:', !!audioSource);
      console.log('   - Microphone connected to:', window.AWPF?.isAudioWorkletPolyfilled ? 'WAM.input' : 'WAM controller');
      console.log('   - WAM connected to: actx.destination');
      console.log('   - AudioContext state:', actx.state);
    } catch (err) {
      console.error('❌ Error initializing user media stream:', err);
      console.error('   Error name:', (err as Error).name);
      console.error('   Error message:', (err as Error).message);
      console.error('   This likely means getUserMedia failed or was denied');
      console.log('   Connecting WAM to destination without microphone input...');
      wamController.connect(actx.destination);
      console.log('✅ WAM controller connected (no microphone)');
    }
  } else {
    console.log('   No inputs needed, connecting WAM directly to destination...');
    wamController.connect(actx.destination);
    console.log('✅ WAM controller connected to audio destination');
  }

  // Initialize default parameter values
  initializeDefaultParameters();

  // Attach audio context to controller for test tone access
  (wamController as any).audioContext = actx;

  // Store global reference for test tone management
  globalWAMController = wamController as WAMController & { audioContext: AudioContext; mediaStream?: MediaStream; audioSource?: MediaStreamAudioSourceNode };

  // Initialize test tone generator
  globalTestTone = new TestToneGenerator(actx);

  return globalWAMController;
}

/**
 * Toggle test tone on/off
 * For now, this just ensures microphone input is active for testing
 */
export async function toggleTestTone(enable: boolean): Promise<void> {
  if (!globalWAMController) {
    console.error('WAM controller not initialized');
    return;
  }

  console.log('🎚️  Microphone input mode:', enable ? 'ACTIVE' : 'INACTIVE');
  console.log('   WAM controller exists:', !!globalWAMController);
  console.log('   Audio source exists:', !!globalWAMController.audioSource);

  const targetNode = window.AWPF?.isAudioWorkletPolyfilled && globalWAMController.input
    ? globalWAMController.input
    : (globalWAMController as unknown as AudioNode);

  console.log('   Target node type:', window.AWPF?.isAudioWorkletPolyfilled ? 'WAM input (polyfilled)' : 'WAM controller (native)');
  console.log('   Target node exists:', !!targetNode);

  if (enable) {
    // Ensure microphone is connected
    if (globalWAMController.audioSource) {
      try {
        // Disconnect first to avoid double-connection
        globalWAMController.audioSource.disconnect();

        // TEMPORARY DIAGNOSTIC: Connect mic directly to destination to verify mic works
        const audioContext = (globalWAMController as any).audioContext;
        console.log('🔬 DIAGNOSTIC: Connecting microphone DIRECTLY to speakers (bypass WAM)');
        globalWAMController.audioSource.connect(audioContext.destination);
        console.log('   If you hear your voice now, the microphone works but WAM routing is broken');
        console.log('   If you still hear nothing, the microphone input itself has an issue');

        // Reconnect to WAM input (this will be in parallel with direct connection for testing)
        globalWAMController.audioSource.connect(targetNode);
        console.log('🎤 Microphone connected to BOTH WAM input AND direct output');
        console.log('   You should hear your voice (possibly louder from direct connection)');
      } catch (error) {
        console.error('Could not connect audio source:', error);
      }
    } else {
      console.warn('⚠️  No microphone input available');
    }
  } else {
    // Disconnect microphone
    if (globalWAMController.audioSource) {
      try {
        globalWAMController.audioSource.disconnect();
        console.log('🔇 Microphone disconnected');
      } catch (error) {
        console.warn('Could not disconnect audio source:', error);
      }
    }
  }
}

/**
 * Check if test tone is currently playing
 */
export function isTestTonePlaying(): boolean {
  return globalTestTone?.getIsPlaying() ?? false;
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
    if (src.includes('TemplateProject2-awn.js') && (window as any).TemplateProject2Controller) {
      console.log(`✅ ${src} already loaded (TemplateProject2Controller found)`);
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
        } else if (src.includes('TemplateProject2-awn.js') && (window as any).TemplateProject2Controller) {
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
      if (src.includes('TemplateProject2-awn.js') && !(window as any).TemplateProject2Controller) {
        reject(new Error(`Script ${src} loaded but TemplateProject2Controller not found`));
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
  sendParameterValue(EParams.kParamGain, 1.0); // 100% (normalized: 100/200 = 0.5)
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
    if (window.TemplateProject2_WAM) {
      (window.TemplateProject2_WAM as any).midiIn = port;
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
    if (window.TemplateProject2_WAM) {
      (window.TemplateProject2_WAM as any).midiOut = port;
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

