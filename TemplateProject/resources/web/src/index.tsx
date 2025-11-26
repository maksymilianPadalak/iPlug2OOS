/**
 * React entry point
 */

import React from 'react';
import { createRoot } from 'react-dom/client';
import { App } from '@/components/App';
import { initializeEnvironment } from '@/utils/environment';

// Prevent tab key navigation and touch scrolling
document.addEventListener('keydown', (e) => {
  if (e.keyCode === 9) e.preventDefault();
});

document.addEventListener('touchmove', (e) => {
  e.preventDefault();
}, { passive: false });

// Initialize environment detection
initializeEnvironment();

// Mount React app - wait for DOM to be ready
function mountReactApp() {
  const container = document.getElementById('main');
  if (container) {
    const root = createRoot(container);
    root.render(<App />);
  } else {
    console.error('Could not find #main element to mount React app');
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

