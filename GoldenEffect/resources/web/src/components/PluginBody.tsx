/**
 * Plugin Body - Gain Effect
 *
 * Simple stereo gain effect with smoothed parameter control.
 */

import { Section } from 'sharedUi/components/Section';
import { SubGroup } from 'sharedUi/components/SubGroup';
import { GridFoundation } from 'sharedUi/components/GridFoundation';
import { Meter } from 'sharedUi/components/Meter';
import { Title } from 'sharedUi/components/Title';
import { Knob } from 'sharedUi/components/Knob';
import { Dropdown } from 'sharedUi/components/Dropdown';
import { XYPad } from 'sharedUi/components/XYPad';
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
        <Title title="Gain" version="1.0" color="cyan" />

        {/* Grid for all sections */}
                <GridFoundation>
          <Section title="Controls" size="wide" description=",">
            <SubGroup title="Master" layout="row">
              <div className="flex flex-col gap-2 flex-1">
                <Meter channel={0} label="L" color="cyan" />
                <Meter channel={1} label="R" color="cyan" />
              </div>
              <Knob paramId={EParams.kParamGain} label="Gain" color="magenta" />
            </SubGroup>

                        <SubGroup title="Delay" layout="row">
              <Knob paramId={EParams.kParamDelayTime} label="Time" color="cyan" />
              <Knob paramId={EParams.kParamDelayFeedback} label="Feedback" color="magenta" />
              <Knob paramId={EParams.kParamDelayDry} label="Dry" color="green" />
              <Knob paramId={EParams.kParamDelayWet} label="Wet" color="orange" />
              <Knob paramId={EParams.kParamDelaySync} label="Sync" color="cyan" />
              <Dropdown paramId={EParams.kParamDelayDivision} label="Division" />
                        </SubGroup>

            <SubGroup title="Reverb" layout="row">
              <Knob paramId={EParams.kParamReverbOn} label="On" color="cyan" />
              <Knob paramId={EParams.kParamReverbMix} label="Mix" color="magenta" />
              <Knob paramId={EParams.kParamReverbSize} label="Size" color="green" />
              <Knob paramId={EParams.kParamReverbDecay} label="Decay" color="orange" />
              <Knob paramId={EParams.kParamReverbDamping} label="Damping" color="cyan" />
              <Knob paramId={EParams.kParamReverbPreDelay} label="Pre-Delay" color="magenta" />
              <Knob paramId={EParams.kParamReverbWidth} label="Width" color="green" />
              <Knob paramId={EParams.kParamReverbLowCut} label="Low-Cut" color="orange" />
            </SubGroup>
          </Section>

          <Section size="wide" borderless>
            <XYPad
              paramIdX={EParams.kParamDelayTime}
              paramIdY={EParams.kParamDelayFeedback}
              labelX="Time"
              labelY="Feedback"
            />
          </Section>
        </GridFoundation>

      </div>
    </div>
  );
}
