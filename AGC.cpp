#include "AGC.h"
#include <algorithm>
#include <iostream>

CAGC::CAGC() 
    : m_enabled(false)
    , m_gain(1.0f)
    , m_peak_env(0.5f)       // Start assuming "nominal" signal
    , m_target_level(0.5f)   // -6 dBFS (0.5) - Recovered from -20dBFS test
    , m_max_gain(3.0f)       // +9.5 dB Limit
{
    // Time constants
    // Attack: Very Fast to catch transients
    m_attack_coeff = 0.5f; 
    
    // Release: Slow
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
            // Attack phase: Instant track of peak to prevent clipping
            m_peak_env = abs_input;
        } else {
            // Release phase: Slow decay
            m_peak_env = (1.0f - m_release_coeff) * m_peak_env + m_release_coeff * abs_input;
        }
        
        // Prevent Divide by Zero
        float current_env = std::max(m_peak_env, 0.001f);

        // 3. Calculate Ideal Gain to hit Target Level
        // Gain = Target / Envelope
        float ideal_gain = m_target_level / current_env;

        // 4. Limit Gain (Don't boost silence infinitely)
        ideal_gain = std::min(ideal_gain, m_max_gain);
        
        // 5. Apply Gain Control
        if (ideal_gain < m_gain) {
             // Attack (Gain Reduction) - INSTANT to prevent clip
             m_gain = ideal_gain;
        } else {
             // Release (Gain Recovery) - Smoothed
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
