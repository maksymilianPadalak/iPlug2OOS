/**
 * KeyboardSection - Web Preview Only
 *
 * Piano keyboard for testing MIDI input in web/standalone mode.
 * This component is NOT part of the plugin UI in DAW - it's only
 * visible in the web preview for testing without a MIDI controller.
 *
 * DO NOT modify this for plugin customization - it's a fixed utility.
 */

import React from 'react';
import { PianoKeyboard } from '@/components/staticComponents/PianoKeyboard';

export function KeyboardSection() {
  return (
    <div className="mt-4">
      <PianoKeyboard />
    </div>
  );
}
