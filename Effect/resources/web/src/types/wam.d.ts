/**
 * Type definitions for Web Audio Module (WAM) controller
 */

export interface WAMController {
  setParam?(paramIdx: number, value: number): void;
  sendMessage(verb: string, prop: string, data: number | string | ArrayBuffer): void;
  connect(destination: AudioNode): void;
  port?: MessagePort;
  onmessage?: (msg: any) => void;
  input?: AudioNode;
  numberOfInputs?: number;
  numberOfOutputs?: number;
}

export interface AudioWorkletPolyfill {
  polyfill(audioContext: AudioContext): Promise<void>;
  isAudioWorkletPolyfilled: boolean;
}

export interface WAMControllerConstructor {
  importScripts(audioContext: AudioContext): Promise<void>;
  new (audioContext: AudioContext, options: AudioWorkletNodeOptions): WAMController;
}

declare global {
  interface Window {
    AWPF?: AudioWorkletPolyfill;
    WAMController?: WAMControllerConstructor;
    PluginController?: WAMControllerConstructor;
    Plugin_WAM?: WAMController;
  }
}

export {};

