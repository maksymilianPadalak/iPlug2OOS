/**
 * ReverbSection
 * Reverb effect controls
 */

import React from 'react';
import { EParams } from '../../config/constants';
import { Knob } from '../controls/Knob';

export function ReverbSection() {
  return (
    <div className="flex flex-col items-center gap-3">
      <div className="text-orange-300 text-xs font-black uppercase tracking-widest">REVERB</div>
      <div className="flex gap-3">
        <Knob paramIdx={EParams.kParamReverbRoomSize} label="ROOM" />
        <Knob paramIdx={EParams.kParamReverbDamp} label="DAMP" />
        <Knob paramIdx={EParams.kParamReverbWidth} label="WIDTH" />
        <Knob paramIdx={EParams.kParamReverbDry} label="DRY" step={0.01} />
        <Knob paramIdx={EParams.kParamReverbWet} label="WET" step={0.01} />
      </div>
    </div>
  );
}
