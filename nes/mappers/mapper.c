#include "../cartridge.h"
#include "mapper.h"

#include "impl/mmc1.h"
#include "impl/nrom.h"
#include "impl/uxrom.h"
#include "impl/mmc3.h"

void mapper_init(struct Cartridge* cartridge, const char* file_path) {
    switch (cartridge->mapper_type) {
        case Mapper_NRom:
            nrom_init(cartridge);
            break;
        case Mapper_UxRom:
            uxrom_init(cartridge);
            break;
        case Mapper_MMC1:
            mmc1_init(cartridge, file_path);
            break;
        case Mapper_MMC3:
            mmc3_init(cartridge, file_path);
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
        case Mapper_MMC1:
            mmc1_free(cartridge);
            break;
        case Mapper_MMC3:
            mmc3_free(cartridge);
            break;
        default:
            break;
    }
}

enum Mirroring mapper_get_mirroring(struct Cartridge* cartridge) {
    switch (cartridge->mapper_type) {
        case Mapper_MMC1:
            return mmc1_get_mirroring(cartridge);
        case Mapper_MMC3:
            return mmc3_get_mirroring(cartridge);
        default:
            return cartridge->mirroring;
    }
}

bool mapper_get_irq(struct Cartridge* cartridge) {
    switch (cartridge->mapper_type) {
        case Mapper_MMC3:
            return mmc3_get_irq(cartridge);
        default:
            return false;
    }
}
void mapper_set_irq(struct Cartridge* cartridge, bool irq) {
    switch (cartridge->mapper_type) {
        case Mapper_MMC3:
            mmc3_set_irq(cartridge, irq);
            break;
        default:
            break;
    }
}

void mapper_set_pc(struct Cartridge* cartridge, uint16_t pc) {
    switch (cartridge->mapper_type) {
        case Mapper_MMC1:
            mmc1_set_pc(cartridge, pc);
            break;
        default:
            break;
    }
}

void mapper_decrement_scanline(struct Cartridge* cartridge) {
    switch (cartridge->mapper_type) {
        case Mapper_MMC3:
            mmc3_decrement_scanline(cartridge);
            break;
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
        case Mapper_MMC1:
            return mmc1_cpu_read(cartridge, addr, is_mapped);
        case Mapper_MMC3:
            return mmc3_cpu_read(cartridge, addr, is_mapped);
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
        case Mapper_MMC1:
            mmc1_cpu_write(cartridge, addr, val, is_mapped);
            return;
        case Mapper_MMC3:
            mmc3_cpu_write(cartridge, addr, val, is_mapped);
            return;
        default:
            break;
    }

    *is_mapped = false;
}

uint8_t mapper_ppu_read(const struct Cartridge* cartridge, uint16_t addr, bool* is_mapped) {
    switch (cartridge->mapper_type) {
        case Mapper_MMC1:
            return mmc1_ppu_read(cartridge, addr, is_mapped);
        case Mapper_MMC3:
            return mmc3_ppu_read(cartridge, addr, is_mapped);
        default:
            break;
    }

    *is_mapped = false;
    return 0;
}
void mapper_ppu_write(const struct Cartridge* cartridge, uint16_t addr, uint8_t val, bool* is_mapped) {
    switch (cartridge->mapper_type) {
        case Mapper_MMC1:
            mmc1_ppu_write(cartridge, addr, val, is_mapped);
            return;
        case Mapper_MMC3:
            mmc3_ppu_write(cartridge, addr, val, is_mapped);
        default:
            break;
    }

    *is_mapped = false;
}