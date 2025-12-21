/**
 * Plugin Body - DrumMachine
 *
 * Synthesized drum machine with Kick voice
 */

import { Section } from 'sharedUi/components/Section';
import { SubGroup } from 'sharedUi/components/SubGroup';
import { GridFoundation } from 'sharedUi/components/GridFoundation';
import { Title } from 'sharedUi/components/Title';
import { Knob } from 'sharedUi/components/Knob';
import { Meter } from 'sharedUi/components/Meter';
import { VoiceCard } from 'sharedUi/components/VoiceCard';
import { EParams } from '@/config/runtimeParameters';

export function PluginBody() {
  return (
    <div
      id="plugin-body"
      className="relative rounded-lg p-4 h-full overflow-hidden"
      style={{
        backgroundColor: '#080508',
        backgroundImage: `
          linear-gradient(rgba(255,0,128,0.03) 1px, transparent 1px),
          linear-gradient(90deg, rgba(255,0,128,0.03) 1px, transparent 1px)
        `,
        backgroundSize: '24px 24px',
        boxShadow: 'inset 0 1px 2px rgba(255,0,128,0.05), inset 0 -1px 2px rgba(0,0,0,0.5), 0 0 20px rgba(255,0,128,0.03)',
      }}
    >
      {/* Corner accents */}
      <div className="absolute top-2 left-2 w-8 h-8 border-l border-t border-pink-500/40" />
      <div className="absolute top-2 right-2 w-8 h-8 border-r border-t border-pink-500/40" />
      <div className="absolute bottom-2 left-2 w-8 h-8 border-l border-b border-pink-500/40" />
      <div className="absolute bottom-2 right-2 w-8 h-8 border-r border-b border-pink-500/40" />

      <div className="flex flex-col gap-3 h-full">
        {/* Header */}
        <Title title="DrumMachine" version="1.0" color="magenta" />

        <GridFoundation>
          {/* Master Section */}
          <Section title="Master" size="compact">
            <SubGroup layout="row">
              <Knob paramId={EParams.kParamGain} label="Gain" color="magenta" />
              <div className="flex flex-col gap-1 w-full mt-2">
                <Meter channel={0} label="L" color="magenta" />
                <Meter channel={1} label="R" color="magenta" />
              </div>
            </SubGroup>
          </Section>

                    {/* Kick Section - C3 (48) */}
          <VoiceCard title="Kick" icon="decay" triggerId={48} color="orange">
            <SubGroup layout="grid-4">
              <Knob paramId={EParams.kParamKickPitchStart} label="Start" color="orange" />
              <Knob paramId={EParams.kParamKickPitchEnd} label="End" color="orange" />
              <Knob paramId={EParams.kParamKickPitchDecay} label="P.Decay" color="orange" />
              <Knob paramId={EParams.kParamKickAmpDecay} label="A.Decay" color="orange" />
            </SubGroup>
          </VoiceCard>
        </GridFoundation>
      </div>
    </div>
  );
}
