/**
 * Effect Parameters Section
 * Contains effect-specific controls (Gain + Delay parameters)
 */

import React from 'react';
import { Knob } from '../controls/Knob';
import { EParams } from '../../config/constants';

export function EffectParametersSection() {
  return (
    <div className="flex items-stretch justify-center gap-8">
      {/* Gain Knob - self-aligned */}
      <div className="flex flex-col items-center justify-center">
        <Knob paramIdx={EParams.kParamGain} label="GAIN" />
      </div>

      {/* Divider */}
      <div className="w-px bg-gradient-to-b from-transparent via-cyan-500/30 to-transparent self-stretch" />

      {/* Delay Controls - matched height */}
      <div className="flex flex-col items-center justify-center gap-3">
        <span className="text-cyan-300 text-xs font-bold uppercase tracking-wider">Delay</span>
        <div className="flex gap-4">
          <Knob paramIdx={EParams.kParamDelayTime} label="TIME" />
          <Knob paramIdx={EParams.kParamDelayFeedback} label="FEEDBACK" />
          <Knob paramIdx={EParams.kParamDelayDry} label="DRY" />
          <Knob paramIdx={EParams.kParamDelayWet} label="WET" />
        </div>
      </div>
    </div>
  );
}
