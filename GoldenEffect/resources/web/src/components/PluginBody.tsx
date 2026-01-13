/**
 * Plugin Body - Golden Effect (Dattorro Plate Reverb)
 *
 * Professional Dattorro plate reverb UI.
 * Based on the Lexicon 224 topology with figure-8 tank.
 * This is a "golden example" for LLMs to learn from.
 *
 * PARAMETERS (7 total):
 * - Mix: Dry, Wet
 * - Space: Size, Decay, Pre-Delay, Diffusion
 * - Tone: Damping
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
        <Title title="Golden Reverb" version="1.0" color="cyan" />

        <GridFoundation>
          {/* Mix Section - Input/Output levels */}
          <Section title="Mix" size="compact">
            <SubGroup title="Levels" layout="row">
              <div className="flex flex-col gap-2">
                <Meter channel={0} label="L" color="cyan" />
                <Meter channel={1} label="R" color="cyan" />
              </div>
              <Knob paramId={EParams.kParamDry} label="Dry" color="green" size="medium" />
              <Knob paramId={EParams.kParamWet} label="Wet" color="orange" size="medium" />
            </SubGroup>
          </Section>

          {/* Space Section - Room characteristics */}
          <Section title="Space" size="wide" description="Room size and decay">
            <SubGroup title="Tank" layout="row">
              <Knob paramId={EParams.kParamSize} label="Size" color="cyan" size="medium" />
              <Knob paramId={EParams.kParamDecay} label="Decay" color="magenta" size="medium" />
              <Knob paramId={EParams.kParamPreDelay} label="Pre-Delay" color="orange" size="medium" />
              <Knob paramId={EParams.kParamDiffusion} label="Diffusion" color="green" size="medium" />
            </SubGroup>
          </Section>

          {/* Tone Section - Frequency shaping */}
          <Section title="Tone" size="compact" description="High frequency damping">
            <SubGroup title="Color" layout="row">
              <Knob paramId={EParams.kParamDamping} label="Damping" color="orange" size="medium" />
            </SubGroup>
          </Section>
        </GridFoundation>
      </div>
    </div>
  );
}
