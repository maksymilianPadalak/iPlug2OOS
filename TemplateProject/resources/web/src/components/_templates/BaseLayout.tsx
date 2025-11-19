/**
 * BASE TEMPLATE: Layout Component
 *
 * This is a template for creating new layout/container components.
 * Use this as a reference when AI needs to create organizational UI elements.
 *
 * Key patterns demonstrated:
 * - Flexible container patterns
 * - Responsive design
 * - Child component composition
 * - Berlin Brutalism styling (borders, stark contrast)
 * - Section organization
 * - Collapsible/expandable sections
 */

import React from 'react';

interface BaseLayoutProps {
  /** Layout title */
  title?: string;
  /** Child components */
  children: React.ReactNode;
  /** Layout direction */
  direction?: 'horizontal' | 'vertical';
  /** Whether section can be collapsed */
  collapsible?: boolean;
  /** Initial collapsed state */
  initialCollapsed?: boolean;
}

export function BaseLayout({
  title,
  children,
  direction = 'vertical',
  collapsible = false,
  initialCollapsed = false,
}: BaseLayoutProps) {
  const [isCollapsed, setIsCollapsed] = React.useState(initialCollapsed);

  const toggleCollapsed = () => {
    if (collapsible) {
      setIsCollapsed(!isCollapsed);
    }
  };

  return (
    <section className="border-2 border-white bg-black p-4">
      {/* Header */}
      {title && (
        <div
          className={`
            flex items-center justify-between mb-3 pb-2 border-b-2 border-white/30
            ${collapsible ? 'cursor-pointer select-none hover:border-white/50' : ''}
          `}
          onClick={toggleCollapsed}
        >
          <h2 className="text-white text-xs font-bold uppercase tracking-widest font-mono">
            {title}
          </h2>
          {collapsible && (
            <span className="text-white text-xs font-mono">
              {isCollapsed ? '▼' : '▲'}
            </span>
          )}
        </div>
      )}

      {/* Content */}
      {!isCollapsed && (
        <div
          className={`
            flex gap-4
            ${direction === 'horizontal' ? 'flex-row flex-wrap' : 'flex-col'}
          `}
        >
          {children}
        </div>
      )}

      {/* Collapsed state */}
      {isCollapsed && (
        <div className="text-white/40 text-[10px] uppercase tracking-wider font-mono text-center py-2">
          Collapsed
        </div>
      )}
    </section>
  );
}

/**
 * Example: Grid Layout Component
 *
 * Demonstrates grid-based organization for multiple controls
 */
export function BaseGridLayout({
  title,
  children,
  columns = 3,
}: {
  title?: string;
  children: React.ReactNode;
  columns?: number;
}) {
  return (
    <section className="border-2 border-white bg-black p-4">
      {/* Header */}
      {title && (
        <h2 className="text-white text-xs font-bold uppercase tracking-widest font-mono mb-3 pb-2 border-b-2 border-white/30">
          {title}
        </h2>
      )}

      {/* Grid */}
      <div
        className="grid gap-4"
        style={{
          gridTemplateColumns: `repeat(${columns}, 1fr)`,
        }}
      >
        {children}
      </div>
    </section>
  );
}

/**
 * Example: Split Panel Layout
 *
 * Demonstrates side-by-side panel organization
 */
export function BaseSplitLayout({
  leftPanel,
  rightPanel,
  splitRatio = 50,
}: {
  leftPanel: React.ReactNode;
  rightPanel: React.ReactNode;
  splitRatio?: number;
}) {
  return (
    <div className="flex gap-4 w-full">
      {/* Left Panel */}
      <div
        className="border-2 border-white bg-black p-4"
        style={{ flex: splitRatio }}
      >
        {leftPanel}
      </div>

      {/* Divider */}
      <div className="w-0.5 bg-white/30" />

      {/* Right Panel */}
      <div
        className="border-2 border-white bg-black p-4"
        style={{ flex: 100 - splitRatio }}
      >
        {rightPanel}
      </div>
    </div>
  );
}
