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
      onMeterData: (channel, peak, rms) => {
        handlersRef.current.onMeterData?.(channel, peak, rms);
      },
    });

    return () => {
      unregisterProcessorCallbacks();
    };
  }, []);
}




