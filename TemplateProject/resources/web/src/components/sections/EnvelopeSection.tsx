/**
 * EnvelopeSection
 * ADSR envelope controls
 */

import React from 'react';
import { EParams } from '../../config/constants';
import { Knob } from '../controls/Knob';

export function EnvelopeSection() {
  return (
    <div className="flex flex-col items-center gap-3">
      <div className="text-orange-300 text-xs font-black uppercase tracking-widest">ENVELOPE</div>
      <div className="flex gap-3">
        <Knob paramIdx={EParams.kParamAttack} label="ATTACK" />
        <Knob paramIdx={EParams.kParamDecay} label="DECAY" />
        <Knob paramIdx={EParams.kParamSustain} label="SUSTAIN" step={0.01} />
        <Knob paramIdx={EParams.kParamRelease} label="RELEASE" />
      </div>
    </div>
  );
}
