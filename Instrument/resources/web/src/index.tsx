/**
 * React entry point
 *
 * Renders full App (with header, keyboard) in WAM/web mode,
 * or just PluginBody in WebView (AU/VST) mode.
 */

import React from 'react';
import { createRoot } from 'react-dom/client';
import { App } from '@/components/App';
import { PluginBody } from '@/components/PluginBody';
import { BridgeProvider } from 'sharedUi/BridgeProvider';
import { RuntimeParametersProvider } from 'sharedUi/RuntimeParametersProvider';
import { controlTags, runtimeParameters } from '@/config/runtimeParameters';
import { initializeEnvironment } from '@/utils/environment';

// Prevent tab key navigation and touch scrolling
document.addEventListener('keydown', (e) => {
  if (e.keyCode === 9) e.preventDefault();
});

document.addEventListener('touchmove', (e) => {
  e.preventDefault();
}, { passive: false });

// Initialize environment detection
const env = initializeEnvironment();

// Mount React app - wait for DOM to be ready
function mountReactApp() {
  const container = document.getElementById('main');
  if (!container) {
    console.error('Could not find #main element to mount React app');
    return;
  }

  const root = createRoot(container);

  if (env === 'webview') {
    // AU/VST: Render only PluginBody with minimal providers
    root.render(
      <RuntimeParametersProvider parameters={runtimeParameters}>
        <BridgeProvider controlTags={controlTags}>
          <PluginBody />
        </BridgeProvider>
      </RuntimeParametersProvider>
    );
  } else {
    // Web: Render full App with header, keyboard, etc.
    root.render(<App />);
  }
}

// Wait for DOM to be ready
if (document.readyState === 'loading') {
  document.addEventListener('DOMContentLoaded', mountReactApp);
} else {
  // DOM already loaded
  mountReactApp();
}

// Handle websocket mode
if ((window as any).WEBSOCKET_MODE === true) {
  const buttonsEl = document.getElementById('buttons');
  if (buttonsEl) {
    buttonsEl.style.display = 'none';
  }
}

