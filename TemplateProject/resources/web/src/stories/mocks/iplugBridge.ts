/**
 * Mock iPlug2 Bridge for Storybook
 * Provides no-op implementations of bridge functions
 */

// No-op: In Storybook we don't send to a real plugin
export function sendParameterValue(_paramIdx: number, _value: number): void {
  // Log for debugging in Storybook
  console.log(`[Storybook Mock] sendParameterValue(${_paramIdx}, ${_value})`);
}

export function sendMidiNote(_channel: number, _pitch: number, _velocity: number): void {
  console.log(`[Storybook Mock] sendMidiNote(${_channel}, ${_pitch}, ${_velocity})`);
}

export function sendMidiCC(_channel: number, _cc: number, _value: number): void {
  console.log(`[Storybook Mock] sendMidiCC(${_channel}, ${_cc}, ${_value})`);
}


