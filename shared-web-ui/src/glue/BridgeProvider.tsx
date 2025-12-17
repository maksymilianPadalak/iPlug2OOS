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
 * - meterStore: meter data (SCMFD with IPeakSender/IPeakAvgSender)
 * - waveformStore: waveform data (SCMFD with IBufferSender)
 * - arbitraryMessageStore: LFO waveform, spectrum, etc. (SAMFD)
 * - midiStore: MIDI messages (SMMFD)
 * - systemExclusiveStore: System Exclusive messages (SSMFD)
 */

import React, { useEffect, useMemo } from 'react';
import { useParameterBridge } from './hooks/useParameterBridge';
import { parameterStore } from './state/parameterStore';
import { meterStore } from './state/realtimeBuffers';
import { waveformStore } from './state/waveformStore';
import { arbitraryMessageStore } from './state/arbitraryMessageStore';
import { midiStore } from './state/midiStore';
import { systemExclusiveStore } from './state/systemExclusiveStore';
import { requestStateSync } from './iplugBridge/iplugBridge';
import { getVisualizationForSenderType } from '../config/senderTypeMapping';
import type { SenderType } from '../config/senderTypeMapping';

/**
 * Control tag definition for routing control messages.
 */
export type ControlTag = {
  id: number;
  senderType: string | null;
};

export type BridgeProviderProps = {
  controlTags: ControlTag[];
  children: React.ReactNode;
};

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
 * Parse waveform buffer data from SCMFD buffer (IBufferSender).
 * Format: [12 bytes header][samples: f32[]]
 */
function parseWaveformData(ctrlTag: number, buffer: ArrayBuffer): void {
  // Skip 12-byte header, rest is float32 samples
  const samples = new Float32Array(buffer.slice(12));
  waveformStore.update(ctrlTag, samples);
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

/** Build a lookup map from control tag ID to sender type for fast routing */
function buildSenderTypeLookup(controlTags: ControlTag[]): Map<number, SenderType | null> {
  const lookup = new Map<number, SenderType | null>();
  for (const tag of controlTags) {
    lookup.set(tag.id, tag.senderType as SenderType | null);
  }
  return lookup;
}

export function BridgeProvider({ controlTags, children }: BridgeProviderProps) {
  const senderTypeLookup = useMemo(() => buildSenderTypeLookup(controlTags), [controlTags]);

  useParameterBridge({
    onParameterValue: (paramIdx: number, normalizedValue: number) => {
      parameterStore.set(paramIdx, normalizedValue);
    },
    onControlMessage: (ctrlTag: number, _msgTag: number, _dataSize: number, data: ArrayBuffer) => {
      // Route control messages by visualization component (derived from senderType)
      const senderType = senderTypeLookup.get(ctrlTag);
      const visualization = getVisualizationForSenderType(senderType ?? null);
      if (!visualization) return;

      switch (visualization) {
        case 'Meter':
          parseMeterData(data);
          break;
        case 'WaveformDisplay':
          parseWaveformData(ctrlTag, data);
          break;
      }
    },
    onArbitraryMessage: (msgTag: number, _dataSize: number, data: ArrayBuffer) => {
      // Route ALL arbitrary messages to generic store
      arbitraryMessageStore.set(msgTag, data);
    },
    onMidiMessage: (statusByte: number, dataByte1: number, dataByte2: number) => {
      midiStore.handleMessage(statusByte, dataByte1, dataByte2);
    },
    onSysexMessage: (data: ArrayBuffer) => {
      systemExclusiveStore.handleMessage(data);
    },
    onStateDump: (data: ArrayBuffer) => {
      parseStateDump(data);
    },
  });

  // Request state sync on mount (recovery after reload)
  useEffect(() => {
    requestStateSync();
  }, []);

  return <>{children}</>;
}
