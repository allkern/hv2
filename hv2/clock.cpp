#include "clock.hpp"

hv2_clock_t* hv2_clock_create() {
    return new hv2_clock_t;
}

void hv2_clock_init(hv2_clock_t* clk, float freq, float master_freq) {
    clk->freq = freq;
    clk->master_freq = master_freq;
}

bool hv2_clock_tick(hv2_clock_t* clk) {
    float ratio = clk->master_freq / clk->freq;

    if (clk->cycles_elapsed < ratio) {
        clk->cycles_elapsed += 1.0;
    } else {
        clk->cycles_elapsed -= ratio;

        return true;
    }

    return false;
}