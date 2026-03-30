void Calculate_Measurements(void) {
    uint16_t max_adc = 0;
    uint16_t min_adc = 4095;
    int zero_crossings = 0;

    for (int i = 0; i < DISPLAY_BUF_LEN; i++) {
        if (display_buf[i] > max_adc) max_adc = display_buf[i];
        if (display_buf[i] < min_adc) min_adc = display_buf[i];

        // Zero-crossing (centered at 2048)
        if (i > 0) {
            //Only count rising edge crossing to get full cycles
            if (display_buf[i-1] < 2048 && display_buf[i] >= 2048) {
                zero_crossings++;
            }
        }
    }

    // Vpp Calculation
    v_max = ((float)max_adc / 4095.0f) * 3.3f;
    v_min = ((float)min_adc / 4095.0f) * 3.3f;
    v_pp = v_max - v_min;

    // Frequency Fix:
    if (zero_crossings > 0 && hdiv > 0) {
        //use 128 samples. If each sample is (hdiv/32) microseconds:
        float total_time_us = 128.0f * ((float)hdiv / 32.0f); 
        freq = (float)zero_crossings * (1000000.0f / total_time_us);
    } else {
        freq = 0;
    }
}