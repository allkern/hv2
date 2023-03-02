#pragma once

struct hv2_clock_t {
    float master_freq;
    float freq;
    float cycles_elapsed;
};

hv2_clock_t* hv2_clock_create();
void hv2_clock_init(hv2_clock_t*, float, float);
bool hv2_clock_tick(hv2_clock_t*);