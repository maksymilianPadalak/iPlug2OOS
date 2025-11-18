/**
 * WAM (Web Audio Module) controller initialization and management
 */

import { WAMController } from '../types/wam';
import { sendParameterValue } from '../communication/iplug-bridge';
import { EParams } from '../config/constants';
import { TestToneGenerator } from './test-tone';

// Global references for test tone management
let globalTestTone: TestToneGenerator | null = null;
let globalWAMController: (WAMController & { audioContext: AudioContext; mediaStream?: MediaStream; audioSource?: MediaStreamAudioSourceNode; audioBufferSource?: AudioBufferSourceNode }) | null = null;

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

  // Don't request microphone access automatically - wait for user to click button
  console.log('   Microphone will be requested when user clicks MIC ON button');

  // Connect WAM to destination (microphone will be connected later via toggleTestTone)
  wamController.connect(actx.destination);
  console.log('✅ WAM controller connected to audio destination');

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
 * Toggle microphone input on/off
 */
export async function toggleTestTone(enable: boolean): Promise<void> {
  if (!globalWAMController) {
    console.error('WAM controller not initialized');
    return;
  }

  console.log('🎚️  Microphone input mode:', enable ? 'ACTIVE' : 'INACTIVE');

  const targetNode = window.AWPF?.isAudioWorkletPolyfilled && globalWAMController.input
    ? globalWAMController.input
    : (globalWAMController as unknown as AudioNode);

  if (enable) {
    // Stop audio file if playing
    if (globalWAMController.audioBufferSource) {
      stopAudioFile();
    }

    // Request microphone access if not already obtained
    if (!globalWAMController.audioSource) {
      try {
        console.log('🎤 Requesting microphone access...');
        const mediaStream = await navigator.mediaDevices.getUserMedia({ audio: true, video: false });
        console.log('✅ Microphone access granted');

        const audioSource = globalWAMController.audioContext.createMediaStreamSource(mediaStream);

        // Store references
        (globalWAMController as any).mediaStream = mediaStream;
        (globalWAMController as any).audioSource = audioSource;
      } catch (error) {
        console.error('❌ Failed to get microphone access:', error);
        throw error;
      }
    }

    // Connect microphone to WAM input
    if (globalWAMController.audioSource) {
      try {
        // Disconnect first to avoid double-connection
        globalWAMController.audioSource.disconnect();

        // Connect to WAM input for processing
        globalWAMController.audioSource.connect(targetNode);
        console.log('🎤 Microphone connected to WAM input for processing');
      } catch (error) {
        console.error('Could not connect audio source:', error);
      }
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
 * Load and play an audio file
 */
export async function loadAndPlayAudioFile(file: File): Promise<void> {
  if (!globalWAMController) {
    console.error('WAM controller not initialized');
    throw new Error('WAM controller not initialized');
  }

  const actx = globalWAMController.audioContext;

  // Ensure audio context is running
  if (actx.state === 'suspended') {
    console.log('Resuming audio context...');
    await actx.resume();
  }

  try {
    // Disconnect any existing audio source (mic or previous file)
    if (globalWAMController.audioSource) {
      try {
        globalWAMController.audioSource.disconnect();
      } catch (e) {
        console.warn('Error disconnecting existing audio source:', e);
      }
    }
    if (globalWAMController.audioBufferSource) {
      try {
        globalWAMController.audioBufferSource.stop();
        globalWAMController.audioBufferSource.disconnect();
      } catch (e) {
        console.warn('Error stopping existing audio buffer source:', e);
      }
    }
    if ((globalWAMController as any).audioFileGainNode) {
      try {
        (globalWAMController as any).audioFileGainNode.disconnect();
      } catch (e) {
        console.warn('Error disconnecting audio file gain node:', e);
      }
    }
    if ((globalWAMController as any).channelMerger) {
      try {
        (globalWAMController as any).channelMerger.disconnect();
      } catch (e) {
        console.warn('Error disconnecting channel merger:', e);
      }
    }

    // Read file as array buffer
    const arrayBuffer = await file.arrayBuffer();
    
    // Decode audio data
    const audioBuffer = await actx.decodeAudioData(arrayBuffer);
    console.log('✅ Audio file decoded:', {
      duration: audioBuffer.duration,
      sampleRate: audioBuffer.sampleRate,
      numberOfChannels: audioBuffer.numberOfChannels,
    });

    // Create buffer source node
    const source = actx.createBufferSource();
    source.buffer = audioBuffer;
    source.loop = true; // Loop the audio file

    // Create gain node for volume control
    const gainNode = actx.createGain();
    gainNode.gain.value = 1.0;

    // Connect source to gain
    source.connect(gainNode);

    // Handle channel conversion: ensure stereo output
    let finalOutput: AudioNode = gainNode;
    
    if (audioBuffer.numberOfChannels === 1) {
      // Mono file: split to stereo using ChannelMerger
      console.log('Converting mono to stereo...');
      const merger = actx.createChannelMerger(2);
      gainNode.connect(merger, 0, 0); // Left channel
      gainNode.connect(merger, 0, 1); // Right channel
      finalOutput = merger;
      (globalWAMController as any).channelMerger = merger;
    } else if (audioBuffer.numberOfChannels === 2) {
      // Already stereo, use gain node directly
      console.log('Audio file is already stereo');
    } else {
      // Multi-channel: take first two channels
      console.log(`Audio file has ${audioBuffer.numberOfChannels} channels, using first 2`);
      const splitter = actx.createChannelSplitter(2);
      const merger = actx.createChannelMerger(2);
      gainNode.connect(splitter);
      splitter.connect(merger, 0, 0); // Left
      splitter.connect(merger, 1, 1); // Right
      finalOutput = merger;
      (globalWAMController as any).channelMerger = merger;
    }
    
    // Connect to WAM input
    const targetNode = window.AWPF?.isAudioWorkletPolyfilled && globalWAMController.input
      ? globalWAMController.input
      : (globalWAMController as unknown as AudioNode);

    console.log('Connecting audio file to WAM input...');
    console.log('Target node:', targetNode);
    console.log('Target node type:', window.AWPF?.isAudioWorkletPolyfilled ? 'WAM.input (polyfill)' : 'WAM controller (native)');
    
    finalOutput.connect(targetNode);
    console.log('✅ Audio file connected to WAM input');

    // Start playback
    source.start(0);
    console.log('🎵 Audio file playback started');

    // Store references
    (globalWAMController as any).audioBufferSource = source;
    (globalWAMController as any).audioFileGainNode = gainNode;
    (globalWAMController as any).currentAudioFile = file.name;

  } catch (error) {
    console.error('❌ Error loading/playing audio file:', error);
    console.error('Error details:', {
      name: (error as Error).name,
      message: (error as Error).message,
      stack: (error as Error).stack,
    });
    throw error;
  }
}

/**
 * Stop audio file playback
 */
export function stopAudioFile(): void {
  if (!globalWAMController) {
    return;
  }

  if (globalWAMController.audioBufferSource) {
    try {
      globalWAMController.audioBufferSource.stop();
      globalWAMController.audioBufferSource.disconnect();
      (globalWAMController as any).audioBufferSource = null;
      console.log('🛑 Audio file playback stopped');
    } catch (e) {
      console.warn('Error stopping audio buffer source:', e);
    }
  }

  if ((globalWAMController as any).audioFileGainNode) {
    try {
      (globalWAMController as any).audioFileGainNode.disconnect();
      (globalWAMController as any).audioFileGainNode = null;
    } catch (e) {
      console.warn('Error disconnecting audio file gain node:', e);
    }
  }

  if ((globalWAMController as any).channelMerger) {
    try {
      (globalWAMController as any).channelMerger.disconnect();
      (globalWAMController as any).channelMerger = null;
    } catch (e) {
      console.warn('Error disconnecting channel merger:', e);
    }
  }
}

/**
 * Check if audio file is currently playing
 */
export function isAudioFilePlaying(): boolean {
  return !!(globalWAMController?.audioBufferSource);
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

