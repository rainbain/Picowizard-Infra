/* Picowizard Emulator System
 *
 * System emulation with MMIO
 * 
 * Address Layout:
 * 0x0000 - 0x7FFF - ROM
 * 0x8000 - 0xEFFF - RAM
 * 0xF000 - 0xFFFF - MMIO
 * 
 * MMIO Layout:
 *   0xF000 HLT, Write to halt CPU
 *   0xF001 SER, Serial Write
 *   
 *
 * Auther: Samuel Fitzsimons (rainbain)
 * File: system.c
 * Data: 2026
 * License: GPLv2 (See included license file)
*/

#include "cpu.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static cpu_t cpu;
static uint8_t* memory;

static uint8_t* ROM;
static uint8_t* RAM;

static bool system_halted;

#define RAM_SIZE  0x8000
#define ROM_SIZE  0x7000
#define MMIO_SIZE 0x0002

#define ROM_ADDRESS  0x0000
#define RAM_ADDRESS  0x8000
#define MMIO_ADDRESS 0xF000

#define MMIO_HLT     0x000
#define MMIO_SER     0x001

static void out_of_bounds_msg(uint16_t addr, bool is_write) {
    printf("Out of bounds memory %s to address %04X\n", is_write ? "write" : "read", addr);
}

static uint8_t system_mmio_read(uint16_t addr) {
    (void)addr;
    return 0;
}

static void system_mmio_write(uint16_t addr, uint8_t value) {
    if(addr == MMIO_HLT) {
        system_halted = true;
    } else if(addr == MMIO_SER) {
        printf("%c", value);
    } else {
        printf("Invalid MMIO address %02X", addr);
    }
}

static uint8_t system_mem_read(void* user, void* cpu_v, uint16_t addr) {
    (void)user;
    (void)cpu_v;

    if(addr < RAM_ADDRESS) {
        addr -= ROM_ADDRESS;
        if(addr >= ROM_SIZE) {
            out_of_bounds_msg(addr, false);
            return 0;
        }

        return ROM[addr];
    } else if(addr < MMIO_ADDRESS) {
        addr -= RAM_ADDRESS;
        if(addr >= RAM_SIZE) {
            out_of_bounds_msg(addr, false);
            return 0;
        }

        return RAM[addr];
    } else {
        addr -= MMIO_ADDRESS;
        return system_mmio_read(addr);
    }
}

static void system_mem_write(void* user, void* cpu_v, uint16_t addr, uint8_t value) {
    (void)user;
    (void)cpu_v;

    if(addr < RAM_ADDRESS) {
        addr -= ROM_ADDRESS;
        out_of_bounds_msg(addr, true);
    } else if(addr < MMIO_ADDRESS) {
        addr -= RAM_ADDRESS;
        if(addr >= RAM_SIZE) {
            out_of_bounds_msg(addr, false);
            return;
        }

        RAM[addr] = value;
    } else {
        addr -= MMIO_ADDRESS;
        system_mmio_write(addr, value);
    }
}

int system_initialize(bool cpu_plus) {
    // Construct memory
    memory = malloc(RAM_SIZE + ROM_SIZE);
    if(memory == NULL) {
        printf("Failed to initialize system memory. Out of memory!\n");
        return -1;
    }

    // Clear it
    memset(memory, 0, RAM_SIZE + ROM_SIZE);

    // Setup points
    ROM = memory;
    RAM = memory + ROM_SIZE;

    // System state
    system_halted = false;

    // Setup CPU
    cpu_initialize(&cpu, cpu_plus, system_mem_read, system_mem_write, NULL);

    return 0;
}

void system_run() {
    while(true) {
        cpu_tick(&cpu, 1000);
        if(system_halted)
            return;
    }
}

void system_write_rom(const uint8_t* rom, size_t len) {
    if(len > ROM_SIZE) {
        printf("ROM file too big! Truncated from %d to %d bytes", (int)len, ROM_SIZE);
        len = ROM_SIZE;
    }

    memcpy(ROM, rom, len);
}