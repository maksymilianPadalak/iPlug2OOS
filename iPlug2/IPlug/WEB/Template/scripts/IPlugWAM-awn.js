/* Declares the NAME_PLACEHOLDER Audio Worklet Node */

class NAME_PLACEHOLDERController extends WAMController
{
  constructor (actx, options) {
    options = options || {};
    if (options.numberOfInputs === undefined)       options.numberOfInputs = 0;
    if (options.numberOfOutputs === undefined)      options.numberOfOutputs = 1;
    if (options.outputChannelCount === undefined)   options.outputChannelCount = [2];
    if (options.processorOptions.inputChannelCount === undefined) options.processorOptions = {inputChannelCount:[]};

    options.buflenSPN = 1024;
    super(actx, "NAME_PLACEHOLDER", options);

    // Setup WebView-to-WAM adapter if available
    if (typeof window.setupWAMMessageHandlers === 'function') {
      window.setupWAMMessageHandlers(this);
    }
  }

  static importScripts (actx) {
    var origin = "ORIGIN_PLACEHOLDER";

    return new Promise( (resolve) => {
      actx.audioWorklet.addModule(origin + "scripts/NAME_PLACEHOLDER-wam.js").then(() => {
      actx.audioWorklet.addModule(origin + "scripts/wam-processor.js").then(() => {
      actx.audioWorklet.addModule(origin + "scripts/NAME_PLACEHOLDER-awp.js").then(() => {
        resolve();
      }) }) });
    })
  }

  onmessage(msg) {
    //Received the WAM descriptor from the processor - could create an HTML UI here, based on descriptor
    if(msg.type == "descriptor") {
      console.log("got WAM descriptor...");
    }

    // If WebView adapter is set up, it will handle routing messages
    // Otherwise fall back to Module-based handlers (for backward compatibility)
    if (typeof window.setupWAMMessageHandlers === 'function') {
      // Adapter handles routing, but we still need to call the handler
      // The adapter replaces onmessage, so this won't be called for adapter-handled messages
      // Keep this for any custom handling needed
      return;
    }

    // Fallback: Module-based handlers (original behavior)
    //Send Parameter Value From Delegate
    if(msg.verb == "SPVFD") {
      if (typeof Module !== 'undefined' && Module.SPVFD) {
        Module.SPVFD(parseInt(msg.prop), parseFloat(msg.data));
      }
    }
    //Set Control Value From Delegate
    else if(msg.verb == "SCVFD") {
      if (typeof Module !== 'undefined' && Module.SCVFD) {
        Module.SCVFD(parseInt(msg.prop), parseFloat(msg.data));
      }
    }
    //Send Control Message From Delegate
    else if(msg.verb == "SCMFD") {
      if (typeof Module !== 'undefined' && Module.SCMFD) {
        var res = msg.prop.split(":");
        var data = new Uint8Array(msg.data);
        const buffer = Module._malloc(data.length);
        Module.HEAPU8.set(data, buffer);
        Module.SCMFD(parseInt(res[0]), parseInt(res[1]), data.length, buffer);
        Module._free(buffer);
      }
    }
    //Send Arbitrary Message From Delegate
    else if(msg.verb == "SAMFD") {
      if (typeof Module !== 'undefined' && Module.SAMFD) {
        var data = new Uint8Array(msg.data);
        const buffer = Module._malloc(data.length);
        Module.HEAPU8.set(data, buffer);
        Module.SAMFD(parseInt(msg.prop), data.length, buffer);
        Module._free(buffer);
      }
    }
    //Send MIDI Message From Delegate
    else if(msg.verb == "SMMFD") {
      if (typeof Module !== 'undefined' && Module.SMMFD) {
        var res = msg.prop.split(":");
        Module.SMMFD(parseInt(res[0]), parseInt(res[1]), parseInt(res[2]));
      }
    }
    //Send Sysex Message From Delegate
    else if(msg.verb == "SSMFD") {
      if (typeof Module !== 'undefined' && Module.SSMFD) {
        var data = new Uint8Array(msg.data);
        const buffer = Module._malloc(data.length);
        Module.HEAPU8.set(data, buffer);
        Module.SSMFD(parseInt(msg.prop), data.length, buffer);
        Module._free(buffer);
      }
    }
    else if(msg.verb == "StartIdleTimer") {
      if (typeof Module !== 'undefined' && Module.StartIdleTimer) {
        Module.StartIdleTimer();
      }
    }

  }
}
