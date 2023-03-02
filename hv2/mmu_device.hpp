#pragma once

#include <cstdint>

#define HV2_BYTE  0
#define HV2_SHORT 1
#define HV2_LONG  2
#define HV2_EXEC  3

struct hv2_range_t {
    uint32_t start, end;
};

class hv2_mmio_device_t {
public:
    virtual hv2_range_t get_physical_range() = 0;
    virtual uint32_t read(uint32_t, int) = 0;
    virtual void write(uint32_t, uint32_t, int) = 0;
    virtual void master_clock() {};
};