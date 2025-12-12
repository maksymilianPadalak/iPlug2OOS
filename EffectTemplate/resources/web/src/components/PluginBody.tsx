/**
 * Plugin Body - Reverb + Delay Effect
 *
 * Stereo reverb and delay effect with Freeverb-style algorithm.
 * Features: Delay time/feedback, Reverb size/damping/width, Dry/Wet mix.
 */

import { Section } from '@/components/layouts/Section';
import { Knob } from '@/components/controls/Knob';
import { Meter } from '@/components/visualizations/Meter';
import { EParams } from '@/config/runtimeParameters';
import { useParameter } from '@/glue/hooks/useParameter';

function BypassButton() {
  const { value, setValue, beginChange, endChange } = useParameter(EParams.kParamBypass);
  const isBypassed = value > 0.5;

  const handleClick = () => {
    beginChange();
    setValue(isBypassed ? 0 : 1);
    endChange();
  };

  return (
    <button
      onClick={handleClick}
      className={`
        px-4 py-2 rounded-lg font-bold text-xs uppercase tracking-wider
        transition-all duration-150
        ${isBypassed
          ? 'bg-gradient-to-b from-red-500 to-red-600 text-white shadow-lg shadow-red-500/30'
          : 'bg-gradient-to-b from-[#F8F4EF] to-[#EDE5DA] text-[#1a1a1a] border border-[#B8860B]/40'
        }
      `}
      style={{
        boxShadow: isBypassed
          ? '0 4px 12px rgba(239, 68, 68, 0.3)'
          : 'inset 0 1px 2px rgba(255,255,255,0.8), 0 2px 4px rgba(0,0,0,0.08)',
      }}
    >
      {isBypassed ? 'BYPASSED' : 'BYPASS'}
    </button>
  );
}

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
                ReverbDelay
              </span>
            </h1>
          </div>
          <div className="flex items-center gap-4">
            <BypassButton />
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
          </div>
        </header>

        {/* Control Sections */}
        <div className="flex gap-4 justify-center flex-wrap">
          {/* Mix Section */}
          <Section title="Mix">
            <Knob paramId={EParams.kParamMix} label="Dry/Wet" />
          </Section>

          {/* Delay Section */}
          <Section title="Delay">
            <Knob paramId={EParams.kParamDelayTime} label="Time" />
            <Knob paramId={EParams.kParamDelayFeedback} label="Feedback" />
          </Section>

          {/* Reverb Section */}
          <Section title="Reverb">
            <Knob paramId={EParams.kParamReverbSize} label="Size" />
            <Knob paramId={EParams.kParamReverbDamping} label="Damping" />
            <Knob paramId={EParams.kParamReverbWidth} label="Width" />
          </Section>

          {/* Output Section */}
          <Section title="Output">
            <Knob paramId={EParams.kParamGain} label="Gain" />
            <Meter channel={0} />
            <Meter channel={1} />
          </Section>
        </div>
      </div>
    </div>
  );
}
