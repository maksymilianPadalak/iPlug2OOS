/**
 * Plugin Body - Gain Effect
 *
 * Simple stereo gain effect with smoothed parameter control.
 * Cyberpunk style with futuristic meters.
 */

import { FuturisticKnob } from 'sharedUi/components/FuturisticKnob';
import { FuturisticMeter } from 'sharedUi/components/FuturisticMeter';
import { FuturisticTitle } from 'sharedUi/components/FuturisticTitle';
import { useParameter } from 'sharedUi/hooks/useParameter';
import { EParams } from '@/config/runtimeParameters';
import { normalizedToDisplay } from '@/utils/parameter';

export function PluginBody() {
  const gainParam = useParameter(EParams.kParamGain);
  return (
    <div
      id="plugin-body"
      className="relative rounded-lg bg-gradient-to-br from-slate-900 via-slate-800 to-slate-900 p-4 h-full"
      style={{
        boxShadow: 'inset 0 1px 2px rgba(0,255,255,0.1), inset 0 -1px 2px rgba(0,0,0,0.3), 0 0 30px rgba(0,255,255,0.05)',
      }}
    >
      {/* Corner accents - cyan glow */}
      <div className="absolute top-2 left-2 w-8 h-8 border-l border-t border-cyan-500/40" />
      <div className="absolute top-2 right-2 w-8 h-8 border-r border-t border-cyan-500/40" />
      <div className="absolute bottom-2 left-2 w-8 h-8 border-l border-b border-cyan-500/40" />
      <div className="absolute bottom-2 right-2 w-8 h-8 border-r border-b border-cyan-500/40" />

      <div className="flex flex-col gap-4 h-full">
        {/* Plugin Header */}
        <FuturisticTitle title="Gain" version="1.0" color="cyan" />

        {/* Meters at top */}
        <div className="flex flex-col gap-2">
          <FuturisticMeter channel={0} label="L" color="cyan" />
          <FuturisticMeter channel={1} label="R" color="cyan" />
        </div>

        {/* Main control - centered knob */}
        <div className="flex-1 flex items-center justify-center">
          <div className="flex flex-col items-center gap-2">
            <FuturisticKnob
              value={gainParam.value}
              onChange={gainParam.setValue}
              onBeginChange={gainParam.beginChange}
              onEndChange={gainParam.endChange}
              label="Gain"
              displayValue={normalizedToDisplay(EParams.kParamGain, gainParam.value)}
              color="cyan"
              size="large"
            />
          </div>
        </div>
      </div>
    </div>
  );
}
