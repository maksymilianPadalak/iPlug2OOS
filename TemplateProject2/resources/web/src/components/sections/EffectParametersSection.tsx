/**
 * Effect Parameters Section
 * Dreamy Psychedelic Reverb / Space Delay layout
 */

import React from 'react';
import { Knob } from '../controls/Knob';
import { resolveParamIdx } from '../../config/parameterManifest';

export function EffectParametersSection() {
  const gainParam = resolveParamIdx([
    'kParamGain',
    'Gain',
    'Output',
    'Volume',
    'Level',
  ]);

  const delayTime = resolveParamIdx(['kParamDelayTime', 'Delay Time', 'Time']);
  const delayFeedback = resolveParamIdx([
    'kParamDelayFeedback',
    'Delay Feedback',
    'Feedback',
    'Repeats',
  ]);
  const delayDry = resolveParamIdx(['kParamDelayDry', 'Dry', 'Input Mix']);
  const delayWet = resolveParamIdx(['kParamDelayWet', 'Wet', 'Effect Mix']);

  const hasCoreParams = gainParam || delayTime || delayFeedback || delayDry || delayWet;

  if (!hasCoreParams) {
    return null;
  }

  return (
    <div className="flex flex-col gap-4">
      {/* Main control strip */}
      <div className="flex flex-col lg:flex-row items-stretch justify-between gap-4">
        {/* Input / Dry & Output / Gain cluster */}
        <div className="flex flex-row lg:flex-col gap-3 lg:max-w-[190px]">
          {delayDry && (
            <div className="flex-1 flex flex-col items-center gap-2 px-3 py-3 border border-cyan-700/50 bg-black/50 rounded-md">
              <span className="text-cyan-300 text-[9px] font-bold uppercase tracking-[0.3em]">
                INPUT
              </span>
              <Knob paramIdx={delayDry} label="DRY" />
            </div>
          )}

          {gainParam && (
            <div className="flex-1 flex flex-col items-center gap-2 px-3 py-3 border border-cyan-700/50 bg-black/50 rounded-md">
              <span className="text-cyan-300 text-[9px] font-bold uppercase tracking-[0.3em]">
                OUTPUT
              </span>
              <Knob paramIdx={gainParam} label="GAIN" />
            </div>
          )}
        </div>

        {/* Dream space core */}
        <div className="flex-1 flex flex-col items-center gap-3 px-4 py-4 border border-cyan-500/70 bg-gradient-to-br from-slate-950 via-black to-cyan-950/50 rounded-md shadow-[0_0_40px_rgba(34,211,238,0.25)]">
          <span className="text-cyan-100 text-[9px] font-black uppercase tracking-[0.35em]">
            DREAM SPACE
          </span>

          <div className="flex flex-wrap justify-center gap-5">
            {delayTime && <Knob paramIdx={delayTime} label="SMEAR" />}
            {delayFeedback && <Knob paramIdx={delayFeedback} label="ECHO BLOOM" />}
            {delayWet && <Knob paramIdx={delayWet} label="CLOUD MIX" />}
          </div>

          <div className="mt-1 px-3 py-1 border border-cyan-500/50 bg-black/40 text-[9px] text-cyan-300/80 font-mono uppercase tracking-[0.28em]">
            Bon Iver Choir • Infinite Room
          </div>
        </div>
      </div>

      {/* Usage hint strip */}
      <div className="px-3 py-2 border border-cyan-700/40 bg-black/70 text-[9px] text-cyan-400/90 font-mono uppercase tracking-[0.25em] flex flex-wrap items-center justify-center gap-3">
        <span className="whitespace-nowrap">Turn CLOUD MIX ↑ for washed-out guitars</span>
        <span className="h-px w-8 bg-cyan-500/60" />
        <span className="whitespace-nowrap">Push ECHO BLOOM into self-oscillation</span>
        <span className="h-px w-8 bg-cyan-500/60" />
        <span className="whitespace-nowrap">Sweep SMEAR to stretch the room</span>
      </div>
    </div>
  );
}
