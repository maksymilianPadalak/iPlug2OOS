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
        backgroundColor: '#080808',
        backgroundImage: `
          linear-gradient(rgba(255,0,128,0.02) 1px, transparent 1px),
          linear-gradient(90deg, rgba(255,0,128,0.02) 1px, transparent 1px)
        `,
        backgroundSize: '24px 24px',
        boxShadow: 'inset 0 1px 2px rgba(255,0,128,0.03), inset 0 -1px 2px rgba(0,0,0,0.5), 0 0 20px rgba(255,0,128,0.02)',
      }}
    >
      {/* Corner accents */}
      <div className="absolute top-2 left-2 w-8 h-8 border-l border-t border-pink-500/20" />
      <div className="absolute top-2 right-2 w-8 h-8 border-r border-t border-pink-500/20" />
      <div className="absolute bottom-2 left-2 w-8 h-8 border-l border-b border-pink-500/20" />
      <div className="absolute bottom-2 right-2 w-8 h-8 border-r border-b border-pink-500/20" />

      <div className="flex flex-col gap-3 h-full">
        {/* Header */}
        <Title title="DrumMachine" version="1.0" color="magenta" />

        <GridFoundation>
          {/* Master Section */}
          <Section title="Master" size="wide">
            <SubGroup layout="row">
              <Knob paramId={EParams.kParamGain} label="Gain" color="cyan" size="small" />
              <div className="flex flex-col gap-1 flex-1">
                <Meter channel={0} label="L" color="cyan" />
                <Meter channel={1} label="R" color="cyan" />
              </div>
            </SubGroup>
          </Section>

          {/* Kick Section - C3 (48) */}
          <VoiceCard title="Kick" icon="decay" triggerId={48} color="orange">
            <SubGroup layout="grid-4">
              <Knob paramId={EParams.kParamKickPitchStart} label="Hi" color="orange" size="small" />
              <Knob paramId={EParams.kParamKickPitchEnd} label="Lo" color="orange" size="small" />
              <Knob paramId={EParams.kParamKickPitchDecay} label="Drop" color="orange" size="small" />
              <Knob paramId={EParams.kParamKickAmpDecay} label="Ring" color="orange" size="small" />
            </SubGroup>
          </VoiceCard>
        </GridFoundation>
      </div>
    </div>
  );
}
