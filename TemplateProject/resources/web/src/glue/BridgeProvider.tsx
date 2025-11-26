/**
 * Bridge Provider
 *
 * Initializes the DSP-UI communication bridge and routes messages
 * to the appropriate stores. This is the single point of connection
 * between the iPlug2 processor and React.
 *
 * ALL message parsing and routing logic lives here - the transport layer
 * (processorCallbacks) is completely agnostic.
 *
 * Stores updated:
 * - parameterStore: parameter values (SPVFD), state dumps (SSTATE)
 * - meterStore: meter data (SCMFD with kCtrlTagMeter)
 * - arbitraryMessageStore: LFO waveform, spectrum, etc. (SAMFD)
 * - midiStore: MIDI messages (SMMFD)
 * - systemExclusiveStore: System Exclusive messages (SSMFD)
 */

import React, { useEffect } from 'react';
import { useParameterBridge } from './hooks/useParameterBridge';
import { parameterStore } from './state/parameterStore';
import { meterStore } from './state/realtimeBuffers';
import { arbitraryMessageStore } from './state/arbitraryMessageStore';
import { midiStore } from './state/midiStore';
import { systemExclusiveStore } from './state/systemExclusiveStore';
import { requestStateSync } from './iplugBridge/iplugBridge';
import { EControlTags } from '../config/runtimeParameters';

/**
 * Parse meter data from SCMFD buffer.
 * Format: [12 bytes header][leftPeak: f32][leftRMS: f32][rightPeak: f32][rightRMS: f32]
 */
function parseMeterData(buffer: ArrayBuffer): void {
  const view = new DataView(buffer);
  const leftPeak = view.getFloat32(12, true);
  const leftRMS = view.getFloat32(16, true);
  const rightPeak = view.getFloat32(20, true);
  const rightRMS = view.getFloat32(24, true);

  meterStore.update(0, leftPeak, leftRMS);
  meterStore.update(1, rightPeak, rightRMS);
}

/**
 * Parse state dump from SSTATE buffer.
 * Format: [paramCount: u16][paramIdx: u16, value: f32]...
 */
function parseStateDump(buffer: ArrayBuffer): void {
  const view = new DataView(buffer);
  const paramCount = view.getUint16(0, true);
  const params = new Map<number, number>();

  for (let i = 0; i < paramCount; i++) {
    const offset = 2 + i * 6;
    if (offset + 6 > buffer.byteLength) {
      console.warn(`State dump truncated at ${i}/${paramCount}`);
      break;
    }
    const paramIdx = view.getUint16(offset, true);
    const value = view.getFloat32(offset + 2, true);
    params.set(paramIdx, value);
  }

  parameterStore.setMany(params);
}

export function BridgeProvider({ children }: { children: React.ReactNode }) {
  useParameterBridge({
    onParameterValue: (paramIdx, normalizedValue) => {
      parameterStore.set(paramIdx, normalizedValue);
    },
    onControlMessage: (ctrlTag, _msgTag, _dataSize, data) => {
      // Route control messages by ctrlTag
      if (ctrlTag === EControlTags.kCtrlTagMeter) {
        parseMeterData(data);
      }
      // Future control message types can be added here
    },
    onArbitraryMessage: (msgTag, _dataSize, data) => {
      // Route ALL arbitrary messages to generic store
      arbitraryMessageStore.set(msgTag, data);
    },
    onMidiMessage: (statusByte, dataByte1, dataByte2) => {
      midiStore.handleMessage(statusByte, dataByte1, dataByte2);
    },
    onSysexMessage: (data) => {
      systemExclusiveStore.handleMessage(data);
    },
    onStateDump: (data) => {
      parseStateDump(data);
    },
  });

  // Request state sync on mount (recovery after reload)
  useEffect(() => {
    requestStateSync();
  }, []);

  return <>{children}</>;
}
