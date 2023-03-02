#pragma once

#include <cstdint>
#include <vector>

typedef std::vector <uint16_t> io_device_port_list_t;

class io_device_t {
public:
    virtual io_device_port_list_t* get_port_list() = 0;
    virtual uint32_t read(uint32_t, int) = 0;
    virtual void write(uint32_t, uint32_t, int) = 0;
};
