/**
 * WebControls Section
 * MIDI and audio status controls (only visible in WAM mode)
 */

import React from 'react';

export type WebControlsProps = {
  audioStatus: 'working' | 'not-working' | null;
};

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
      <div className="wam-only max-w-7xl mx-auto">
        <div className="grid grid-cols-3 items-center">
          {/* Audio Status - Right aligned */}
          <div className="flex justify-end">
            {audioStatus && (
              <div className="flex items-center gap-2">
                <span className="text-white/50 text-xs font-semibold uppercase tracking-wider">
                  Audio
                </span>
                <span className={`text-xs font-bold uppercase tracking-wider ${
                  audioStatus === 'working' ? 'text-emerald-400' : 'text-rose-400'
                }`}>
                  {audioStatus === 'working' ? '\u2713' : '\u2717'}
                </span>
              </div>
            )}
          </div>

          {/* Vertical Divider - Centered */}
          <div className="flex justify-center">
            <div className="w-px h-6 bg-white/10"></div>
          </div>

          {/* MIDI Controls - Left aligned */}
          <div className="flex justify-start">
            {hasMidiDevices ? (
              <div className="flex items-center gap-4">
                <div className="flex items-center gap-2">
                  <label htmlFor="midiInSelect" className="text-white/50 text-xs font-semibold uppercase tracking-wider">
                    In
                  </label>
                  <select
                    id="midiInSelect"
                    className="bg-white/10 border border-white/20 text-white px-2 py-1 rounded text-xs focus:outline-none cursor-pointer min-w-[100px]"
                  >
                    <option value="default">Select</option>
                  </select>
                </div>
                <div className="flex items-center gap-2">
                  <label htmlFor="midiOutSelect" className="text-white/50 text-xs font-semibold uppercase tracking-wider">
                    Out
                  </label>
                  <select
                    id="midiOutSelect"
                    className="bg-white/10 border border-white/20 text-white px-2 py-1 rounded text-xs focus:outline-none cursor-pointer min-w-[100px]"
                  >
                    <option value="default">Select</option>
                  </select>
                </div>
              </div>
            ) : (
              <div className="flex items-center gap-2">
                <span className="text-white/50 text-xs font-semibold uppercase tracking-wider">
                  MIDI
                </span>
                <span className="text-white/30 text-xs">
                  No device
                </span>
              </div>
            )}
          </div>
        </div>
      </div>

      {/* Horizontal Separator - Only visible in WAM mode */}
      <div className="wam-only border-t border-white/10 mt-2 max-w-7xl mx-auto"></div>
    </>
  );
}
