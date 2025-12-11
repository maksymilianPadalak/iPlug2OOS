/**
 * PluginHeader Section
 * 
 * Plugin title and version display.
 * AI patches can modify the title to match the generated plugin.
 */

import React from 'react';

type PluginHeaderProps = {
  title?: string;
  version?: string;
};

export function PluginHeader({ 
  title = 'TemplateProject',  // AI_PLUGIN_NAME
  version = '1.0' 
}: PluginHeaderProps) {
  return (
    <header className="flex items-center justify-between pb-4 border-b border-orange-900/30">
      <h1 className="text-2xl font-black uppercase tracking-tight">
        <span className="bg-gradient-to-r from-orange-400 via-orange-500 to-red-500 text-transparent bg-clip-text">
          {title}
        </span>
      </h1>
      <div className="flex items-center gap-3">
        <span className="text-orange-400/60 text-xs font-bold uppercase tracking-wider">
          v{version}
        </span>
      </div>
    </header>
  );
}
