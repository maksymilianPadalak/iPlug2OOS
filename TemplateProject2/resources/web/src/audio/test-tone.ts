/**
 * Test Tone Generator for Effect Plugin Testing
 * Generates a simple 440Hz sine wave for testing audio effects
 */

export class TestToneGenerator {
  private audioContext: AudioContext;
  private oscillator: OscillatorNode | null = null;
  private gainNode: GainNode;
  private isPlaying: boolean = false;

  constructor(audioContext: AudioContext) {
    this.audioContext = audioContext;

    // Create gain node for controlling test tone volume
    this.gainNode = audioContext.createGain();
    this.gainNode.gain.value = 0.3; // 30% volume to avoid being too loud
  }

  /**
   * Start playing the test tone
   * @param destination - Audio destination to connect to (usually the effect processor)
   */
  async start(destination: AudioNode): Promise<void> {
    if (this.isPlaying) {
      console.warn('⚠️  Test tone already playing');
      return;
    }

    console.log('🎵 Test Tone Generator - Starting...');
    console.log('   AudioContext state:', this.audioContext.state);
    console.log('   AudioContext sample rate:', this.audioContext.sampleRate);
    console.log('   Destination node:', destination);
    console.log('   Destination constructor name:', destination.constructor.name);

    // Ensure AudioContext is resumed
    if (this.audioContext.state === 'suspended') {
      console.log('   Resuming suspended AudioContext...');
      await this.audioContext.resume();
      console.log('   AudioContext resumed, new state:', this.audioContext.state);
    }

    // Create new oscillator (they can only be used once)
    this.oscillator = this.audioContext.createOscillator();
    this.oscillator.type = 'sine';
    this.oscillator.frequency.value = 440; // A4 note (440Hz)

    console.log('   Created oscillator - type:', this.oscillator.type, 'freq:', this.oscillator.frequency.value);
    console.log('   Gain value:', this.gainNode.gain.value);

    // Connect: Oscillator -> Gain -> Destination (Effect Processor)
    this.oscillator.connect(this.gainNode);
    console.log('   ✓ Oscillator connected to gain node');

    this.gainNode.connect(destination);
    console.log('   ✓ Gain node connected to destination');
    console.log('   Destination type:', destination.constructor.name);

    // Start oscillator
    this.oscillator.start();
    this.isPlaying = true;

    console.log('🔊 Test tone started: 440Hz sine wave at', (this.gainNode.gain.value * 100).toFixed(0) + '%', 'volume');
    console.log('   Audio chain: Oscillator → Gain (', this.gainNode.gain.value, ') → WAM Controller → WAM DSP → Output');
    console.log('   ⚠️  Effect plugins need input to process - test tone is feeding the WAM controller input');
  }

  /**
   * Stop playing the test tone
   */
  stop(): void {
    if (!this.isPlaying || !this.oscillator) {
      return;
    }

    try {
      // Disconnect and stop oscillator
      this.oscillator.stop();
      this.oscillator.disconnect();
      this.oscillator = null;
      this.isPlaying = false;

      console.log('🔇 Test tone stopped');
    } catch (error) {
      console.error('Error stopping test tone:', error);
    }
  }

  /**
   * Check if test tone is currently playing
   */
  getIsPlaying(): boolean {
    return this.isPlaying;
  }

  /**
   * Set test tone volume (0.0 to 1.0)
   */
  setVolume(volume: number): void {
    this.gainNode.gain.value = Math.max(0, Math.min(1, volume));
  }

  /**
   * Cleanup resources
   */
  destroy(): void {
    this.stop();
    this.gainNode.disconnect();
  }
}
