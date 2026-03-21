//
// Created by tromo on 6/4/25.
//

#ifndef AUTO_TESTS_H
#define AUTO_TESTS_H

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "../../dmg/dmg.h"
#include "../../dmg/cartridge.h"
#include "../../dmg/cpu.h"
#include "../json/cJSON.h"

constexpr int PRG_ROM_BANK_END = 0x8000;

#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_RESET   "\x1b[0m"

void run_auto_tests() {
    int successful_tests = 0;

    for (int opcode = 0; opcode <= 0xFF; ++opcode) {
        // Prefix, skipping for now
        if (opcode == 0xCB) {
            continue;
        }

        // opcodes that don't exist
        if (opcode == 0xD3 || opcode == 0xDB || opcode == 0xDD ||
            opcode == 0xE3 || opcode == 0xE4 || opcode == 0xEB || opcode == 0xEC || opcode == 0xED ||
            opcode == 0xF4 || opcode == 0xFC || opcode == 0xFD) {
            continue;
            }

        bool success = true;

        bool incorrect_registers = false;
        bool incorrect_memory = false;
        bool incorrect_cycles = false;

        char* file_name = malloc(sizeof(char) * 20);
        sprintf(file_name, "SingleStepTests/%02x.json", opcode);

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
            struct DMG* dmg = dmg_init_from_buffer(rom);

            const cJSON* c_name = child->child;
            const cJSON* c_initial_values = c_name->next;
            const cJSON* c_final_values = c_initial_values->next;
            const cJSON* c_cycles = c_final_values->next;

            const cJSON* c_pc = c_initial_values->child;
            const cJSON* c_sp = c_pc->next;
            const cJSON* c_a = c_sp->next;
            const cJSON* c_b = c_a->next;
            const cJSON* c_c = c_b->next;
            const cJSON* c_d = c_c->next;
            const cJSON* c_e = c_d->next;
            const cJSON* c_f = c_e->next;
            const cJSON* c_h = c_f->next;
            const cJSON* c_l = c_h->next;

            // Not yet used
            const cJSON* c_ime = c_l->next;
            const cJSON* c_ie = c_ime->next;

            const cJSON* c_ram = c_ie->next;

            const cJSON* c_ram_set = c_ram->child;
            while (c_ram_set) {
                const cJSON* c_ram_addr = c_ram_set->child;
                const cJSON* c_ram_val = c_ram_addr->next;

                dmg->cartridge->rom[c_ram_addr->valueint] = c_ram_val->valueint;

                c_ram_set = c_ram_set->next;
            }

            dmg->cpu->total_cycles = 0;
            dmg->cpu->PC = c_pc->valueint;
            dmg->cpu->SP = c_sp->valueint;
            dmg->cpu->A = c_a->valueint;
            dmg->cpu->B = c_b->valueint;
            dmg->cpu->C = c_c->valueint;
            dmg->cpu->D = c_d->valueint;
            dmg->cpu->E = c_e->valueint;
            dmg->cpu->F = c_f->valueint;
            dmg->cpu->H = c_h->valueint;
            dmg->cpu->L = c_l->valueint;

            dmg_cpu_fetch_next_instruction(dmg, dmg->cpu);
            auto current_cycle = c_cycles->child;
            while (current_cycle) {
                dmg_cpu_tick_m_cycle(dmg, dmg->cpu);

                // Todo: verify reads and writes

                current_cycle = current_cycle->next;
            }

            const cJSON* c_final_a = c_final_values->child;
            const cJSON* c_final_b = c_final_a->next;
            const cJSON* c_final_c = c_final_b->next;
            const cJSON* c_final_d = c_final_c->next;
            const cJSON* c_final_e = c_final_d->next;
            const cJSON* c_final_f = c_final_e->next;
            const cJSON* c_final_h = c_final_f->next;
            const cJSON* c_final_l = c_final_h->next;

            const cJSON* c_final_pc = c_final_l->next;
            const cJSON* c_final_sp = c_final_pc->next;

            const cJSON* c_final_ime = c_final_sp->next;

            const cJSON* c_final_ram = c_final_ime->next;

            success &= dmg->cpu->A == c_final_a->valueint;
            success &= dmg->cpu->B == c_final_b->valueint;
            success &= dmg->cpu->C == c_final_c->valueint;
            success &= dmg->cpu->D == c_final_d->valueint;
            success &= dmg->cpu->E == c_final_e->valueint;
            success &= dmg->cpu->F == c_final_f->valueint;
            success &= dmg->cpu->H == c_final_h->valueint;
            success &= dmg->cpu->L == c_final_l->valueint;
            success &= dmg->cpu->PC - 1 == c_final_pc->valueint; // -1 is to handle the fetch on the final M cycle of an instruction
            success &= dmg->cpu->SP == c_final_sp->valueint;

            if (!success)
                incorrect_registers = true;

            // assert(!incorrect_registers);

            const cJSON* c_final_ram_set = c_final_ram->child;
            while (c_final_ram_set) {
                const cJSON* c_final_ram_addr = c_final_ram_set->child;
                const cJSON* c_final_ram_val = c_final_ram_addr->next;

                success &= dmg->cartridge->rom[c_final_ram_addr->valueint] == c_final_ram_val->valueint;

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

            success &= dmg->cpu->total_cycles == total_cycle_count;
            if (!success)
                incorrect_cycles = true;

            // assert(!incorrect_cycles);

            dmg_free(dmg);
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
            printf(ANSI_COLOR_GREEN "0x%02x PASSED!\n" ANSI_COLOR_RESET, opcode);
            successful_tests++;
        }
        else {
            printf(ANSI_COLOR_RED "0x%02x FAILED!\n" ANSI_COLOR_RESET, opcode);
            printf("- Failed on test iteration %d\n", current_test_index);

            if (incorrect_registers)
                printf("- Register values are set wrong\n");

            if (incorrect_memory)
                printf("- Invalid memory values\n");

        }
        if (incorrect_cycles)
            printf(ANSI_COLOR_YELLOW "- Wrong number of cycles\n" ANSI_COLOR_RESET);
    }

    printf("Amount of successful tests: %d/256", successful_tests);
}

#endif //AUTO_TESTS_H
