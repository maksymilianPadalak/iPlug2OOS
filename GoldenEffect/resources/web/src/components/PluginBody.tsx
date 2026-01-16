/**
 * Plugin Body - Golden Effect (Multi-Mode Reverb)
 *
 * Professional algorithmic reverb UI with multiple space types.
 * Based on the Lexicon 224 topology with figure-8 tank.
 * This is a "golden example" for LLMs to learn from.
 *
 * PARAMETERS (13 total):
 * - Mix: Dry, Wet
 * - Space: Mode, Size, Decay, Pre-Delay, Density, Early/Late
 * - Tone: Low Cut, High Cut, Color (Color includes output EQ + feedback damping)
 * - Modulation: Mod Rate, Mod Depth
 * - Output: Width
 */

import { Section } from 'sharedUi/components/Section';
import { SubGroup } from 'sharedUi/components/SubGroup';
import { GridFoundation } from 'sharedUi/components/GridFoundation';
import { Meter } from 'sharedUi/components/Meter';
import { Title } from 'sharedUi/components/Title';
import { Knob } from 'sharedUi/components/Knob';
import { Dropdown } from 'sharedUi/components/Dropdown';
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
          <Section title="Space" size="wide" description="Reverb type and room characteristics">
            <SubGroup title="Type" layout="row">
              <Dropdown paramId={EParams.kParamMode} label="Mode" color="cyan" />
            </SubGroup>
            <SubGroup title="Reverb" layout="row">
              <Knob paramId={EParams.kParamSize} label="Size" color="cyan" size="medium" />
              <Knob paramId={EParams.kParamDecay} label="Decay" color="magenta" size="medium" />
              <Knob paramId={EParams.kParamPreDelay} label="Pre-Delay" color="orange" size="medium" />
              <Knob paramId={EParams.kParamDensity} label="Density" color="green" size="medium" />
              <Knob paramId={EParams.kParamEarlyLate} label="Early/Late" color="cyan" size="medium" />
            </SubGroup>
          </Section>

          {/* Tone Section - Frequency shaping */}
          {/* Color controls BOTH output EQ AND feedback damping (FutureVerb-style) */}
          <Section title="Tone" size="wide" description="Input EQ and tonal character">
            <SubGroup title="Input EQ" layout="row">
              <Knob paramId={EParams.kParamLowCut} label="Low Cut" color="cyan" size="medium" />
              <Knob paramId={EParams.kParamHighCut} label="High Cut" color="magenta" size="medium" />
            </SubGroup>
            <SubGroup title="Character" layout="row">
              <Dropdown paramId={EParams.kParamColor} label="Color" color="green" />
            </SubGroup>
          </Section>

          {/* Modulation Section - Tank LFO for lushness */}
          <Section title="Modulation" size="compact" description="Tank LFO for lushness">
            <SubGroup title="LFO" layout="row">
              <Knob paramId={EParams.kParamModRate} label="Rate" color="magenta" size="medium" />
              <Knob paramId={EParams.kParamModDepth} label="Depth" color="cyan" size="medium" />
            </SubGroup>
          </Section>

          {/* Output Section - Stereo image */}
          <Section title="Output" size="compact" description="Stereo width control">
            <SubGroup title="Stereo" layout="row">
              <Knob paramId={EParams.kParamWidth} label="Width" color="green" size="medium" />
            </SubGroup>
          </Section>
        </GridFoundation>
      </div>
    </div>
  );
}
