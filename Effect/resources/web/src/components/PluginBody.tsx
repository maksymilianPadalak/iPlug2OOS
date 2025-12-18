/**
 * Plugin Body - Gain Effect
 *
 * Simple stereo gain effect with smoothed parameter control.
 */

import { Meter } from 'sharedUi/components/Meter';
import { Title } from 'sharedUi/components/Title';
import { Knob } from 'sharedUi/components/Knob';
import { EParams } from '@/config/runtimeParameters';

export function PluginBody() {
  return (
    <div
      id="plugin-body"
      className="relative rounded-lg p-4 h-full overflow-hidden"
      style={{
        backgroundColor: '#050508',
        backgroundImage: `
          linear-gradient(rgba(0,255,255,0.04) 1px, transparent 1px),
          linear-gradient(90deg, rgba(0,255,255,0.04) 1px, transparent 1px)
        `,
        backgroundSize: '30px 30px',
        boxShadow: 'inset 0 1px 2px rgba(0,255,255,0.05), inset 0 -1px 2px rgba(0,0,0,0.5), 0 0 20px rgba(0,255,255,0.03)',
      }}
    >
      {/* Corner accents */}
      <div className="absolute top-2 left-2 w-8 h-8 border-l border-t border-cyan-500/40" />
      <div className="absolute top-2 right-2 w-8 h-8 border-r border-t border-cyan-500/40" />
      <div className="absolute bottom-2 left-2 w-8 h-8 border-l border-b border-cyan-500/40" />
      <div className="absolute bottom-2 right-2 w-8 h-8 border-r border-b border-cyan-500/40" />

      <div className="flex flex-col gap-4 h-full">
        {/* Header */}
        <Title title="Gain" version="1.0" color="cyan" />

        {/* Meters + Gain knob */}
        <div className="flex items-center gap-4">
          <div className="flex-1 flex flex-col gap-2">
            <Meter channel={0} label="L" color="cyan" />
            <Meter channel={1} label="R" color="cyan" />
          </div>
          <Knob paramId={EParams.kParamGain} label="Gain" size="small" color="cyan" />
        </div>
      </div>
    </div>
  );
}
