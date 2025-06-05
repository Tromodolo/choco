#include <stdlib.h>

#include "auto-tests.h"

#include <assert.h>
#include <stdio.h>

#include "../cartridge.h"
#include "../cpu.h"
#include "json/cJSON.h"
#include "../nes.h"

constexpr int TEST_HEADER_SIZE = 0x10;
constexpr int PRG_ROM_BANK_END = 0x8000;

#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_RESET   "\x1b[0m"

const char* instruction_names[] = {
    "BRK",
    "ORA",
    "KIL",
    "SLO",
    "NOP",
    "ORA",
    "ASL",
    "SLO",
    "PHP",
    "ORA",
    "ASL",
    "AAC",
    "NOP",
    "ORA",
    "ASL",
    "SLO",
    "BPL",
    "ORA",
    "KIL",
    "SLO",
    "NOP",
    "ORA",
    "ASL",
    "SLO",
    "CLC",
    "ORA",
    "NOP",
    "SLO",
    "NOP",
    "ORA",
    "ASL",
    "SLO",
    "JSR",
    "AND",
    "KIL",
    "RLA",
    "BIT",
    "AND",
    "ROL",
    "RLA",
    "PLP",
    "AND",
    "ROL",
    "AAC",
    "BIT",
    "AND",
    "ROL",
    "RLA",
    "BMI",
    "AND",
    "KIL",
    "RLA",
    "NOP",
    "AND",
    "ROL",
    "RLA",
    "SEC",
    "AND",
    "NOP",
    "RLA",
    "NOP",
    "AND",
    "ROL",
    "RLA",
    "RTI",
    "EOR",
    "KIL",
    "SRE",
    "NOP",
    "EOR",
    "LSR",
    "SRE",
    "PHA",
    "EOR",
    "LSR",
    "ASR",
    "JMP",
    "EOR",
    "LSR",
    "SRE",
    "BVC",
    "EOR",
    "KIL",
    "SRE",
    "NOP",
    "EOR",
    "LSR",
    "SRE",
    "CLI",
    "EOR",
    "NOP",
    "SRE",
    "NOP",
    "EOR",
    "LSR",
    "SRE",
    "RTS",
    "ADC",
    "KIL",
    "RRA",
    "NOP",
    "ADC",
    "ROR",
    "RRA",
    "PLA",
    "ADC",
    "ROR",
    "ARR",
    "JMP",
    "ADC",
    "ROR",
    "RRA",
    "BVS",
    "ADC",
    "KIL",
    "RRA",
    "NOP",
    "ADC",
    "ROR",
    "RRA",
    "SEI",
    "ADC",
    "NOP",
    "RRA",
    "NOP",
    "ADC",
    "ROR",
    "RRA",
    "NOP",
    "STA",
    "NOP",
    "SAX",
    "STY",
    "STA",
    "STX",
    "SAX",
    "DEY",
    "NOP",
    "TXA",
    "XAA",
    "STY",
    "STA",
    "STX",
    "SAX",
    "BCC",
    "STA",
    "KIL",
    "AXA",
    "STY",
    "STA",
    "STX",
    "SAX",
    "TYA",
    "STA",
    "TXS",
    "XAS",
    "SYA",
    "STA",
    "SXA",
    "AXA",
    "LDY",
    "LDA",
    "LDX",
    "LAX",
    "LDY",
    "LDA",
    "LDX",
    "LAX",
    "TAY",
    "LDA",
    "TAX",
    "ATX",
    "LDY",
    "LDA",
    "LDX",
    "LAX",
    "BCS",
    "LDA",
    "KIL",
    "LAX",
    "LDY",
    "LDA",
    "LDX",
    "LAX",
    "CLV",
    "LDA",
    "TSX",
    "LAR",
    "LDY",
    "LDA",
    "LDX",
    "LAX",
    "CPY",
    "CMP",
    "NOP",
    "DCP",
    "CPY",
    "CMP",
    "DEC",
    "DCP",
    "INY",
    "CMP",
    "DEX",
    "AXS",
    "CPY",
    "CMP",
    "DEC",
    "DCP",
    "BNE",
    "CMP",
    "KIL",
    "DCP",
    "NOP",
    "CMP",
    "DEC",
    "DCP",
    "CLD",
    "CMP",
    "NOP",
    "DCP",
    "NOP",
    "CMP",
    "DEC",
    "DCP",
    "CPX",
    "SBC",
    "NOP",
    "ISB",
    "CPX",
    "SBC",
    "INC",
    "ISB",
    "INX",
    "SBC",
    "NOP",
    "SBC",
    "CPX",
    "SBC",
    "INC",
    "ISB",
    "BEQ",
    "SBC",
    "KIL",
    "ISB",
    "NOP",
    "SBC",
    "INC",
    "ISB",
    "SED",
    "SBC",
    "NOP",
    "ISB",
    "NOP",
    "SBC",
    "INC",
    "ISB",
};

void run_auto_tests() {
    int successful_tests = 0;

    for (int opcode = 0; opcode <= 0xFF; ++opcode) {
        bool success = true;

        bool incorrect_registers = false;
        bool incorrect_memory = false;
        bool incorrect_cycles = false;

        char* file_name = malloc(sizeof(char) * 20);
        sprintf(file_name, "tomharte/%02x.json", opcode);

        FILE* file = fopen(file_name, "r");
        fseek(file, 0, SEEK_END);
        const long file_size = ftell(file);
        fseek(file, 0, SEEK_SET);

        const char* file_contents = malloc(file_size * sizeof(uint8_t));
        assert(file_contents);

        for(int i = 0; i < file_size; i++) {
            fread((void*)(file_contents + i), 1, 1, file);
        }

        cJSON* json = cJSON_Parse(file_contents);
        assert(json);

        free((void*)file_contents);
        free(file_name);

        // using calloc instead of malloc here to make sure its initialized to 0
        unsigned char* rom = calloc(0xFFFF, sizeof(unsigned char));

        int current_test_index = 0;
        const cJSON* child = json->child;
        while (child) {
            struct Nes* nes = nes_init_from_buffer(rom, 0xFFFF);

            const cJSON* c_name = child->child;
            const cJSON* c_initial_values = c_name->next;
            const cJSON* c_final_values = c_initial_values->next;
            const cJSON* c_cycles = c_final_values->next;

            const cJSON* c_pc = c_initial_values->child;
            const cJSON* c_sp = c_pc->next;
            const cJSON* c_acc = c_sp->next;
            const cJSON* c_x = c_acc->next;
            const cJSON* c_y = c_x->next;
            const cJSON* c_p = c_y->next;
            const cJSON* c_ram = c_p->next;

            const cJSON* c_ram_set = c_ram->child;
            while (c_ram_set) {
                const cJSON* c_ram_addr = c_ram_set->child;
                const cJSON* c_ram_val = c_ram_addr->next;

                nes->cartridge->prg_ram[c_ram_addr->valueint] = c_ram_val->valueint;

                c_ram_set = c_ram_set->next;
            }

            nes->cpu->total_cycles = 0;
            nes->cpu->pc = c_pc->valueint;
            nes->cpu->sp = c_sp->valueint;
            nes->cpu->acc = c_acc->valueint;
            nes->cpu->x = c_x->valueint;
            nes->cpu->y = c_y->valueint;
            nes->cpu->p.value = c_p->valueint;

            nes_cpu_tick(nes);

            const cJSON* c_final_pc = c_final_values->child;
            const cJSON* c_final_sp = c_final_pc->next;
            const cJSON* c_final_acc = c_final_sp->next;
            const cJSON* c_final_x = c_final_acc->next;
            const cJSON* c_final_y = c_final_x->next;
            const cJSON* c_final_p = c_final_y->next;
            const cJSON* c_final_ram = c_final_p->next;

            success &= nes->cpu->pc == c_final_pc->valueint;
            success &= nes->cpu->sp == c_final_sp->valueint;
            success &= nes->cpu->acc == c_final_acc->valueint;
            success &= nes->cpu->x == c_final_x->valueint;
            success &= nes->cpu->y == c_final_y->valueint;
            success &= nes->cpu->p.value == c_final_p->valueint;

            if (!success)
                incorrect_registers = true;

            // assert(!incorrect_registers);

            const cJSON* c_final_ram_set = c_final_ram->child;
            while (c_final_ram_set) {
                const cJSON* c_final_ram_addr = c_final_ram_set->child;
                const cJSON* c_final_ram_val = c_final_ram_addr->next;

                success &= nes->cartridge->prg_ram[c_final_ram_addr->valueint] == c_final_ram_val->valueint;

                if (!success)
                    incorrect_memory = true;

                // assert(!incorrect_memory);

                c_final_ram_set = c_final_ram_set->next;
            }

            const cJSON* c_cycles_child = c_cycles->child;
            int total_cycle_count = 0;
            while (c_cycles_child) {
                total_cycle_count++;
                c_cycles_child = c_cycles_child->next;
            }

            success &= nes->cpu->total_cycles == total_cycle_count;
            if (!success)
                incorrect_cycles = true;

            // assert(!incorrect_cycles);

            nes_free(nes);
            child = child->next;
            current_test_index++;

            success = !incorrect_registers && !incorrect_memory;
            if (!success)
                break;
        }

        free(rom);
        cJSON_Delete(json);
        fclose(file);

        if (success) {
            printf(ANSI_COLOR_GREEN "0x%02x %s PASSED!\n" ANSI_COLOR_RESET, opcode, instruction_names[opcode]);
            successful_tests++;
        }
        else {
            printf(ANSI_COLOR_RED "0x%02x %s FAILED!\n" ANSI_COLOR_RESET, opcode, instruction_names[opcode]);
            printf("- Failed on test iteration %d\n", current_test_index);

            if (incorrect_registers)
                printf("- Register values are set wrong\n");

            if (incorrect_memory)
                printf("- Invalid memory values\n");

        }
        if (incorrect_cycles)
            printf(ANSI_COLOR_YELLOW "- Wrong number of cycles\n" ANSI_COLOR_RESET);
    }

    printf("Amount of successful tests: %d/255", successful_tests);
}