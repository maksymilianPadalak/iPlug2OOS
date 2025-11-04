/* WebView-to-WAM Adapter
 * Bridges WebView communication pattern (IPlugSendMsg) to WAMController API
 * Works universally for both WebView (AU/VST3) and WAM (Web) environments
 */

let WAMControllerInstance = null;
let isWebViewMode = false;
let _originalIPlugSendMsg = null;

/**
 * Detect the runtime environment
 * @returns {string} 'webview' or 'wam'
 */
function detectEnvironment() {
  // Check if IPlugSendMsg already exists (injected by platform WebView)
  if (typeof window.IPlugSendMsg === 'function' &&
      window.IPlugSendMsg.toString().includes('webkit.messageHandlers') ||
      window.IPlugSendMsg.toString().includes('chrome.webview')) {
    return 'webview';
  }

  // Check if we're in a browser environment (WAM)
  if (typeof window !== 'undefined' && typeof AudioWorkletNode !== 'undefined') {
    return 'wam';
  }

  // Default to webview if uncertain
  return 'webview';
}

/**
 * Initialize the adapter when WAM controller is ready
 * @param {WAMController} wamController - The WAM controller instance
 */
function initWebViewWAMAdapter(wamController) {
  WAMControllerInstance = wamController;
  const env = detectEnvironment();

  if (env === 'webview') {
    // In WebView mode, IPlugSendMsg is already injected by platform
    // Don't override it, just store reference
    isWebViewMode = true;
    _originalIPlugSendMsg = window.IPlugSendMsg;
    console.log("WebView-to-WAM adapter: WebView mode detected, using native IPlugSendMsg");
    return;
  }

  // In WAM mode, override IPlugSendMsg to route to WAMController
  isWebViewMode = false;
  _originalIPlugSendMsg = window.IPlugSendMsg; // May be undefined

  window.IPlugSendMsg = function(message) {
    if (!WAMControllerInstance) {
      console.warn("WAMController not initialized yet");
      return;
    }

    const msg = message.msg;

    switch(msg) {
      case "SPVFUI": // Send Parameter Value From UI
        console.log("SPVFUI: paramIdx=" + message.paramIdx + ", value=" + message.value);
        // Use WAM's built-in setParam method instead of custom message
        if (typeof WAMControllerInstance.setParam === 'function') {
          WAMControllerInstance.setParam(message.paramIdx, message.value);
        } else {
          console.warn("setParam not available, falling back to sendMessage");
          WAMControllerInstance.sendMessage("SPVFUI", String(message.paramIdx), message.value);
        }
        break;

      case "BPCFUI": // Begin Parameter Change From UI
        // Send as arbitrary message - processor can handle if needed
        const bpcProp = `${-1}:${message.paramIdx}`;
        WAMControllerInstance.sendMessage("SAMFUI", bpcProp, new Uint8Array([0]));
        break;

      case "EPCFUI": // End Parameter Change From UI
        // Send as arbitrary message - processor can handle if needed
        const epcProp = `${-2}:${message.paramIdx}`;
        WAMControllerInstance.sendMessage("SAMFUI", epcProp, new Uint8Array([0]));
        break;

      case "SAMFUI": // Send Arbitrary Message From UI
        const msgTag = message.msgTag || -1;
        const ctrlTag = message.ctrlTag || -1;
        const prop = `${msgTag}:${ctrlTag}`;

        let data = new Uint8Array(0);
        if (message.data) {
          if (typeof message.data === 'string') {
            // Base64 decode if string
            const binaryString = atob(message.data);
            data = new Uint8Array(binaryString.length);
            for (let i = 0; i < binaryString.length; i++) {
              data[i] = binaryString.charCodeAt(i);
            }
          } else if (message.data instanceof ArrayBuffer) {
            data = new Uint8Array(message.data);
          } else if (Array.isArray(message.data)) {
            data = new Uint8Array(message.data);
          }
        }

        // For SAMFUI with binary data, we need to send via onMessageA
        // The WAM controller sendMessage only handles simple data
        // We'll encode as base64 and send as string, processor can decode
        if (data.length > 0) {
          const base64 = btoa(String.fromCharCode(...data));
          WAMControllerInstance.sendMessage("SAMFUI", prop + ":" + base64, data.length);
        } else {
          WAMControllerInstance.sendMessage("SAMFUI", prop, 0);
        }
        break;

      case "SMMFUI": // Send MIDI Message From UI
        const status = message.statusByte || message.status || 0;
        const data1 = message.dataByte1 || message.data1 || 0;
        const data2 = message.dataByte2 || message.data2 || 0;
        const midiProp = `${status}:${data1}:${data2}`;
        console.log("Sending MIDI:", status, data1, data2, "->", midiProp);
        WAMControllerInstance.sendMessage("SMMFUI", midiProp, 0);
        break;

      default:
        console.warn("Unknown message type:", msg);
    }
  };

  console.log("WebView-to-WAM adapter initialized in WAM mode");
}

// Export environment detection
window.getIPlugEnvironment = function() {
  return isWebViewMode ? 'webview' : 'wam';
};

/**
 * Setup message handlers to route WAMController messages to WebView JS functions
 * @param {WAMController} controller - The WAM controller instance
 */
function setupWAMMessageHandlers(controller) {
  const originalOnMessage = controller.onmessage.bind(controller);

  controller.onmessage = function(msg) {
    if (!msg || !msg.verb) {
      // Call original handler for non-standard messages
      if (originalOnMessage) {
        originalOnMessage(msg);
      }
      return;
    }

    switch(msg.verb) {
      case "SPVFD": // Send Parameter Value From Delegate
        console.log("SPVFD received: paramIdx=" + msg.prop + ", value=" + msg.data);
        if (typeof window.SPVFD === 'function') {
          window.SPVFD(parseInt(msg.prop), parseFloat(msg.data));
        } else {
          console.error("window.SPVFD is not a function!");
        }
        break;

      case "SCVFD": // Send Control Value From Delegate
        if (typeof window.SCVFD === 'function') {
          window.SCVFD(parseInt(msg.prop), parseFloat(msg.data));
        } else {
          console.error("window.SCVFD is not a function!");
        }
        break;

      case "SCMFD": // Send Control Message From Delegate
        if (typeof window.SCMFD === 'function') {
          const parts = msg.prop.split(":");
          const ctrlTag = parseInt(parts[0]);
          const msgTag = parseInt(parts[1]);

          let base64 = "";
          let dataSize = 0;

          if (msg.data) {
            if (msg.data instanceof ArrayBuffer) {
              // Convert ArrayBuffer to base64 safely
              const uint8Array = new Uint8Array(msg.data);
              // Use chunked approach for large arrays
              const chunks = [];
              const chunkSize = 8192;
              for (let i = 0; i < uint8Array.length; i += chunkSize) {
                chunks.push(String.fromCharCode.apply(null, uint8Array.slice(i, i + chunkSize)));
              }
              base64 = btoa(chunks.join(''));
              dataSize = msg.data.byteLength;
            } else if (Array.isArray(msg.data)) {
              base64 = btoa(String.fromCharCode(...msg.data));
              dataSize = msg.data.length;
            } else if (typeof msg.data === 'string') {
              // Data might already be base64
              base64 = msg.data;
              dataSize = Math.floor(base64.length * 3 / 4);
            }
          }

          window.SCMFD(ctrlTag, msgTag, dataSize, base64);
        } else {
          console.error("window.SCMFD is not a function!");
        }
        break;

      case "SAMFD": // Send Arbitrary Message From Delegate
        if (typeof window.SAMFD === 'function') {
          let base64 = "";
          if (msg.data) {
            if (msg.data instanceof ArrayBuffer) {
              const data = new Uint8Array(msg.data);
              base64 = btoa(String.fromCharCode(...data));
            } else if (Array.isArray(msg.data)) {
              base64 = btoa(String.fromCharCode(...msg.data));
            }
          }

          window.SAMFD(parseInt(msg.prop), base64.length ? (base64.length * 3 / 4) : 0, base64);
        }
        break;

      case "SMMFD": // Send MIDI Message From Delegate
        if (typeof window.SMMFD === 'function') {
          const parts = msg.prop.split(":");
          if (parts.length >= 3) {
            window.SMMFD(parseInt(parts[0]), parseInt(parts[1]), parseInt(parts[2]));
          }
        }
        break;

      case "StartIdleTimer":
        // Start idle timer for periodic updates
        if (typeof window.StartIdleTimer === 'function') {
          window.StartIdleTimer();
        }
        break;

      default:
        // Call original handler for unknown messages
        if (originalOnMessage) {
          originalOnMessage(msg);
        }
    }
  };

  console.log("WAM message handlers setup complete");
}

// Make functions globally available
window.initWebViewWAMAdapter = initWebViewWAMAdapter;
window.setupWAMMessageHandlers = setupWAMMessageHandlers;
