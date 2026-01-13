/**
 * Plugin Body - GoldenEffect2 (Simple Gain Controller)
 *
 * A minimal effect UI with just a gain control.
 * Use this as a starting point for building more complex effects.
 */

import { Section } from 'sharedUi/components/Section';
import { SubGroup } from 'sharedUi/components/SubGroup';
import { GridFoundation } from 'sharedUi/components/GridFoundation';
import { Meter } from 'sharedUi/components/Meter';
import { Title } from 'sharedUi/components/Title';
import { Knob } from 'sharedUi/components/Knob';
import { EParams } from '@/config/runtimeParameters';


export function PluginBody() {
  return (
    <div
      id="plugin-body"
      className="relative rounded-lg p-4 h-full overflow-scroll moderne-scroll"
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
        <Title title="Gain Controller" version="1.0" color="cyan" />

        <GridFoundation>
          <Section title="Main" size="wide">
            <SubGroup title="Output" layout="row">
              <div className="flex flex-col gap-2">
                <Meter channel={0} label="L" color="cyan" />
                <Meter channel={1} label="R" color="cyan" />
              </div>
              <Knob paramId={EParams.kParamGain} label="Gain" color="cyan" size="medium" />
            </SubGroup>
          </Section>
        </GridFoundation>
      </div>
    </div>
  );
}
