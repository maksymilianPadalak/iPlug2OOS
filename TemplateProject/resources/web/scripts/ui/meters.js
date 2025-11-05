/**
 * Audio meter display
 */
/**
 * Update audio meter display
 */
export function updateMeter(channel, peak, rms) {
    const meterId = channel === 0 ? 'meterLeft' : 'meterRight';
    const meterEl = document.getElementById(meterId);
    if (!meterEl)
        return;
    // Convert linear value to dB and then to percentage
    const db = peak > 0.0001 ? 20 * Math.log10(peak) : -60;
    const percentage = Math.max(0, Math.min(100, ((db + 60) / 60) * 100));
    meterEl.style.height = percentage + '%';
}
window.updateMeter = updateMeter;
//# sourceMappingURL=meters.js.map