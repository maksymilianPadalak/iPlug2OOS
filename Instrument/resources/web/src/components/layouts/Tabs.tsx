import React from 'react';

interface TabProps {
  label: string;
  active: boolean;
  onClick: () => void;
}

export function Tab({ label, active, onClick }: TabProps) {
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

interface TabContainerProps {
  children: React.ReactNode;
  tabs: string[];
  activeTab: number;
  onTabChange: (index: number) => void;
}

export function TabContainer({ children, tabs, activeTab, onTabChange }: TabContainerProps) {
  return (
    <div className="flex flex-col">
      {/* Tab buttons */}
      <div className="flex gap-2 mb-2 justify-center">
        {tabs.map((tab, index) => (
          <Tab
            key={index}
            label={tab}
            active={index === activeTab}
            onClick={() => onTabChange(index)}
          />
        ))}
      </div>

      {/* Tab content */}
      <div className="bg-amber-950 border-2 border-amber-700 p-3">
        {children}
      </div>
    </div>
  );
}
