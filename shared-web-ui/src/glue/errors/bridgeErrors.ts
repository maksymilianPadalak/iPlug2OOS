/**
 * Bridge Error Handling System
 *
 * Provides visibility into UI-DSP communication failures.
 * Errors are collected and can be subscribed to for debugging/UI display.
 */

export type BridgeErrorType =
  | 'DECODE_FAILED'      // Base64/binary decode error
  | 'HANDLER_MISSING'    // No handler registered
  | 'HANDLER_THREW'      // Handler threw exception
  | 'MESSAGE_MALFORMED'  // Invalid message format
  | 'STATE_MISMATCH';    // UI/DSP state divergence detected

export type BridgeError = {
  type: BridgeErrorType;
  message: string;
  timestamp: number;
  context?: Record<string, unknown>;
};

type ErrorHandler = (error: BridgeError) => void;

let errorHandlers: ErrorHandler[] = [];
let recentErrors: BridgeError[] = [];
const MAX_RECENT_ERRORS = 50;

/**
 * Subscribe to bridge errors.
 * Returns unsubscribe function.
 */
export function onBridgeError(handler: ErrorHandler): () => void {
  errorHandlers.push(handler);
  return () => {
    errorHandlers = errorHandlers.filter(h => h !== handler);
  };
}

/**
 * Report a bridge error.
 * Notifies all subscribers and stores in recent errors buffer.
 */
export function reportBridgeError(
  type: BridgeErrorType,
  message: string,
  context?: Record<string, unknown>
): void {
  const error: BridgeError = {
    type,
    message,
    timestamp: Date.now(),
    context,
  };

  recentErrors.push(error);
  if (recentErrors.length > MAX_RECENT_ERRORS) {
    recentErrors.shift();
  }

  errorHandlers.forEach(h => h(error));
  console.error(`[Bridge] ${type}: ${message}`, context);
}

/**
 * Get recent errors for debugging/display.
 */
export function getRecentErrors(): BridgeError[] {
  return [...recentErrors];
}

/**
 * Clear recent errors buffer.
 */
export function clearRecentErrors(): void {
  recentErrors = [];
}
