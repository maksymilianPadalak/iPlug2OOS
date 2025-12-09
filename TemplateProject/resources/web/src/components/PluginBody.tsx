/**
 * Plugin Body - AI-Generated Content
 *
 * This file contains the plugin's visual design and control sections.
 * Modified by the AI code generation pipeline.
 */

import { Section } from '@/components/layouts/Section';
import { Knob } from '@/components/controls/Knob';
import { Meter } from '@/components/visualizations/Meter';
import { EParams } from '@/config/runtimeParameters';

export function PluginBody() {
  return (
    <div
      id="plugin-body"
      className="relative rounded-3xl bg-gradient-to-br from-[#F5E6D3] via-[#EDE0CC] to-[#E8D4B8] p-6"
      style={{
        boxShadow: 'inset 0 2px 4px rgba(255,255,255,0.5), inset 0 -2px 4px rgba(0,0,0,0.1)',
      }}
    >
      {/* Decorative corner accents */}
      <div className="absolute top-4 left-4 w-12 h-12 border-l-2 border-t-2 border-[#B8860B]/40 rounded-tl-xl" />
      <div className="absolute top-4 right-4 w-12 h-12 border-r-2 border-t-2 border-[#B8860B]/40 rounded-tr-xl" />
      <div className="absolute bottom-4 left-4 w-12 h-12 border-l-2 border-b-2 border-[#B8860B]/40 rounded-bl-xl" />
      <div className="absolute bottom-4 right-4 w-12 h-12 border-r-2 border-b-2 border-[#B8860B]/40 rounded-br-xl" />

      {/* Horizontal accent lines */}
      <div className="absolute top-0 left-1/4 right-1/4 h-1 bg-gradient-to-r from-transparent via-[#B8860B]/30 to-transparent rounded-full" />
      <div className="absolute bottom-0 left-1/4 right-1/4 h-1 bg-gradient-to-r from-transparent via-[#B8860B]/30 to-transparent rounded-full" />

      <div className="flex flex-col gap-5">
        {/* Plugin Header */}
        <header className="flex items-center justify-between pb-4 border-b-2 border-[#B8860B]/20">
          <div className="flex items-center gap-4">
            {/* Art Deco geometric accent */}
            <div className="flex gap-1">
              <div className="w-1 h-8 bg-gradient-to-b from-[#B8860B] to-[#8B6914] rounded-full" />
              <div className="w-1 h-6 bg-gradient-to-b from-[#B8860B] to-[#8B6914] rounded-full mt-1" />
              <div className="w-1 h-4 bg-gradient-to-b from-[#B8860B] to-[#8B6914] rounded-full mt-2" />
            </div>
            <h1 className="text-2xl font-black uppercase tracking-tight">
              <span className="bg-gradient-to-r from-[#1a1a1a] via-[#3a3a3a] to-[#1a1a1a] text-transparent bg-clip-text">
                TemplateProject
              </span>
            </h1>
          </div>
          <div className="flex items-center gap-3">
            <div className="flex gap-1">
              <div className="w-2 h-2 rounded-full bg-[#B8860B]/60" />
              <div className="w-2 h-2 rounded-full bg-[#B8860B]/40" />
              <div className="w-2 h-2 rounded-full bg-[#B8860B]/20" />
            </div>
            <span className="text-[#2a2a2a] text-xs font-bold uppercase tracking-[0.2em]">
              v1.0
            </span>
          </div>
        </header>

        {/* Control Sections */}
        <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
          <Section title="Envelope">
            <Knob paramId={EParams.kParamAttack} label="Attack" />
            <Knob paramId={EParams.kParamDecay} label="Decay" />
            <Knob paramId={EParams.kParamSustain} label="Sustain" />
            <Knob paramId={EParams.kParamRelease} label="Release" />
          </Section>

          <Section title="Master">
            <Knob paramId={EParams.kParamGain} label="Gain" />
            <Meter channel={0} />
            <Meter channel={1} />
          </Section>
        </div>
      </div>
    </div>
  );
}
