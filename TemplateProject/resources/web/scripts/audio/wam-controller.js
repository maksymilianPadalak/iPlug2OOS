/**
 * WAM (Web Audio Module) controller initialization and management
 */
import { sendParameterValue, sendParameterEnum } from '../communication/iplug-bridge';
import { EParams } from '../config/constants';
/**
 * Initialize WAM controller
 */
export async function initializeWAM() {
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
    // Load WAM controller scripts
    await loadScript('scripts/wam-controller.js');
    await loadScript('scripts/TemplateProject-awn.js');
    if (window.AWPF.isAudioWorkletPolyfilled) {
        console.log('AudioWorklet NOT Supported');
    }
    else {
        console.log('AudioWorklet Supported');
    }
    if (!window.TemplateProjectController) {
        console.error('TemplateProjectController not found');
        return null;
    }
    const inputBuses = [2];
    const outputBuses = [2];
    const options = {
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
            }
            else {
                audioSource.connect(wamController);
            }
            wamController.connect(actx.destination);
        }
        catch (err) {
            console.error('Error initializing user media stream:', err);
            wamController.connect(actx.destination);
        }
    }
    else {
        wamController.connect(actx.destination);
    }
    // Initialize default parameter values
    initializeDefaultParameters();
    return wamController;
}
/**
 * Load a script dynamically
 */
function loadScript(src) {
    return new Promise((resolve, reject) => {
        const script = document.createElement('script');
        script.src = src;
        script.onload = () => resolve();
        script.onerror = () => reject(new Error(`Failed to load script: ${src}`));
        document.head.appendChild(script);
    });
}
/**
 * Initialize default parameter values
 */
function initializeDefaultParameters() {
    sendParameterValue(EParams.kParamGain, 1.0); // 100%
    sendParameterValue(EParams.kParamNoteGlideTime, 0.0); // 0ms
    sendParameterValue(EParams.kParamAttack, 0.01); // ~10ms
    sendParameterValue(EParams.kParamDecay, 0.01); // ~10ms
    sendParameterValue(EParams.kParamSustain, 0.5); // 50%
    sendParameterValue(EParams.kParamRelease, 0.01); // ~10ms
    sendParameterEnum(EParams.kParamLFOShape, 0); // Triangle
    sendParameterValue(EParams.kParamLFORateHz, 0.025); // ~1Hz
    sendParameterEnum(EParams.kParamLFORateTempo, 11); // 1/1
    sendParameterValue(EParams.kParamLFORateMode, 1.0); // Sync enabled
    sendParameterValue(EParams.kParamLFODepth, 0.0); // 0%
}
/**
 * Setup MIDI input/output dropdowns
 */
export function setupMIDIDevices() {
    if (!navigator.requestMIDIAccess)
        return;
    navigator.requestMIDIAccess().then((midiIF) => {
        setupMIDIInput(midiIF);
        setupMIDIOutput(midiIF);
    });
}
function setupMIDIInput(midiIF) {
    const select = document.getElementById('midiInSelect');
    if (!select)
        return;
    select.options.length = 0;
    for (const input of midiIF.inputs.values()) {
        const option = new Option(input.name || 'Unnamed');
        option.port = input;
        select.appendChild(option);
    }
    select.onchange = (e) => {
        const target = e.target;
        const port = target.options[target.selectedIndex].port;
        if (window.TemplateProject_WAM) {
            window.TemplateProject_WAM.midiIn = port;
        }
    };
    if (select.options.length > 0) {
        select.removeAttribute('disabled');
        const changeEvent = { target: select };
        if (select.onchange) {
            select.onchange(changeEvent);
        }
    }
}
function setupMIDIOutput(midiIF) {
    const select = document.getElementById('midiOutSelect');
    if (!select)
        return;
    select.options.length = 0;
    for (const output of midiIF.outputs.values()) {
        const option = new Option(output.name || 'Unnamed');
        option.port = output;
        select.appendChild(option);
    }
    select.onchange = (e) => {
        const target = e.target;
        const port = target.options[target.selectedIndex].port;
        if (window.TemplateProject_WAM) {
            window.TemplateProject_WAM.midiOut = port;
        }
    };
    if (select.options.length > 0) {
        select.removeAttribute('disabled');
        const changeEvent = { target: select };
        if (select.onchange) {
            select.onchange(changeEvent);
        }
    }
}
//# sourceMappingURL=wam-controller.js.map