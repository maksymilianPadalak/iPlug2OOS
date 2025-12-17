/**
 * Plugin Body - Gain Effect
 *
 * Simple stereo gain effect with smoothed parameter control.
 */

import { Section, Knob, Meter, useParameter, FuturisticTitle } from 'sharedUi';
import { EParams } from '@/config/runtimeParameters';
import { normalizedToDisplay } from '@/utils/parameter';

export function PluginBody() {
  const gainParam = useParameter(EParams.kParamGain);
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
        <FuturisticTitle title="Gain" version="1.0" color="cyan" />

        {/* Control Section */}
        <div className="flex gap-4 justify-center">
          <Section title="Output">
            <Knob
              value={gainParam.value}
              onChange={gainParam.setValue}
              onBeginChange={gainParam.beginChange}
              onEndChange={gainParam.endChange}
              label="Gain"
              displayValue={normalizedToDisplay(EParams.kParamGain, gainParam.value)}
            />
            <Meter channel={0} />
            <Meter channel={1} />
          </Section>
        </div>
      </div>
    </div>
  );
}
