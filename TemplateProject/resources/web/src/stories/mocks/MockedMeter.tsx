/**
 * Mocked Meter for Storybook - Vintage Analog VU Style
 */

import React from 'react';

type MockedMeterProps = {
  peakDb?: number;
  channel?: 0 | 1;
  compact?: boolean;
};

export function MockedMeter({ peakDb = -12, channel = 0, compact = false }: MockedMeterProps) {
  // Format dB display
  const formatDb = (db: number): string => {
    if (db === -Infinity || db <= -60) return '−∞';
    return db >= 0 ? `+${db.toFixed(1)}` : `${db.toFixed(1)}`;
  };

  const isClipping = peakDb >= -0.5;
  const isHot = peakDb >= -6 && peakDb < -0.5;

  // Normalize to 0-1 for needle angle (map -40dB to +3dB range)
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

  return (
    <div className="flex flex-col items-center">
      {/* Meter housing */}
      <div
        className="relative overflow-hidden"
        style={{
          width: '120px',
          height: '100px',
          background: 'linear-gradient(145deg, #8B7355 0%, #6B5740 100%)',
          borderRadius: '6px',
          boxShadow: `
            inset 0 1px 1px rgba(255,255,255,0.15),
            0 2px 8px rgba(0,0,0,0.3)
          `,
          border: '1px solid #5A4A35',
          padding: '4px',
        }}
      >
      {/* Vintage brass trim - thin inner border */}
      <div
        className="absolute pointer-events-none"
        style={{
          left: '3px',
          right: '3px',
          top: '3px',
          bottom: '3px',
          borderRadius: '4px',
          border: '1px solid #D4AF37',
          opacity: 0.4,
        }}
      />

      {/* Meter face background - aged cream/ivory with patina */}
      <div
        className="absolute"
        style={{
          left: '4px',
          right: '4px',
          top: '4px',
          bottom: '4px',
          background: `
            radial-gradient(ellipse at 30% 20%, rgba(255,255,250,0.9) 0%, transparent 50%),
            linear-gradient(180deg, #FAF6E8 0%, #F0E8D8 40%, #E8DCC8 100%)
          `,
          borderRadius: '4px',
          boxShadow: 'inset 0 1px 4px rgba(0,0,0,0.15), inset 0 0 20px rgba(139,115,85,0.08)',
        }}
      />

      {/* Aged paper texture overlay */}
      <div
        className="absolute pointer-events-none"
        style={{
          left: '4px',
          right: '4px',
          top: '4px',
          bottom: '4px',
          borderRadius: '4px',
          background: 'url("data:image/svg+xml,%3Csvg viewBox=\'0 0 100 100\' xmlns=\'http://www.w3.org/2000/svg\'%3E%3Cfilter id=\'noise\'%3E%3CfeTurbulence type=\'fractalNoise\' baseFrequency=\'0.8\' numOctaves=\'4\' stitchTiles=\'stitch\'/%3E%3C/filter%3E%3Crect width=\'100%25\' height=\'100%25\' filter=\'url(%23noise)\'/%3E%3C/svg%3E")',
          opacity: 0.03,
          mixBlendMode: 'multiply',
        }}
      />

      {/* Scale arc and markings */}
      <svg
        className="absolute"
        style={{ left: '4px', top: '4px', width: '112px', height: '92px' }}
        viewBox="0 0 112 92"
      >
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
              fill="none"
              stroke="#C41E00"
              strokeWidth="2.5"
              opacity="0.5"
              strokeLinecap="round"
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
              <line
                x1={x1}
                y1={y1}
                x2={x2}
                y2={y2}
                stroke={mark.db >= 0 ? '#C41E00' : '#3A3530'}
                strokeWidth={mark.major ? 1.5 : 0.75}
                opacity={mark.db >= 0 ? 0.8 : 0.5}
              />
              {mark.major && (
                <text
                  x={labelX}
                  y={labelY}
                  textAnchor="middle"
                  dominantBaseline="middle"
                  fill={mark.db >= 0 ? '#B01800' : '#5A5550'}
                  fontSize="8"
                  fontWeight="600"
                  fontFamily="Georgia, serif"
                >
                  {mark.label}
                </text>
              )}
            </g>
          );
        })}

        {/* VU label - vintage style */}
        <text
          x="56"
          y="58"
          textAnchor="middle"
          fill="#6B5B4B"
          fontSize="9"
          fontWeight="bold"
          fontFamily="Georgia, serif"
          letterSpacing="0.12em"
        >
          VU
        </text>
      </svg>

      {/* Needle pivot point - vintage brass */}
      <div
        className="absolute"
        style={{
          left: '50%',
          bottom: '18px',
          width: '10px',
          height: '10px',
          marginLeft: '-5px',
          background: 'radial-gradient(circle at 30% 30%, #C9B896 0%, #8B7355 60%, #6B5740 100%)',
          borderRadius: '50%',
          boxShadow: `
            inset 0 1px 1px rgba(255,255,255,0.4),
            0 1px 2px rgba(0,0,0,0.3)
          `,
          zIndex: 20,
        }}
      >
        <div
          className="absolute"
          style={{
            left: '50%',
            top: '50%',
            width: '4px',
            height: '4px',
            marginLeft: '-2px',
            marginTop: '-2px',
            background: 'radial-gradient(circle at 30% 30%, #A89470 0%, #5A4A35 100%)',
            borderRadius: '50%',
          }}
        />
      </div>

      {/* Needle with arrow tip - vintage dark */}
      <div
        className="absolute"
        style={{
          left: '50%',
          bottom: '23px',
          width: '2px',
          height: '52px',
          marginLeft: '-1px',
          transformOrigin: 'center bottom',
          transform: `rotate(${needleAngle}deg)`,
          background: `linear-gradient(to top,
            #2A2520 0%,
            ${isClipping ? '#C41E00' : '#1A1510'} 80%,
            ${isClipping ? '#E84420' : '#2A2520'} 100%
          )`,
          borderRadius: '0.5px',
          boxShadow: isClipping
            ? '0 0 6px rgba(196, 30, 0, 0.5)'
            : '0.5px 0.5px 1px rgba(0,0,0,0.3)',
          zIndex: 10,
        }}
      >
        {/* Arrow tip */}
        <div
          className="absolute -top-1 left-1/2 -translate-x-1/2"
          style={{
            width: 0,
            height: 0,
            borderLeft: '3px solid transparent',
            borderRight: '3px solid transparent',
            borderBottom: `5px solid ${isClipping ? '#C41E00' : '#1A1510'}`,
          }}
        />
      </div>

      </div>

      {/* Current dB readout - integrated vintage frame */}
      <div
        style={{
          background: 'linear-gradient(180deg, #8B7355 0%, #6B5740 100%)',
          padding: '2px 3px',
          borderRadius: '3px',
          boxShadow: `
            inset 0 1px 1px rgba(255,255,255,0.15),
            0 2px 4px rgba(0,0,0,0.25)
          `,
          border: '1px solid #5A4A35',
          marginTop: '-14px',
          zIndex: 40,
          position: 'relative',
        }}
      >
        <div
          style={{
            background: 'linear-gradient(180deg, #F8F4E8 0%, #EBE3D0 100%)',
            padding: '3px 0',
            borderRadius: '2px',
            boxShadow: 'inset 0 1px 2px rgba(0,0,0,0.1)',
            width: '70px',
            textAlign: 'center',
            border: '1px solid #D0C4A8',
          }}
        >
          <span
            className={`text-[11px] font-bold tabular-nums ${
              isClipping ? 'text-[#B01800]' : isHot ? 'text-[#8B6914]' : 'text-[#3A3530]'
            }`}
            style={{
              fontFamily: 'Georgia, serif',
              letterSpacing: '0.05em',
            }}
          >
            {formatDb(peakDb)} dB
          </span>
        </div>
      </div>
    </div>
  );
}

// Bar meter variant kept for backwards compatibility
type MockedBarMeterProps = {
  value?: number;
  label?: string;
  orientation?: 'horizontal' | 'vertical';
};

export function MockedBarMeter({ value = 0.7, label, orientation = 'vertical' }: MockedBarMeterProps) {
  // Convert 0-1 to dB for the analog meter
  const peakDb = value > 0.0001 ? 20 * Math.log10(value) : -Infinity;
  return <MockedMeter peakDb={peakDb} />;
}

