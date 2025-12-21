/**
 * TabbedPanel - Generic tabbed container with LED-style tab buttons
 *
 * Use when grouping 2+ similar modules (oscillators, LFOs, envelopes).
 * AI should detect similar parameter groups and use this component.
 *
 * Usage:
 * ```tsx
 * <TabbedPanel
 *   tabs={[
 *     { title: 'OSC 1', color: 'cyan', content: <Osc1Controls /> },
 *     { title: 'OSC 2', color: 'purple', content: <Osc2Controls /> },
 *   ]}
 * />
 * ```
 *
 * Available colors: 'cyan' | 'magenta' | 'green' | 'orange' | 'purple' | 'yellow'
 */

import { useState } from 'react';
import type { TabbedPanelProps } from './componentProps';

export type { TabbedPanelProps };

type TabColor = TabbedPanelProps['tabs'][number]['color'];

const COLOR_STYLES: Record<TabColor, { active: string; led: string; border: string }> = {
  cyan: {
    active: 'bg-cyan-500/10 text-cyan-400 border-cyan-500/50',
    led: 'bg-cyan-400 shadow-[0_0_8px_2px_rgba(34,211,238,0.6)]',
    border: 'border-cyan-500/30 bg-cyan-500/5',
  },
  magenta: {
    active: 'bg-fuchsia-500/10 text-fuchsia-400 border-fuchsia-500/50',
    led: 'bg-fuchsia-400 shadow-[0_0_8px_2px_rgba(217,70,239,0.6)]',
    border: 'border-fuchsia-500/30 bg-fuchsia-500/5',
  },
  green: {
    active: 'bg-green-500/10 text-green-400 border-green-500/50',
    led: 'bg-green-400 shadow-[0_0_8px_2px_rgba(74,222,128,0.6)]',
    border: 'border-green-500/30 bg-green-500/5',
  },
  orange: {
    active: 'bg-orange-500/10 text-orange-400 border-orange-500/50',
    led: 'bg-orange-400 shadow-[0_0_8px_2px_rgba(251,146,60,0.6)]',
    border: 'border-orange-500/30 bg-orange-500/5',
  },
  purple: {
    active: 'bg-purple-500/10 text-purple-400 border-purple-500/50',
    led: 'bg-purple-400 shadow-[0_0_8px_2px_rgba(192,132,252,0.6)]',
    border: 'border-purple-500/30 bg-purple-500/5',
  },
  yellow: {
    active: 'bg-yellow-500/10 text-yellow-400 border-yellow-500/50',
    led: 'bg-yellow-400 shadow-[0_0_8px_2px_rgba(250,204,21,0.6)]',
    border: 'border-yellow-500/30 bg-yellow-500/5',
  },
};

const INACTIVE_STYLES = {
  button: 'bg-gray-900/50 text-gray-500 border-gray-700/50 hover:text-gray-300 hover:border-gray-600',
  led: 'bg-gray-600',
};

export function TabbedPanel({ tabs, defaultTab = 0 }: TabbedPanelProps) {
  const [activeIndex, setActiveIndex] = useState(defaultTab);
  const activeTab = tabs[activeIndex] ?? tabs[0];

  if (tabs.length === 0) return null;

  return (
    <div>
      {/* Tab buttons with LED indicators */}
      <div className="flex gap-3 mb-4">
        {tabs.map((tab, index) => {
          const isActive = index === activeIndex;
          const colorStyles = COLOR_STYLES[tab.color];

          return (
            <button
              key={index}
              onClick={() => setActiveIndex(index)}
              className={`flex items-center gap-2 px-4 py-2 rounded-md font-medium text-sm transition-all border ${
                isActive ? colorStyles.active : INACTIVE_STYLES.button
              }`}
            >
              {/* LED indicator */}
              <span
                className={`w-2.5 h-2.5 rounded-full transition-all ${
                  isActive ? colorStyles.led : INACTIVE_STYLES.led
                }`}
              />
              {tab.title}
            </button>
          );
        })}
      </div>

      {/* Tab content */}
      <div className={`p-4 rounded-lg border ${COLOR_STYLES[activeTab.color].border}`}>
        {activeTab.content}
      </div>
    </div>
  );
}
