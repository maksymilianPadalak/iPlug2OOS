/**
 * VintageMeter Component
 *
 * VU-style meter visualization for audio levels.
 * Uses useMeter hook from sharedUi glue.
 *
 * NOTE: This component is kept for potential future use but is not
 * included in the LLM component registry. Use Meter for new plugins.
 */

import { useMeter } from '../glue/hooks/useMeter';
import type { VintageMeterProps } from './componentProps';

export type { VintageMeterProps };

export function VintageMeter({ channel, compact = false }: VintageMeterProps) {
  const meter = useMeter(channel);

  // Calculate dB values from linear amplitude (0-1)
  const peakDb = meter.peak > 0.0001 ? 20 * Math.log10(meter.peak) : -Infinity;
  const rmsValue = meter.rms !== undefined ? meter.rms : meter.peak * 0.7;
  const rmsDb = rmsValue > 0.0001 ? 20 * Math.log10(rmsValue) : -Infinity;

  // Format dB display
  const formatDb = (db: number): string => {
    if (db === -Infinity || db <= -60) return '−∞';
    return db >= 0 ? `+${db.toFixed(1)}` : `${db.toFixed(1)}`;
  };

  const isClipping = peakDb >= -0.5;
  const isHot = peakDb >= -6 && peakDb < -0.5;

  // Normalize to 0-1 for needle angle (map -40dB to +3dB range to needle swing)
  const minDb = -40;
  const maxDb = 3;
  const clampedDb = Math.max(minDb, Math.min(maxDb, peakDb === -Infinity ? minDb : peakDb));
  const normalizedValue = (clampedDb - minDb) / (maxDb - minDb);

  // Needle angle: -55deg (left/min) to +55deg (right/max) matching the arc
  const needleAngle = -55 + normalizedValue * 110;

  // Scale markings for the meter (VU-style) - simplified for clarity
  const scaleMarks = [
    { db: -40, label: '40', major: true },
    { db: -20, label: '20', major: true },
    { db: -10, label: '10', major: false },
    { db: -6, label: '6', major: false },
    { db: -3, label: '3', major: false },
    { db: 0, label: '0', major: true },
    { db: 3, label: '+3', major: true },
  ];

  const channelLabel = channel === 0 ? 'L' : 'R';

  return (
    <div className="flex flex-col items-center">
      {/* Channel label */}
      <span className="text-[10px] font-bold uppercase tracking-[0.08em] text-[#1a1a1a] mb-1">
        {channelLabel}
      </span>
      {/* Meter housing */}
      <div
        className="relative overflow-hidden"
        style={{
          width: '120px',
          height: '100px',
          background: 'linear-gradient(145deg, #8B7355 0%, #6B5740 100%)',
          borderRadius: '6px',
          boxShadow: `inset 0 1px 1px rgba(255,255,255,0.15), 0 2px 8px rgba(0,0,0,0.3)`,
          border: '1px solid #5A4A35',
          padding: '4px',
        }}
      >
        {/* Vintage brass trim */}
        <div
          className="absolute pointer-events-none"
          style={{
            left: '3px', right: '3px', top: '3px', bottom: '3px',
            borderRadius: '4px',
            border: '1px solid #D4AF37',
            opacity: 0.4,
          }}
        />

        {/* Meter face background */}
        <div
          className="absolute"
          style={{
            left: '4px', right: '4px', top: '4px', bottom: '4px',
            background: `radial-gradient(ellipse at 30% 20%, rgba(255,255,250,0.9) 0%, transparent 50%),
              linear-gradient(180deg, #FAF6E8 0%, #F0E8D8 40%, #E8DCC8 100%)`,
            borderRadius: '4px',
            boxShadow: 'inset 0 1px 4px rgba(0,0,0,0.15), inset 0 0 20px rgba(139,115,85,0.08)',
          }}
        />

        {/* Scale arc and markings */}
        <svg className="absolute" style={{ left: '4px', top: '4px', width: '112px', height: '92px' }} viewBox="0 0 112 92">
          {/* Red zone arc (0dB to +3dB) */}
          {(() => {
            const startNorm = (0 - minDb) / (maxDb - minDb);
            const endNorm = 1;
            const startAngle = (-145 + startNorm * 110) * Math.PI / 180;
            const endAngle = (-145 + endNorm * 110) * Math.PI / 180;
            const cx = 56, cy = 70, r = 52;
            const x1 = cx + Math.cos(startAngle) * r;
            const y1 = cy + Math.sin(startAngle) * r;
            const x2 = cx + Math.cos(endAngle) * r;
            const y2 = cy + Math.sin(endAngle) * r;
            return (
              <path
                d={`M ${x1} ${y1} A ${r} ${r} 0 0 1 ${x2} ${y2}`}
                fill="none" stroke="#C41E00" strokeWidth="2.5" opacity="0.5" strokeLinecap="round"
              />
            );
          })()}

          {/* Scale tick marks and labels */}
          {scaleMarks.map((mark) => {
            const normalized = (mark.db - minDb) / (maxDb - minDb);
            const angle = -145 + normalized * 110;
            const radians = (angle * Math.PI) / 180;
            const cx = 56, cy = 70;
            const innerRadius = 44;
            const outerRadius = mark.major ? 56 : 51;
            const labelRadius = 33;
            const x1 = cx + Math.cos(radians) * innerRadius;
            const y1 = cy + Math.sin(radians) * innerRadius;
            const x2 = cx + Math.cos(radians) * outerRadius;
            const y2 = cy + Math.sin(radians) * outerRadius;
            const labelX = cx + Math.cos(radians) * labelRadius;
            const labelY = cy + Math.sin(radians) * labelRadius;

            return (
              <g key={mark.db}>
                <line x1={x1} y1={y1} x2={x2} y2={y2}
                  stroke={mark.db >= 0 ? '#C41E00' : '#3A3530'}
                  strokeWidth={mark.major ? 1.5 : 0.75}
                  opacity={mark.db >= 0 ? 0.8 : 0.5}
                />
                {mark.major && (
                  <text x={labelX} y={labelY} textAnchor="middle" dominantBaseline="middle"
                    fill={mark.db >= 0 ? '#B01800' : '#5A5550'} fontSize="8" fontWeight="600" fontFamily="Georgia, serif">
                    {mark.label}
                  </text>
                )}
              </g>
            );
          })}

          {/* VU label */}
          <text x="56" y="58" textAnchor="middle" fill="#6B5B4B" fontSize="9" fontWeight="bold" fontFamily="Georgia, serif" letterSpacing="0.12em">
            VU
          </text>
        </svg>

        {/* Needle pivot point */}
        <div className="absolute" style={{
          left: '50%', bottom: '18px', width: '10px', height: '10px', marginLeft: '-5px',
          background: 'radial-gradient(circle at 30% 30%, #C9B896 0%, #8B7355 60%, #6B5740 100%)',
          borderRadius: '50%',
          boxShadow: `inset 0 1px 1px rgba(255,255,255,0.4), 0 1px 2px rgba(0,0,0,0.3)`,
          zIndex: 20,
        }}>
          <div className="absolute" style={{
            left: '50%', top: '50%', width: '4px', height: '4px', marginLeft: '-2px', marginTop: '-2px',
            background: 'radial-gradient(circle at 30% 30%, #A89470 0%, #5A4A35 100%)',
            borderRadius: '50%',
          }} />
        </div>

        {/* Needle */}
        <div className="absolute" style={{
          left: '50%', bottom: '23px', width: '2px', height: '52px', marginLeft: '-1px',
          transformOrigin: 'center bottom',
          transform: `rotate(${needleAngle}deg)`,
          transition: 'transform 300ms cubic-bezier(0.4, 0.0, 0.2, 1)',
          background: `linear-gradient(to top, #2A2520 0%, ${isClipping ? '#C41E00' : '#1A1510'} 80%, ${isClipping ? '#E84420' : '#2A2520'} 100%)`,
          borderRadius: '0.5px',
          boxShadow: isClipping ? '0 0 6px rgba(196, 30, 0, 0.5)' : '0.5px 0.5px 1px rgba(0,0,0,0.3)',
          zIndex: 10,
        }}>
          <div className="absolute -top-1 left-1/2 -translate-x-1/2" style={{
            width: 0, height: 0,
            borderLeft: '3px solid transparent', borderRight: '3px solid transparent',
            borderBottom: `5px solid ${isClipping ? '#C41E00' : '#1A1510'}`,
          }} />
        </div>
      </div>

      {/* Current dB readout */}
      <div style={{
        background: 'linear-gradient(180deg, #8B7355 0%, #6B5740 100%)',
        padding: '2px 3px', borderRadius: '3px',
        boxShadow: `inset 0 1px 1px rgba(255,255,255,0.15), 0 2px 4px rgba(0,0,0,0.25)`,
        border: '1px solid #5A4A35', marginTop: '-14px', zIndex: 40, position: 'relative',
      }}>
        <div style={{
          background: 'linear-gradient(180deg, #F8F4E8 0%, #EBE3D0 100%)',
          padding: '3px 0', borderRadius: '2px',
          boxShadow: 'inset 0 1px 2px rgba(0,0,0,0.1)',
          width: '70px', textAlign: 'center', border: '1px solid #D0C4A8',
        }}>
          <span className={`text-[11px] font-bold tabular-nums ${isClipping ? 'text-[#B01800]' : isHot ? 'text-[#8B6914]' : 'text-[#3A3530]'}`}
            style={{ fontFamily: 'Georgia, serif', letterSpacing: '0.05em' }}>
            {formatDb(peakDb)} dB
          </span>
        </div>
      </div>
    </div>
  );
}
