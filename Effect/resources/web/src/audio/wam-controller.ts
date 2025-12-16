/**
 * WAM (Web Audio Module) controller initialization and management
 * Effect Template - supports audio file input for testing effects
 */

import { WAMController } from '@/types/wam';

// Global references for audio management
let globalWAMController: (WAMController & {
  audioContext: AudioContext;
  mediaStream?: MediaStream;
  audioSource?: MediaStreamAudioSourceNode;
  audioBufferSource?: AudioBufferSourceNode;
  loadedAudioBuffer?: AudioBuffer;
  loadedAudioFileName?: string;
  audioFileGainNode?: GainNode;
  channelMerger?: ChannelMergerNode;
}) | null = null;

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
  console.log('PluginController exists:', !!(window as any).PluginController);

  try {
    if (!(window as any).WAMController) {
      console.log('📥 Loading wam-controller.js...');
      await loadScript('scripts/wam-controller.js');
    } else {
      console.log('✅ wam-controller.js already loaded');
    }

    if (!(window as any).PluginController) {
      console.log('📥 Loading Plugin-awn.js...');
      await loadScript('scripts/Plugin-awn.js');
    } else {
      console.log('✅ Plugin-awn.js already loaded');
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

  if (!window.PluginController) {
    console.error('PluginController not found after loading scripts');
    console.error('Available globals:', Object.keys(window).filter(k => k.includes('Effect') || k.includes('WAM')));
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

  await window.PluginController.importScripts(actx);
  console.log('📦 PluginController.importScripts completed');

  const wamController = new window.PluginController(actx, options);
  console.log('🎛️  WAM Controller created:', wamController);

  // Initialize WebView-to-WAM adapter
  if (window.initWebViewWAMAdapter) {
    window.initWebViewWAMAdapter(wamController);
    console.log('WebView-to-WAM adapter initialized');
  }

  // Setup message handlers
  if (window.setupWAMMessageHandlers) {
    window.setupWAMMessageHandlers(wamController);
  }

  // Connect WAM to destination (audio input will be connected later)
  console.log('🔊 Connecting WAM controller to audio destination...');
  wamController.connect(actx.destination);
  console.log('✅ WAM controller connected to audio destination');

  // Attach audio context to controller
  (wamController as any).audioContext = actx;

  // Store global reference
  globalWAMController = wamController as WAMController & { audioContext: AudioContext };

  return globalWAMController;
}

/**
 * Load an audio file (without playing)
 */
export async function loadAudioFile(file: File): Promise<void> {
  if (!globalWAMController) {
    console.error('WAM controller not initialized');
    throw new Error('WAM controller not initialized');
  }

  const actx = globalWAMController.audioContext;

  try {
    // Read file as array buffer
    const arrayBuffer = await file.arrayBuffer();

    // Decode audio data
    const audioBuffer = await actx.decodeAudioData(arrayBuffer);
    console.log('✅ Audio file decoded:', {
      duration: audioBuffer.duration,
      sampleRate: audioBuffer.sampleRate,
      numberOfChannels: audioBuffer.numberOfChannels,
    });

    // Store the buffer and filename for later playback
    (globalWAMController as any).loadedAudioBuffer = audioBuffer;
    (globalWAMController as any).loadedAudioFileName = file.name;
    console.log('📁 Audio file loaded (ready to play):', file.name);

  } catch (error) {
    console.error('❌ Error loading audio file:', error);
    throw error;
  }
}

/**
 * Play the loaded audio file
 */
export async function playLoadedAudioFile(): Promise<void> {
  if (!globalWAMController) {
    console.error('WAM controller not initialized');
    throw new Error('WAM controller not initialized');
  }

  const actx = globalWAMController.audioContext;
  const audioBuffer = (globalWAMController as any).loadedAudioBuffer;

  if (!audioBuffer) {
    throw new Error('No audio file loaded. Please select a file first.');
  }

  // Ensure audio context is running
  if (actx.state === 'suspended') {
    console.log('Resuming audio context...');
    await actx.resume();
  }

  try {
    // Disconnect any existing audio source
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
    finalOutput.connect(targetNode);
    console.log('✅ Audio file connected to WAM input');

    // Start playback
    source.start(0);
    console.log('🎵 Audio file playback started');

    // Store references
    (globalWAMController as any).audioBufferSource = source;
    (globalWAMController as any).audioFileGainNode = gainNode;

  } catch (error) {
    console.error('❌ Error playing audio file:', error);
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
    if (src.includes('Plugin-awn.js') && (window as any).PluginController) {
      console.log(`✅ ${src} already loaded (PluginController found)`);
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
        } else if (src.includes('Plugin-awn.js') && (window as any).PluginController) {
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
      if (src.includes('Plugin-awn.js') && !(window as any).PluginController) {
        reject(new Error(`Script ${src} loaded but PluginController not found`));
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
