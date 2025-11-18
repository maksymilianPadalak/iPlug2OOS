/**
 * Simple Effect Plugin UI
 * Wide horizontal layout - no scrolling needed
 */

import React from 'react';
import { ParameterProvider } from './ParameterContext';
import { Knob } from './Knob';
import { Meter } from './Meter';
import { TestToneButton } from './TestToneButton';
import { EParams } from '../config/constants';
import { initializeEnvironment } from '../utils/environment';
import { initializeWAM, toggleTestTone, isTestTonePlaying } from '../audio/wam-controller';

export function App() {
  const [audioStatus, setAudioStatus] = React.useState<'working' | 'not-working' | null>(null);
  const [testToneEnabled, setTestToneEnabled] = React.useState(false);

  // Initialize environment detection and auto-start WAM
  React.useEffect(() => {
    const env = initializeEnvironment();

    if (env === 'wam') {
      // Auto-start web audio in WAM mode
      initializeWAM().then((wamController) => {
        if (wamController) {
          window.TemplateProject2_WAM = wamController;
          setAudioStatus('working');
        } else {
          setAudioStatus('not-working');
        }
      }).catch((error) => {
        console.error('Error auto-initializing web audio:', error);
        setAudioStatus('not-working');
      });
    }
  }, []);

  // Handle test tone toggle
  const handleTestToneToggle = React.useCallback(async () => {
    console.log('🎵 Test tone button clicked, current state:', testToneEnabled);
    const newState = !testToneEnabled;
    try {
      await toggleTestTone(newState);
      setTestToneEnabled(newState);
      console.log('✅ Test tone toggled successfully to:', newState);
    } catch (error) {
      console.error('❌ Error toggling test tone:', error);
    }
  }, [testToneEnabled]);

  return (
    <ParameterProvider>
      <div className="w-full h-full bg-gradient-to-br from-stone-950 via-black to-cyan-950/20 flex flex-col">
        {/* Web Controls - Only visible in WAM mode - Fixed at top */}
        {audioStatus && (
          <div className="wam-only flex justify-end p-3">
            <div className={`px-3 py-1 rounded text-xs font-bold uppercase tracking-wider ${
              audioStatus === 'working' ? 'bg-emerald-500/20 text-emerald-400' : 'bg-rose-500/20 text-rose-400'
            }`}>
              {audioStatus === 'working' ? '✓ AUDIO READY' : '✗ AUDIO ERROR'}
            </div>
          </div>
        )}

        {/* Main content centered in remaining space */}
        <div className="flex-1 flex items-center justify-center p-6">

        {/* Plugin Content - Horizontal Layout */}
        <div id="plugin-body" className="w-full max-w-6xl">
          {/* Title at top */}
          <div className="flex flex-col items-center gap-2 mb-8">
            <h1 className="text-4xl font-black uppercase tracking-tighter text-transparent bg-clip-text bg-gradient-to-r from-cyan-200 via-cyan-400 to-cyan-200">
              GAIN
            </h1>
            <div className="h-0.5 w-12 bg-gradient-to-r from-transparent via-cyan-500 to-transparent" />
            <span className="text-cyan-300/60 text-[10px] uppercase tracking-widest font-bold">
              Effect Plugin
            </span>
          </div>

          {/* Main controls in a row */}
          <div className="flex items-center justify-center gap-12">
            {/* Gain Knob */}
            <div className="flex flex-col items-center">
              <Knob paramIdx={EParams.kParamGain} label="GAIN" />
            </div>

            {/* Divider */}
            <div className="h-32 w-px bg-gradient-to-b from-transparent via-cyan-500/30 to-transparent" />

            {/* Output Meters */}
            <div className="flex flex-col items-center gap-4 p-6 border-2 border-cyan-600/30 bg-black/40 rounded">
              <span className="text-cyan-300 text-xs font-bold uppercase tracking-wider">
                Output
              </span>
              <div className="flex items-center gap-6">
                <div className="flex items-center gap-2">
                  <span className="text-cyan-200 text-xs font-bold uppercase">L</span>
                  <Meter channel={0} compact={true} />
                </div>
                <div className="flex items-center gap-2">
                  <span className="text-cyan-200 text-xs font-bold uppercase">R</span>
                  <Meter channel={1} compact={true} />
                </div>
              </div>
            </div>

            {/* Divider */}
            <div className="h-32 w-px bg-gradient-to-b from-transparent via-cyan-500/30 to-transparent" />

            {/* Microphone Input Section */}
            <div className="flex flex-col items-center gap-3 p-6 border-2 border-cyan-600/30 bg-black/40 rounded min-w-[240px]">
              <span className="text-cyan-300 text-xs font-bold uppercase tracking-wider">
                Audio Input
              </span>
              <TestToneButton
                isPlaying={testToneEnabled}
                onToggle={handleTestToneToggle}
              />
            </div>
          </div>
        </div>
        </div>
      </div>
    </ParameterProvider>
  );
}
