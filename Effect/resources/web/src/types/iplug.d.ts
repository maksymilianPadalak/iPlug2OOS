/**
 * Type definitions for iPlug2 WebView/WAM communication
 */

/**
 * iPlug2 message from UI to processor
 */
export interface IPlugUIMessage {
  msg: "SPVFUI" | "BPCFUI" | "EPCFUI" | "SAMFUI" | "SMMFUI" | "SKPFUI" | "SREQ";
  paramIdx?: number;
  value?: number;
  msgTag?: number;
  ctrlTag?: number;
  data?: string | ArrayBuffer | number[];
  statusByte?: number;
  dataByte1?: number;
  dataByte2?: number;
  status?: number;
  data1?: number;
  data2?: number;
  // SKPFUI fields
  keyCode?: number;
  utf8?: string;
  S?: boolean; // shift
  C?: boolean; // ctrl
  A?: boolean; // alt
  isUp?: boolean;
}

/**
 * iPlug2 callback from processor to UI
 */
export interface IPlugCallbackMessage {
  verb: "SPVFD" | "SCVFD" | "SCMFD" | "SAMFD" | "SMMFD" | "SSMFD" | "SSTATE" | "StartIdleTimer";
  prop: string | number;
  data?: number | string | ArrayBuffer | number[];
}

/**
 * Global function injected by WebView (AU/VST3) or created by adapter (WAM)
 */
declare global {
  interface Window {
    IPlugSendMsg?: (message: IPlugUIMessage) => void;
    SPVFD?: (paramIdx: number, value: number) => void;
    SCVFD?: (ctrlTag: number, value: number) => void;
    SCMFD?: (ctrlTag: number, msgTag: number, dataSize: number, base64Data: string) => void;
    SAMFD?: (msgTag: number, dataSize: number, base64Data: string) => void;
    SMMFD?: (statusByte: number, dataByte1: number, dataByte2: number) => void;
    SSMFD?: (dataSize: number, base64Data: string) => void;
    SSTATE?: (base64ParamData: string) => void;
    StartIdleTimer?: () => void;
    _iplugEnv?: 'webview' | 'wam';
    initWebViewWAMAdapter?: (wamController: any) => void;
    setupWAMMessageHandlers?: (controller: any) => void;
    getIPlugEnvironment?: () => 'webview' | 'wam';
  }
}

export {};

