/**
 * Plugin Body - Instrument
 *
 * Simple synth with oscillator, ADSR envelope, waveform display, and master section.
 */

import { Section } from 'sharedUi/components/Section';
import { SubGroup } from 'sharedUi/components/SubGroup';
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
        <Title title="Instrument" version="1.0" color="cyan" />

        {/* Top Row: Master + Output Waveform */}
        <div className="grid grid-cols-4 gap-4">
          <Section title="Master" size="wide">
            <SubGroup layout="row">
              <div className="flex flex-col gap-2 flex-1">
                <Meter channel={0} label="L" color="cyan" />
                <Meter channel={1} label="R" color="cyan" />
              </div>
              <Knob paramId={EParams.kParamGain} label="Gain" color="cyan" />
            </SubGroup>
          </Section>

          <Section title="Output" size="wide" borderless>
            <WaveformDisplay ctrlTag={EControlTags.kCtrlTagWaveform} label="Waveform" />
          </Section>
        </div>

        {/* Bottom Row: Oscillator + Envelope */}
        <div className="grid grid-cols-4 gap-4">
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
        </div>
      </div>
    </div>
  );
}
