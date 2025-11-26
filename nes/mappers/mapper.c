#include "../cartridge.h"
#include "mapper.h"

#include "impl/nrom.h"
#include "impl/uxrom.h"

void mapper_init(struct Cartridge* cartridge) {
    switch (cartridge->mapper_type) {
        case Mapper_NRom:
            nrom_init(cartridge);
            break;
        case Mapper_UxRom:
            uxrom_init(cartridge);
            break;
        default:
            break;
    }
}

void mapper_free(struct Cartridge* cartridge) {
    switch (cartridge->mapper_type) {
        case Mapper_NRom:
            nrom_free(cartridge);
            break;
        case Mapper_UxRom:
            uxrom_free(cartridge);
            break;
        default:
            break;
    }
}

enum Mirroring mapper_get_mirroring(struct Cartridge* cartridge) {
    switch (cartridge->mapper_type) {
        default:
            return cartridge->mirroring;
    }
}

bool mapper_get_irq(struct Cartridge* cartridge) {
    switch (cartridge->mapper_type) {
        default:
            return false;
    }
}
void mapper_set_irq(struct Cartridge* cartridge, bool irq) {
    switch (cartridge->mapper_type) {
        default:
            break;
    }
}

void mapper_set_pc(struct Cartridge* cartridge, uint16_t pc) {
    switch (cartridge->mapper_type) {
        default:
            break;
    }
}

void mapper_decrement_scanline(struct Cartridge* cartridge) {
    switch (cartridge->mapper_type) {
        default:
            break;
    }
}

uint8_t mapper_cpu_read(const struct Cartridge* cartridge, uint16_t addr, bool* is_mapped) {
    switch (cartridge->mapper_type) {
        case Mapper_NRom:
            return nrom_cpu_read(cartridge, addr, is_mapped);
        case Mapper_UxRom:
            return uxrom_cpu_read(cartridge, addr, is_mapped);
        default:
            break;
    }

    *is_mapped = false;
    return 0;
}
void mapper_cpu_write(const struct Cartridge* cartridge, uint16_t addr, uint8_t val, bool* is_mapped) {
    switch (cartridge->mapper_type) {
        case Mapper_NRom:
            nrom_cpu_write(cartridge, addr, val, is_mapped);
            return;
        case Mapper_UxRom:
            uxrom_cpu_write(cartridge, addr, val, is_mapped);
            return;
        default:
            break;
    }

    *is_mapped = false;
}

uint8_t mapper_ppu_read(const struct Cartridge* cartridge, uint16_t addr, bool* is_mapped) {
    switch (cartridge->mapper_type) {
        default:
            break;
    }

    *is_mapped = false;
    return 0;
}
void mapper_ppu_write(const struct Cartridge* cartridge, uint16_t addr, uint8_t val, bool* is_mapped) {
    switch (cartridge->mapper_type) {
        default:
            break;
    }

    *is_mapped = false;
}