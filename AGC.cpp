#include "AGC.h"
#include <algorithm>
#include <iostream>

CAGC::CAGC() 
    : m_enabled(false)
    , m_gain(1.0f)
    , m_peak_env(0.0f)
    , m_target_level(0.707f) // -3 dBFS (approx 23170 for 16-bit)
    , m_max_gain(4.0f)       // +12 dB
{
    // Time constants for 8kHz sample rate
    // Attack: Fast (e.g. 10ms) -> coeff ~= 0.8
    // Release: Slow (e.g. 500ms) -> coeff ~= 0.0005 per sample
    
    // Simplified per-block logic or per-sample? 
    // Let's do per-sample for smooth envelope tracking, but efficiency matters.
    // Given 160-sample blocks (20ms), per-sample is fine.
    
    // Attack (reduce gain quickly to prevent clipping)
    m_attack_coeff = 0.1f; 
    
    // Release (increase gain slowly)
    m_release_coeff = 0.0002f;
}

void CAGC::Process(int16_t* samples, size_t count)
{
    if (!m_enabled) return;

    for (size_t i = 0; i < count; ++i)
    {
        // 1. Get simple absolute value normalized to 0..1 range
        float input = samples[i] / 32768.0f;
        float abs_input = std::abs(input);

        // 2. Track Audio Envelope (Peak Detector)
        if (abs_input > m_peak_env) {
            // Attack phase (signal getting louder)
            m_peak_env = (1.0f - m_attack_coeff) * m_peak_env + m_attack_coeff * abs_input;
        } else {
            // Release phase (signal getting quieter)
            m_peak_env = (1.0f - m_release_coeff) * m_peak_env + m_release_coeff * abs_input;
        }
        
        // Prevent Divide by Zero
        float current_env = std::max(m_peak_env, 0.001f);

        // 3. Calculate Ideal Gain to hit Target Level
        // Gain = Target / Envelope
        float ideal_gain = m_target_level / current_env;

        // 4. Limit Gain (Don't boost silence infinitely)
        ideal_gain = std::min(ideal_gain, m_max_gain);
        
        // 5. Apply Smoothing to Gain changes (prevent clicking)
        // Simple one-pole filter on the gain itself
        if (ideal_gain < m_gain) {
             // Attack (Gain Reduction) - Fast
             m_gain = 0.9f * m_gain + 0.1f * ideal_gain;
        } else {
             // Release (Gain Recovery) - Slow
             m_gain = 0.9995f * m_gain + 0.0005f * ideal_gain;
        }

        // 6. Apply Gain
        float output = input * m_gain;

        // 7. Hard Limiter (Safety Clipping protection)
        if (output > 1.0f) output = 1.0f;
        if (output < -1.0f) output = -1.0f;

        // 8. Output
        samples[i] = int16_t(output * 32767.0f);
    }
}
