/**
 * Plugin Body - Instrument
 *
 * Simple synth with oscillator, ADSR envelope, waveform display, and master section.
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
import { EParams, EControlTags } from '@/config/runtimeParameters';

export function PluginBody() {
  return (
    <div
      id="plugin-body"
      className="relative rounded-lg p-4 h-full overflow-hidden"
      style={{
        backgroundColor: '#050508',
        boxShadow: 'inset 0 1px 1px rgba(0,255,255,0.02), inset 0 -1px 1px rgba(0,0,0,0.3), 0 0 10px rgba(0,255,255,0.01)',
      }}
    >
      {/* Corner accents */}
      <div className="absolute top-2 left-2 w-8 h-8 border-l border-t border-cyan-500/40" />
      <div className="absolute top-2 right-2 w-8 h-8 border-r border-t border-cyan-500/40" />
      <div className="absolute bottom-2 left-2 w-8 h-8 border-l border-b border-cyan-500/40" />
      <div className="absolute bottom-2 right-2 w-8 h-8 border-r border-b border-cyan-500/40" />

      <div className="flex flex-col gap-4 h-full">
        {/* Header */}
        <Title title="Instrument" version="1.0" color="cyan" />

        <GridFoundation>
          {/* Top Row: Master with Meters + Gain + Waveform */}
          <Section title="Master" size="full">
            <div className="flex gap-8 items-center">
              <div className="flex-1 flex gap-4 items-center">
                <Knob paramId={EParams.kParamGain} label="Gain" color="cyan" />
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

          {/* Bottom Row: Oscillator + Envelope */}
          <Section title="Oscillator" size="wide">
            <WaveSelector paramId={EParams.kParamWaveform} label="Waveform" />
          </Section>

                    <Section title="Envelope" size="wide">
            <ADSRDisplay
              attackParam={EParams.kParamAttack}
              decayParam={EParams.kParamDecay}
              sustainParam={EParams.kParamSustain}
              releaseParam={EParams.kParamRelease}
              label="Envelope"
            />
            <SubGroup layout="grid-4">
              <Knob paramId={EParams.kParamAttack} label="Attack" color="magenta" />
              <Knob paramId={EParams.kParamDecay} label="Decay" color="magenta" />
              <Knob paramId={EParams.kParamSustain} label="Sustain" color="magenta" />
              <Knob paramId={EParams.kParamRelease} label="Release" color="magenta" />
            </SubGroup>
          </Section>

          <Section title="Heavy Metal" size="wide">
            <SubGroup title="Distortion" layout="row">
              <Knob paramId={EParams.kParamDrive} label="Drive" color="orange" />
              <Knob paramId={EParams.kParamTone} label="Tone" color="green" />
            </SubGroup>
            <SubGroup title="Double" layout="row">
              <Knob paramId={EParams.kParamOsc2Level} label="Osc2 Level" color="magenta" />
              <Knob paramId={EParams.kParamOsc2Detune} label="Osc2 Detune" color="cyan" />
            </SubGroup>
          </Section>
        </GridFoundation>
      </div>
    </div>
  );
}
