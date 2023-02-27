#include "exception.hpp"
#include "hv2.hpp"
#include "privilege.hpp"

#include <cstring>
#include <cstdio>
#include <cstdlib>

void hv2_exception(hv2_t* cpu, uint32_t cause) {
    hv2_privilege_up(cpu);

    cpu->cop0_xcause = cause;
    cpu->cop0_xpc = cpu->r[31];
    cpu->r[31] = cpu->cop0_xhaddr;

    hv2_flush(cpu, 31);
}
