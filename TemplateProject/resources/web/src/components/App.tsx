/**
 * Main App component
 */

import React from 'react';
import { ParameterProvider, useParameters } from './ParameterContext';
import { WAMControls } from './WAMControls';
import { Slider } from './Slider';
import { Dropdown } from './Dropdown';
import { Checkbox } from './Checkbox';
import { Meter } from './Meter';
import { LFOWaveform } from './LFOWaveform';
import { PianoKeyboard } from './PianoKeyboard';
import { EParams } from '../config/constants';
import { initializeEnvironment } from '../utils/environment';

export function App() {
  // Initialize environment detection
  React.useEffect(() => {
    initializeEnvironment();
  }, []);

  return (
    <ParameterProvider>
      <div style={{ padding: '15px', maxWidth: '1400px', margin: '0 auto' }}>
        <h2 style={{ color: '#ffffff', margin: '0 0 15px 0', textAlign: 'center', fontSize: '24px', fontWeight: 'bold' }}>
          TEMPLATE SYNTH
        </h2>
        
        <p id="adapterStatus" className="wam-only" style={{ color: '#ffff00', textAlign: 'center', marginBottom: '15px', fontSize: '11px' }}>
          Waiting for AudioWorklet initialization... Click "Start web audio!" above to begin.
        </p>

        <WAMControls />

        <div style={{ display: 'grid', gridTemplateColumns: 'repeat(3, 1fr)', gap: '15px', marginBottom: '15px' }}>
          {/* ADSR Section */}
          <div style={{ border: '2px solid #ffffff', padding: '15px', background: 'rgba(0,0,0,0.3)' }}>
            <h3 style={{ color: '#ffffff', margin: '0 0 15px 0', fontSize: '24px', fontWeight: 'bold' }}>ADSR</h3>
            <div style={{ display: 'flex', flexDirection: 'column', gap: '15px' }}>
              <Slider paramIdx={EParams.kParamAttack} label="A - Attack" />
              <Slider paramIdx={EParams.kParamDecay} label="D - Decay" />
              <Slider paramIdx={EParams.kParamSustain} label="S - Sustain" step={0.01} />
              <Slider paramIdx={EParams.kParamRelease} label="R - Release" />
            </div>
          </div>

          {/* Main Controls Section */}
          <div style={{ border: '2px solid #ffffff', padding: '15px', background: 'rgba(0,0,0,0.3)' }}>
            <h3 style={{ color: '#ffffff', margin: '0 0 15px 0', fontSize: '24px', fontWeight: 'bold' }}>MAIN</h3>
            <div style={{ display: 'flex', flexDirection: 'column', gap: '20px' }}>
              <Slider paramIdx={EParams.kParamGain} label="GAIN" />
              <Slider paramIdx={EParams.kParamNoteGlideTime} label="GLIDE" />
            </div>
          </div>

          {/* LFO Section */}
          <div style={{ border: '2px solid #ffffff', padding: '15px', background: 'rgba(0,0,0,0.3)' }}>
            <h3 style={{ color: '#ffffff', margin: '0 0 15px 0', fontSize: '24px', fontWeight: 'bold' }}>LFO</h3>
            <div style={{ display: 'flex', flexDirection: 'column', gap: '15px' }}>
              <Dropdown
                paramIdx={EParams.kParamLFOShape}
                label="SHAPE"
                options={['Triangle', 'Square', 'Ramp Up', 'Ramp Down', 'Sine']}
              />
              <LFOSyncCheckbox />
              <LFORateControls />
              <Slider paramIdx={EParams.kParamLFODepth} label="DEPTH" />
              <LFOWaveform />
            </div>
          </div>
        </div>

        {/* Output Level Meter */}
        <div style={{ marginTop: '15px', border: '2px solid #ffffff', padding: '12px', background: 'rgba(0,0,0,0.3)' }}>
          <h3 style={{ color: '#ffffff', margin: '0 0 10px 0', fontSize: '18px', fontWeight: 'bold' }}>
            OUTPUT LEVEL
          </h3>
          <div style={{ display: 'flex', gap: '15px', alignItems: 'center' }}>
            <div style={{ flex: 1 }}>
              <div style={{ display: 'flex', justifyContent: 'space-between', marginBottom: '5px' }}>
                <span style={{ color: '#ffffff', fontSize: '11px' }}>L</span>
                <span style={{ color: '#ffffff', fontSize: '11px' }}>R</span>
              </div>
              <div style={{ display: 'flex', gap: '8px', height: '180px' }}>
                <Meter channel={0} />
                <Meter channel={1} />
              </div>
              <div style={{ display: 'flex', justifyContent: 'space-between', marginTop: '5px' }}>
                <span style={{ color: '#cccccc', fontSize: '10px' }}>-60</span>
                <span style={{ color: '#cccccc', fontSize: '10px' }}>-40</span>
                <span style={{ color: '#cccccc', fontSize: '10px' }}>-20</span>
                <span style={{ color: '#cccccc', fontSize: '10px' }}>0</span>
              </div>
            </div>
          </div>
        </div>

        {/* Keyboard Section */}
        <div style={{ marginTop: '15px', border: '2px solid #ffffff', padding: '12px', background: 'rgba(0,0,0,0.3)' }}>
          <PianoKeyboard />
        </div>
      </div>
    </ParameterProvider>
  );
}

/**
 * LFO Sync checkbox with conditional rate controls
 */
function LFOSyncCheckbox() {
  return (
    <Checkbox
      paramIdx={EParams.kParamLFORateMode}
      label="SYNC"
    />
  );
}

/**
 * LFO Rate controls (Hz or Tempo based on sync)
 */
function LFORateControls() {
  const { paramValues } = useParameters();
  const syncValue = paramValues.get(EParams.kParamLFORateMode) ?? 1.0;
  const syncEnabled = syncValue > 0.5;

  if (syncEnabled) {
    return (
      <Dropdown
        paramIdx={EParams.kParamLFORateTempo}
        label="RATE"
        options={['1/64', '1/32', '1/16T', '1/16', '1/16D', '1/8T', '1/8', '1/8D', '1/4', '1/4D', '1/2', '1/1', '2/1', '4/1', '8/1']}
      />
    );
  } else {
    return (
      <Slider paramIdx={EParams.kParamLFORateHz} label="RATE HZ" />
    );
  }
}

