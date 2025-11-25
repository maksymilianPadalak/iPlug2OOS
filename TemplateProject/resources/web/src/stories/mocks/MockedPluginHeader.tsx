/**
 * Mocked PluginHeader for Storybook
 * Plugin title and version display
 */

import React from 'react';

type MockedPluginHeaderProps = {
  title?: string;
  version?: string;
  showVersion?: boolean;
};

export function MockedPluginHeader({ 
  title = 'TemplateProject',
  version = '1.0',
  showVersion = true
}: MockedPluginHeaderProps) {
  return (
    <header className="flex items-center justify-between pb-4 border-b border-orange-900/30">
      <h1 className="text-2xl font-black uppercase tracking-tight">
        <span className="bg-gradient-to-r from-orange-400 via-orange-500 to-red-500 text-transparent bg-clip-text">
          {title}
        </span>
      </h1>
      {showVersion && (
        <div className="flex items-center gap-3">
          <span className="text-orange-400/60 text-xs font-bold uppercase tracking-wider">
            v{version}
          </span>
        </div>
      )}
    </header>
  );
}

// Compact header variant
type MockedCompactHeaderProps = {
  title?: string;
  subtitle?: string;
};

export function MockedCompactHeader({ 
  title = 'Plugin',
  subtitle
}: MockedCompactHeaderProps) {
  return (
    <div className="flex items-center gap-2">
      <h1 className="text-lg font-black uppercase tracking-tight">
        <span className="bg-gradient-to-r from-orange-400 to-red-500 text-transparent bg-clip-text">
          {title}
        </span>
      </h1>
      {subtitle && (
        <span className="text-orange-300/50 text-xs font-medium uppercase tracking-wider">
          {subtitle}
        </span>
      )}
    </div>
  );
}

