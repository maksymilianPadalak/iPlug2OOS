/**
 * OscillatorKnobs Component
 * Control knobs for a single oscillator (Mix, Detune, Octave)
 */

import React from 'react';
import { EParams } from '../../config/constants';
import { Knob } from '../controls/Knob';

interface OscillatorKnobsProps {
  oscNum: 1 | 2 | 3 | 4;
}

export function OscillatorKnobs({ oscNum }: OscillatorKnobsProps) {
  let mixParam = EParams.kParamOsc1Mix;
  let detuneParam = EParams.kParamOsc1Detune;
  let octaveParam = EParams.kParamOsc1Octave;

  switch (oscNum) {
    case 1:
      mixParam = EParams.kParamOsc1Mix;
      detuneParam = EParams.kParamOsc1Detune;
      octaveParam = EParams.kParamOsc1Octave;
      break;
    case 2:
      mixParam = EParams.kParamOsc2Mix;
      detuneParam = EParams.kParamOsc2Detune;
      octaveParam = EParams.kParamOsc2Octave;
      break;
    case 3:
      mixParam = EParams.kParamOsc3Mix;
      detuneParam = EParams.kParamOsc3Detune;
      octaveParam = EParams.kParamOsc3Octave;
      break;
    case 4:
      mixParam = EParams.kParamOsc4Mix;
      detuneParam = EParams.kParamOsc4Detune;
      octaveParam = EParams.kParamOsc4Octave;
      break;
  }

  return (
    <div className="flex gap-2 justify-center w-[180px]">
      <Knob paramIdx={mixParam} label="MIX" step={0.01} />
      <Knob paramIdx={detuneParam} label="DETUNE" />
      <Knob paramIdx={octaveParam} label="OCTAVE" />
    </div>
  );
}
