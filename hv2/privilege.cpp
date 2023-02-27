#include "privilege.hpp"
#include "hv2.hpp"

#include <cstring>
#include <cstdio>
#include <cstdlib>

void hv2_privilege_up(hv2_t* cpu) {
    if (!cpu->pl) return;

    cpu->pl--;

    if (cpu->pl == 0) {
        if (cpu->cop4_ctrl & MMU_CTRL_DISABLE_ON_TPL0) {
            cpu->cop4_ctrl &= ~MMU_CTRL_ENABLE;
        }
    }

    if (cpu->pl == 1) {
        if (cpu->cop4_ctrl & MMU_CTRL_ENABLE_ON_TPL1) {
            cpu->cop4_ctrl |= MMU_CTRL_ENABLE;
        }
    }

    if (cpu->cop4_ctrl & MMU_CTRL_REMAP_ON_PLT) {
        cpu->cop4_i_cmap = cpu->pl;
    }
}

void hv2_privilege_down(hv2_t* cpu) {
    if (cpu->pl == 3) return;

    cpu->pl++;

    if (cpu->pl == 0) {
        if (cpu->cop4_ctrl & MMU_CTRL_DISABLE_ON_TPL0) {
            cpu->cop4_ctrl &= ~MMU_CTRL_ENABLE;
        }
    }

    if (cpu->pl == 1) {
        if (cpu->cop4_ctrl & MMU_CTRL_ENABLE_ON_TPL1) {
            cpu->cop4_ctrl |= MMU_CTRL_ENABLE;
        }
    }

    if (cpu->cop4_ctrl & MMU_CTRL_REMAP_ON_PLT) {
        cpu->cop4_i_cmap = cpu->pl;
    }
}