/**
 * Plugin Body - Instrument
 *
 * Subtractive synth with oscillator, SVF filter, ADSR envelope, and master section.
 */

import { Section } from 'sharedUi/components/Section';
import { SubGroup } from 'sharedUi/components/SubGroup';
import { GridFoundation } from 'sharedUi/components/GridFoundation';
import { Title } from 'sharedUi/components/Title';
import { Knob } from 'sharedUi/components/Knob';
import { Meter } from 'sharedUi/components/Meter';
import { WaveformDisplay } from 'sharedUi/components/WaveformDisplay';
import { WaveSelector } from 'sharedUi/components/WaveSelector';
import { ADSRDisplay } from 'sharedUi/components/ADSRDisplay';
import { Dropdown } from 'sharedUi/components/Dropdown';
import { EParams, EControlTags } from '@/config/runtimeParameters';

export function PluginBody() {
  return (
    <div
      id="plugin-body"
      className="relative rounded-lg p-4 h-full overflow-hidden"
      style={{
        backgroundColor: '#050508',
        backgroundImage: `
          linear-gradient(rgba(0,255,255,0.04) 1px, transparent 1px),
          linear-gradient(90deg, rgba(0,255,255,0.04) 1px, transparent 1px)
        `,
        backgroundSize: '30px 30px',
        boxShadow: 'inset 0 1px 2px rgba(0,255,255,0.05), inset 0 -1px 2px rgba(0,0,0,0.5), 0 0 20px rgba(0,255,255,0.03)',
      }}
    >
      {/* Corner accents */}
      <div className="absolute top-2 left-2 w-8 h-8 border-l border-t border-cyan-500/40" />
      <div className="absolute top-2 right-2 w-8 h-8 border-r border-t border-cyan-500/40" />
      <div className="absolute bottom-2 left-2 w-8 h-8 border-l border-b border-cyan-500/40" />
      <div className="absolute bottom-2 right-2 w-8 h-8 border-r border-b border-cyan-500/40" />

      <div className="flex flex-col gap-4 h-full">
        {/* Header */}
        <div className="flex items-center justify-between">
          <Title title="GoldenSynth" version="1.0" color="cyan" />
          <Dropdown paramId={EParams.kParamPresetSelect} label="Preset" />
        </div>

        <div className="flex-1 overflow-y-auto overflow-x-hidden" style={{ scrollbarGutter: 'stable' }}>
        <GridFoundation>
          {/* Top Row: Master with Meters + Gain + Waveform */}
          <Section title="Master" size="full">
            <div className="flex gap-8 items-center">
              <div className="flex-1 flex gap-4 items-center">
                <Knob paramId={EParams.kParamGain} label="Gain" color="cyan" />
                <Dropdown paramId={EParams.kParamVoiceMode} label="Mode" color="cyan" />
                <Dropdown paramId={EParams.kParamGlideEnable} label="Glide" color="cyan" />
                <Knob paramId={EParams.kParamGlideTime} label="Time" color="cyan" />
                <div className="flex flex-col gap-2 flex-1">
                  <Meter channel={0} label="L" color="cyan" />
                  <Meter channel={1} label="R" color="cyan" />
                </div>
              </div>
              <div className="flex-1">
                <WaveformDisplay ctrlTag={EControlTags.kCtrlTagWaveform} label="Waveform" />
              </div>
            </div>
          </Section>

          {/* Middle Row: Oscillator 1 + Oscillator 2 */}
          <Section title="Oscillator 1" size="wide">
            <SubGroup layout="row">
              <WaveSelector paramId={EParams.kParamWaveform} label="Waveform" />
              <Knob paramId={EParams.kParamPulseWidth} label="PW" color="cyan" />
              <Dropdown paramId={EParams.kParamFMRatio} label="Ratio" />
              <Knob paramId={EParams.kParamFMFine} label="Fine" color="cyan" />
              <Knob paramId={EParams.kParamFMDepth} label="Depth" color="cyan" />
              <Knob paramId={EParams.kParamWavetablePosition} label="WT Pos" color="cyan" />
              <Dropdown paramId={EParams.kParamOsc1Octave} label="Octave" />
              <Knob paramId={EParams.kParamOsc1Detune} label="Detune" color="cyan" />
              <Knob paramId={EParams.kParamOsc1Pan} label="Pan" color="cyan" />
              <Knob paramId={EParams.kParamOsc1Level} label="Level" color="cyan" />
            </SubGroup>
            {/* Osc1 Unison - per-oscillator unison (Serum-style) */}
            <SubGroup layout="row">
              <Dropdown paramId={EParams.kParamOsc1UnisonVoices} label="Unison" />
              <Knob paramId={EParams.kParamOsc1UnisonDetune} label="Uni Det" color="cyan" />
              <Knob paramId={EParams.kParamOsc1UnisonWidth} label="Uni Wid" color="cyan" />
              <Knob paramId={EParams.kParamOsc1UnisonBlend} label="Uni Bld" color="cyan" />
            </SubGroup>
          </Section>

          <Section title="Oscillator 2" size="wide">
            <SubGroup layout="row">
              <WaveSelector paramId={EParams.kParamOsc2Waveform} label="Waveform" />
              <Knob paramId={EParams.kParamOsc2PulseWidth} label="PW" color="green" />
              <Dropdown paramId={EParams.kParamOsc2FMRatio} label="Ratio" />
              <Knob paramId={EParams.kParamOsc2FMFine} label="Fine" color="green" />
              <Knob paramId={EParams.kParamOsc2FMDepth} label="Depth" color="green" />
              <Knob paramId={EParams.kParamOsc2Morph} label="WT Pos" color="green" />
              <Dropdown paramId={EParams.kParamOsc2Octave} label="Octave" />
              <Knob paramId={EParams.kParamOsc2Detune} label="Detune" color="green" />
              <Knob paramId={EParams.kParamOsc2Pan} label="Pan" color="green" />
              <Knob paramId={EParams.kParamOsc2Level} label="Level" color="green" />
              <Dropdown paramId={EParams.kParamOscSync} label="Sync" />
            </SubGroup>
            {/* Osc2 Unison - independent from Osc1 */}
            <SubGroup layout="row">
              <Dropdown paramId={EParams.kParamOsc2UnisonVoices} label="Unison" />
              <Knob paramId={EParams.kParamOsc2UnisonDetune} label="Uni Det" color="green" />
              <Knob paramId={EParams.kParamOsc2UnisonWidth} label="Uni Wid" color="green" />
              <Knob paramId={EParams.kParamOsc2UnisonBlend} label="Uni Bld" color="green" />
            </SubGroup>
          </Section>

          {/* Sub Oscillator - Serum-style with waveform selection and Direct Out */}
          <Section title="Sub" size="wide">
            <SubGroup layout="row">
              <Dropdown paramId={EParams.kParamSubOscEnable} label="On" />
              <Dropdown paramId={EParams.kParamSubOscWaveform} label="Wave" />
              <Dropdown paramId={EParams.kParamSubOscOctave} label="Octave" />
              <Knob paramId={EParams.kParamSubOscLevel} label="Level" color="magenta" />
              <Knob paramId={EParams.kParamSubOscPan} label="Pan" color="magenta" />
              <Dropdown paramId={EParams.kParamSubOscDirectOut} label="Direct" />
            </SubGroup>
          </Section>

          {/* Filter + LFO Row */}
          <Section title="Filter" size="wide">
            <SubGroup layout="row">
              <Dropdown paramId={EParams.kParamFilterEnable} label="On" />
              <Dropdown paramId={EParams.kParamFilterType} label="Type" />
              <Knob paramId={EParams.kParamFilterCutoff} label="Cutoff" color="orange" />
              <Knob paramId={EParams.kParamFilterResonance} label="Reso" color="orange" />
              <Knob paramId={EParams.kParamFilterEnvAttack} label="Env A" color="orange" />
              <Knob paramId={EParams.kParamFilterEnvDecay} label="Env D" color="orange" />
              <Knob paramId={EParams.kParamFilterEnvSustain} label="Env S" color="orange" />
              <Knob paramId={EParams.kParamFilterEnvRelease} label="Env R" color="orange" />
              <Knob paramId={EParams.kParamFilterEnvDepth} label="Env Amt" color="orange" />
            </SubGroup>
          </Section>

          <Section title="LFO 1" size="wide">
            <SubGroup layout="row">
              <Dropdown paramId={EParams.kParamLFO1Enable} label="On" />
              <Dropdown paramId={EParams.kParamLFO1Waveform} label="Wave" />
              <Knob paramId={EParams.kParamLFO1Rate} label="Rate" color="orange" />
              <Dropdown paramId={EParams.kParamLFO1Sync} label="Sync" />
              <Knob paramId={EParams.kParamLFO1Low} label="Low" color="orange" />
              <Knob paramId={EParams.kParamLFO1High} label="High" color="orange" />
              <Dropdown paramId={EParams.kParamLFO1Retrigger} label="Retrig" />
              <Dropdown paramId={EParams.kParamLFO1Destination} label="Dest" />
            </SubGroup>
          </Section>

          <Section title="LFO 2" size="wide">
            <SubGroup layout="row">
              <Dropdown paramId={EParams.kParamLFO2Enable} label="On" />
              <Dropdown paramId={EParams.kParamLFO2Waveform} label="Wave" />
              <Knob paramId={EParams.kParamLFO2Rate} label="Rate" color="cyan" />
              <Dropdown paramId={EParams.kParamLFO2Sync} label="Sync" />
              <Knob paramId={EParams.kParamLFO2Low} label="Low" color="cyan" />
              <Knob paramId={EParams.kParamLFO2High} label="High" color="cyan" />
              <Dropdown paramId={EParams.kParamLFO2Retrigger} label="Retrig" />
              <Dropdown paramId={EParams.kParamLFO2Destination} label="Dest" />
            </SubGroup>
          </Section>

          {/* Delay - Stereo delay with ping-pong mode */}
          <Section title="Delay" size="full">
            <SubGroup layout="row">
              <Dropdown paramId={EParams.kParamDelayEnable} label="On" />
              <Knob paramId={EParams.kParamDelayTime} label="Time" color="magenta" />
              <Dropdown paramId={EParams.kParamDelaySync} label="Sync" />
              <Knob paramId={EParams.kParamDelayFeedback} label="Feedback" color="magenta" />
              <Knob paramId={EParams.kParamDelayDry} label="Dry" color="magenta" />
              <Knob paramId={EParams.kParamDelayWet} label="Wet" color="magenta" />
              <Dropdown paramId={EParams.kParamDelayMode} label="Mode" />
            </SubGroup>
          </Section>

          {/* Bottom Row: Envelope */}
          <Section title="Envelope" size="full">
            <ADSRDisplay
              attackParam={EParams.kParamAttack}
              decayParam={EParams.kParamDecay}
              sustainParam={EParams.kParamSustain}
              releaseParam={EParams.kParamRelease}
              label="Envelope"
            />
            <SubGroup layout="row">
              <Knob paramId={EParams.kParamAttack} label="Attack" color="magenta" />
              <Knob paramId={EParams.kParamDecay} label="Decay" color="magenta" />
              <Knob paramId={EParams.kParamSustain} label="Sustain" color="magenta" />
              <Knob paramId={EParams.kParamRelease} label="Release" color="magenta" />
              <Knob paramId={EParams.kParamEnvVelocity} label="Vel" color="magenta" />
            </SubGroup>
          </Section>
        </GridFoundation>
        </div>
      </div>
    </div>
  );
}
