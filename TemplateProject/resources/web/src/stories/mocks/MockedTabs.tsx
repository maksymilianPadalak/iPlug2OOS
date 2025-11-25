/**
 * Mocked Tabs for Storybook
 */

import React, { useState } from 'react';

type MockedTabProps = {
  label: string;
  active: boolean;
  onClick: () => void;
};

export function MockedTab({ label, active, onClick }: MockedTabProps) {
  return (
    <button
      onClick={onClick}
      className={`px-4 py-2 font-mono text-xs uppercase tracking-wider border-2 transition-all ${
        active
          ? 'bg-amber-600 text-white border-amber-600'
          : 'bg-amber-950 text-white border-amber-600 hover:bg-amber-900'
      }`}
    >
      {label}
    </button>
  );
}

type MockedTabContainerProps = {
  tabs: string[];
  initialTab?: number;
  children?: React.ReactNode | ((activeTab: number) => React.ReactNode);
};

export function MockedTabContainer({ tabs, initialTab = 0, children }: MockedTabContainerProps) {
  const [activeTab, setActiveTab] = useState(initialTab);

  return (
    <div className="flex flex-col">
      {/* Tab buttons */}
      <div className="flex gap-2 mb-2 justify-center">
        {tabs.map((tab, index) => (
          <MockedTab
            key={index}
            label={tab}
            active={index === activeTab}
            onClick={() => setActiveTab(index)}
          />
        ))}
      </div>

      {/* Tab content */}
      <div className="bg-amber-950 border-2 border-amber-700 p-3">
        {typeof children === 'function' ? children(activeTab) : children}
      </div>
    </div>
  );
}

