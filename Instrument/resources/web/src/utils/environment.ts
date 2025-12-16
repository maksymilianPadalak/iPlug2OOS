/**
 * Environment detection utilities
 */

export type Environment = 'webview' | 'wam';

/**
 * Detect the runtime environment (WebView or WAM)
 */
export function detectEnvironment(): Environment {
  // Check if IPlugSendMsg already exists (injected by platform WebView)
  if (typeof window.IPlugSendMsg === 'function') {
    try {
      const funcStr = window.IPlugSendMsg.toString();
      if (funcStr.includes('webkit.messageHandlers') || funcStr.includes('chrome.webview')) {
        return 'webview';
      }
    } catch (e) {
      // If toString() fails, assume WAM mode
      console.debug('Could not inspect IPlugSendMsg:', e);
    }
  }

  // Check if we're in a browser environment (WAM)
  if (typeof window !== 'undefined' && typeof AudioWorkletNode !== 'undefined') {
    return 'wam';
  }

  // Default to WAM if uncertain (since we're in a browser)
  return 'wam';
}

/**
 * Initialize environment detection on page load
 */
export function initializeEnvironment(): Environment {
  const env = detectEnvironment();
  
  // Apply environment class to body
  if (document.body) {
    document.body.classList.add(`${env}-mode`);
    
    // Hide WAM-specific elements immediately in WebView mode
    if (env === 'webview') {
      document.addEventListener('DOMContentLoaded', () => {
        const wamOnlyElements = document.querySelectorAll('.wam-only');
        wamOnlyElements.forEach(el => (el as HTMLElement).style.display = 'none');
      });
    }
  }
  
  // Store environment globally
  window._iplugEnv = env;
  
  return env;
}

