/* Picowizard Emulator CPU
 *
 * Main CPU emulation.
 *
 * Auther: Samuel Fitzsimons (rainbain)
 * File: cpu.h
 * Data: 2026
 * License: GPLv2 (See included license file)
*/

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef int (*cpu_op_t)(void* cpu_v, uint8_t opcode);

typedef uint8_t (*cpu_mem_read_t)(void* user, void* cpu_v, uint16_t addr);
typedef void (*cpu_mem_write_t)(void* user, void* cpu_v, uint16_t addr, uint8_t value);

typedef struct {
    // Bus handlers
    void* user;
    cpu_mem_read_t read;
    cpu_mem_write_t write;

    // The program counter
    uint16_t ctr;
    bool halted;

    // General perpause registers
    // A, B, C, SEQ, TA, TB, TC, TD
    uint8_t gpr[8];

    uint8_t cry; // Carry

    // Instruction Jump table
    cpu_op_t opcodes[256];
} cpu_t;

extern void cpu_initialize(cpu_t* cpu, bool plus, cpu_mem_read_t read, cpu_mem_write_t write, void* user);

extern void cpu_reset(cpu_t* cpu);

extern void cpu_tick(cpu_t* cpu, uint64_t cycles);