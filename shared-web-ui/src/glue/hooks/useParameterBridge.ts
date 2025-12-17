import { useEffect, useRef } from "react";

import {
  registerProcessorCallbacks,
  unregisterProcessorCallbacks,
} from "../processorCallbacks/processorCallbacks";
import type { ProcessorEventHandlers } from "../processorCallbacks/types";

export function useParameterBridge(handlers: ProcessorEventHandlers): void {
  const handlersRef = useRef<ProcessorEventHandlers>(handlers);

  useEffect(() => {
    handlersRef.current = handlers;
  }, [handlers]);

  useEffect(() => {
    registerProcessorCallbacks({
      onParameterValue: (paramIdx, normalizedValue) => {
        handlersRef.current.onParameterValue?.(paramIdx, normalizedValue);
      },
      onControlValue: (ctrlTag, normalizedValue) => {
        handlersRef.current.onControlValue?.(ctrlTag, normalizedValue);
      },
      onControlMessage: (ctrlTag, msgTag, dataSize, data) => {
        handlersRef.current.onControlMessage?.(ctrlTag, msgTag, dataSize, data);
      },
      onArbitraryMessage: (msgTag, dataSize, data) => {
        handlersRef.current.onArbitraryMessage?.(msgTag, dataSize, data);
      },
      onMidiMessage: (statusByte, dataByte1, dataByte2) => {
        handlersRef.current.onMidiMessage?.(statusByte, dataByte1, dataByte2);
      },
      onSysexMessage: (data) => {
        handlersRef.current.onSysexMessage?.(data);
      },
      onStateDump: (data) => {
        handlersRef.current.onStateDump?.(data);
      },
    });

    return () => {
      unregisterProcessorCallbacks();
    };
  }, []);
}
