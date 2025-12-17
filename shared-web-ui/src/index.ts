// Components
export { Knob, type KnobProps } from "./components/Knob";
export { Section, ControlRow, type SectionProps, type ControlRowProps } from "./components/Section";
export { Meter, type MeterProps } from "./components/Meter";
export { SubGroup, type SubGroupProps } from "./components/SubGroup";
export { Tab, TabContainer, type TabProps, type TabContainerProps } from "./components/Tabs";
export { WaveformDisplay, type WaveformDisplayProps } from "./components/WaveformDisplay";
export { ADSRDisplay, type ADSRDisplayProps } from "./components/ADSRDisplay";
export { Dropdown, type DropdownProps } from "./components/Dropdown";
export { PianoKeyboard, type PianoKeyboardProps } from "./components/PianoKeyboard";
export { KeyboardSection, type KeyboardSectionProps } from "./components/KeyboardSection";
export { WebControls, type WebControlsProps } from "./components/WebControls";
export { PluginHeader, type PluginHeaderProps } from "./components/PluginHeader";
export { TestToneButton, type TestToneButtonProps } from "./components/TestToneButton";
export { AudioFileInput, type AudioFileInputProps } from "./components/AudioFileInput";
export { WAMControls, type WAMControlsProps } from "./components/WAMControls";

// Glue - BridgeProvider
export { BridgeProvider, type BridgeProviderProps, type ControlTag } from "./glue/BridgeProvider";

// Glue - Hooks
export { useParameter } from "./glue/hooks/useParameter";
export { useParameterBridge } from "./glue/hooks/useParameterBridge";
export { useMeter } from "./glue/hooks/useMeter";
export { useWaveform } from "./glue/hooks/useWaveform";
export { useMidi } from "./glue/hooks/useMidi";
export { useArbitraryMessage } from "./glue/hooks/useArbitraryMessage";

// Glue - State Stores
export { parameterStore } from "./glue/state/parameterStore";
export { meterStore } from "./glue/state/realtimeBuffers";
export { waveformStore } from "./glue/state/waveformStore";
export { midiStore } from "./glue/state/midiStore";
export { arbitraryMessageStore } from "./glue/state/arbitraryMessageStore";
export { systemExclusiveStore } from "./glue/state/systemExclusiveStore";

// Glue - Bridge
export {
  sendIPlugMessage,
  sendParameterValue,
  sendParameterEnum,
  sendMIDIMessage,
  sendNoteOn,
  sendNoteOff,
  isIPlugAvailable,
  beginParameterChange,
  endParameterChange,
  sendArbitraryMessage,
  sendKeyPress,
  requestStateSync,
} from "./glue/iplugBridge/iplugBridge";

// Glue - Processor Callbacks
export {
  registerProcessorCallbacks,
  unregisterProcessorCallbacks,
  isUpdatingFromProcessor,
} from "./glue/processorCallbacks/processorCallbacks";
export type { ProcessorEventHandlers } from "./glue/processorCallbacks/types";

// Glue - Errors
export {
  onBridgeError,
  reportBridgeError,
  getRecentErrors,
  clearRecentErrors,
  type BridgeErrorType,
  type BridgeError,
} from "./glue/errors/bridgeErrors";

// Config
export { MessageTypes, CallbackTypes, type MessageType, type CallbackType } from "./config/protocol";
export {
  SENDER_TO_VISUALIZATION,
  getVisualizationForSenderType,
  getSenderTypesForVisualization,
  type SenderType,
  type VisualizationComponent,
} from "./config/senderTypeMapping";

// Types
export type { IPlugUIMessage, IPlugCallbackMessage } from "./types/iplug";
