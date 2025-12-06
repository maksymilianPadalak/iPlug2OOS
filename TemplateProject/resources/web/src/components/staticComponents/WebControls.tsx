/**
 * WebControls Section
 * MIDI and audio status controls (only visible in WAM mode)
 */

import React from 'react';

interface WebControlsProps {
  audioStatus: 'working' | 'not-working' | null;
}

export function WebControls({ audioStatus }: WebControlsProps) {
  const [hasMidiDevices, setHasMidiDevices] = React.useState(false);

  React.useEffect(() => {
    // Check for MIDI devices
    if (navigator.requestMIDIAccess) {
      navigator.requestMIDIAccess().then((midiIF) => {
        const hasInputs = midiIF.inputs.size > 0;
        const hasOutputs = midiIF.outputs.size > 0;
        setHasMidiDevices(hasInputs || hasOutputs);

        // Listen for device changes
        midiIF.onstatechange = () => {
          setHasMidiDevices(midiIF.inputs.size > 0 || midiIF.outputs.size > 0);
        };
      });
    }
  }, []);

  return (
    <>
      {/* Web Controls - Only visible in WAM mode */}
      <div className="wam-only mb-6 max-w-7xl mx-auto">
        <div className="grid grid-cols-3 items-center">
          {/* Audio Status - Right aligned */}
          <div className="flex justify-end">
            {audioStatus && (
              <div className="flex flex-col items-center gap-2">
                <label className="text-[#1a1a1a]/70 text-sm font-semibold uppercase tracking-wider">
                  AUDIO
                </label>
                <p className={`text-sm font-bold uppercase tracking-[0.2em] ${
                  audioStatus === 'working' ? 'text-emerald-600' : 'text-rose-600'
                }`}>
                  {audioStatus === 'working' ? '✓ WORKING' : '✗ NOT WORKING'}
                </p>
              </div>
            )}
          </div>

          {/* Vertical Divider - Centered */}
          <div className="flex justify-center">
            <div className="w-px h-12 bg-[#1a1a1a]/30"></div>
          </div>

          {/* MIDI Controls - Left aligned */}
          <div className="flex justify-start">
            {hasMidiDevices ? (
              <div className="flex items-center gap-8">
                <div className="flex flex-col items-center gap-2">
                  <label htmlFor="midiInSelect" className="text-[#1a1a1a]/70 text-sm font-semibold uppercase tracking-wider">
                    MIDI INPUT
                  </label>
                  <select
                    id="midiInSelect"
                    className="bg-[#1a1a1a] border-2 border-[#1a1a1a] text-[#F5F0E6] px-5 py-2.5 rounded-lg font-semibold text-sm uppercase tracking-wider focus:outline-none focus:ring-2 focus:ring-[#E8D4B8] cursor-pointer transition-all min-w-[180px]"
                  >
                    <option value="default">SELECT</option>
                  </select>
                </div>
                <div className="flex flex-col items-center gap-2">
                  <label htmlFor="midiOutSelect" className="text-[#1a1a1a]/70 text-sm font-semibold uppercase tracking-wider">
                    MIDI OUTPUT
                  </label>
                  <select
                    id="midiOutSelect"
                    className="bg-[#1a1a1a] border-2 border-[#1a1a1a] text-[#F5F0E6] px-5 py-2.5 rounded-lg font-semibold text-sm uppercase tracking-wider focus:outline-none focus:ring-2 focus:ring-[#E8D4B8] cursor-pointer transition-all min-w-[180px]"
                  >
                    <option value="default">SELECT</option>
                  </select>
                </div>
              </div>
            ) : (
              <div className="flex flex-col items-center gap-2">
                <label className="text-[#1a1a1a]/70 text-sm font-semibold uppercase tracking-wider">
                  MIDI
                </label>
                <p className="text-[#1a1a1a]/50 text-sm font-medium uppercase tracking-wider whitespace-nowrap">
                  Connect a MIDI device.
                </p>
              </div>
            )}
          </div>
        </div>
      </div>

      {/* Horizontal Separator - Only visible in WAM mode */}
      <div className="wam-only border-t-4 border-[#1a1a1a] mb-4 max-w-7xl mx-auto"></div>
    </>
  );
}
