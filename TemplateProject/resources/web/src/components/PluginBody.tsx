/**
 * Plugin Body - AI-Generated Content
 *
 * This file contains the plugin's visual design and control sections.
 * Modified by the AI code generation pipeline.
 */

import React from 'react';
import { Section } from '@/components/layouts/Section';
import { Knob } from '@/components/controls/Knob';
import { EParams } from '@/config/runtimeParameters';

export function PluginBody() {
  return (
    <div
      id="plugin-body"
      className="rounded-2xl border border-orange-900/40 bg-gradient-to-b from-stone-900 via-neutral-950 to-black shadow-[0_20px_60px_rgba(0,0,0,0.6)] p-4"
    >
      <div className="flex flex-col gap-4">
        {/* Plugin Header */}
        <header className="flex items-center justify-between pb-4 border-b border-orange-900/30">
          <h1 className="text-2xl font-black uppercase tracking-tight">
            <span className="bg-gradient-to-r from-orange-400 via-orange-500 to-red-500 text-transparent bg-clip-text">
              TemplateProject
            </span>
          </h1>
          <div className="flex items-center gap-3">
            <span className="text-orange-400/60 text-xs font-bold uppercase tracking-wider">
              v1.0
            </span>
          </div>
        </header>

        {/* Control Sections */}
        <Section title="Master">
          <div className="flex items-center justify-center h-24">
            <Knob paramId={EParams.kParamGain} label="Gain" size="lg" />
          </div>
        </Section>
      </div>
    </div>
  );
}
