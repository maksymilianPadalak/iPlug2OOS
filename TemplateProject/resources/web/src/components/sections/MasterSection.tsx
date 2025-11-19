/**
 * MasterSection
 * Master output gain control
 */

import React from 'react';
import { EParams } from '../../config/constants';
import { Knob } from '../controls/Knob';

export function MasterSection() {
  return (
    <div className="flex flex-col items-center justify-center pl-2">
      <h2 className="text-orange-300 text-xs font-black uppercase tracking-widest mb-3">MAIN</h2>
      <Knob paramIdx={EParams.kParamGain} label="GAIN" />
    </div>
  );
}
