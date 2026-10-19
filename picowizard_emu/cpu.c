/* Picowizard Emulator CPU
 *
 * Main CPU emulation.
 * 
 * Known Bugs:
 * A LDI placed at 0xFFFF will read from 0x0000 before halting.
 *
 * Auther: Samuel Fitzsimons (rainbain)
 * File: cpu.c
 * Data: 2026
 * License: GPLv2 (See included license file)
*/

#include "cpu.h"
#include <stdio.h>
#include <string.h>

#define OPCODE_PREAMBLE_IMM0() \
    cpu_t* cpu = (cpu_t*)cpu_v;

#define OPCODE_PREAMBLE_IMM1() \
    cpu_t* cpu = (cpu_t*)cpu_v; \
    uint8_t imm0 = (opcode >> 3) & 0b11;

#define OPCODE_PREAMBLE_IMM2() \
    cpu_t* cpu = (cpu_t*)cpu_v; \
    uint8_t imm0 = (opcode >> 3) & 0b11; \
    uint8_t imm1 = opcode & 0b11;

#define OPCODE_PREAMBLE_PLUS_IMM2() \
    cpu_t* cpu = (cpu_t*)cpu_v; \
    uint8_t imm0 = (opcode >> 3) & 0b111; \
    uint8_t imm1 = opcode & 0b111;

static int cpu_op_invalid(void* cpu_v, uint8_t opcode) {
    OPCODE_PREAMBLE_IMM0();

    printf("Invalid opcode %02X encountered at %04X", opcode, cpu->ctr);
    return 1;
}

static int cpu_op_mov(void* cpu_v, uint8_t opcode) {
    OPCODE_PREAMBLE_IMM2();
    cpu->gpr[imm0] = cpu->gpr[imm1];
    return 1;
}

static int cpu_op_plus_mov(void* cpu_v, uint8_t opcode) {
    OPCODE_PREAMBLE_PLUS_IMM2();
    cpu->gpr[imm0] = cpu->gpr[imm1];
    return 1;
}

static int cpu_op_add(void* cpu_v, uint8_t opcode) {
    OPCODE_PREAMBLE_IMM2();
    uint16_t sum = (uint16_t)cpu->gpr[imm0] + cpu->gpr[imm1];
    
    cpu->gpr[imm0] = sum & 0xFF;
    cpu->cry = sum >> 8;
    return 1;
}

static int cpu_op_adc(void* cpu_v, uint8_t opcode) {
    OPCODE_PREAMBLE_IMM2();
    uint16_t sum = (uint16_t)cpu->gpr[imm0] + cpu->gpr[imm1] + cpu->cry;

    cpu->gpr[imm0] = sum & 0xFF;
    cpu->cry = sum >> 8;
    return 1;
}

static int cpu_op_nand(void* cpu_v, uint8_t opcode) {
    OPCODE_PREAMBLE_IMM2();
    cpu->gpr[imm0] = ~(cpu->gpr[imm0] & cpu->gpr[imm1]);
    cpu->cry = 1;
    return 1;
}

static int cpu_op_xor(void* cpu_v, uint8_t opcode) {
    OPCODE_PREAMBLE_IMM2();
    cpu->gpr[imm0] ^= cpu->gpr[imm1];
    cpu->cry = 1;
    return 1;
}

static int cpu_op_lda(void* cpu_v, uint8_t opcode) {
    OPCODE_PREAMBLE_IMM2();
    uint16_t seg = (uint16_t)cpu->gpr[3] << 8;
    cpu->gpr[imm0] = cpu->read(cpu->user, cpu_v, seg | cpu->gpr[imm1]);
    return 1;
}

static int cpu_op_stra(void* cpu_v, uint8_t opcode) {
    OPCODE_PREAMBLE_IMM2();
    uint16_t seg = (uint16_t)cpu->gpr[3] << 8;
    cpu->write(cpu->user, cpu_v, seg | cpu->gpr[imm1], cpu->gpr[imm0]);
    return 1;
}

static int cpu_op_biz(void* cpu_v, uint8_t opcode) {
    OPCODE_PREAMBLE_IMM2();
    if(cpu->gpr[imm0])
        return 1;

    // Set new PC
    uint16_t ret = cpu->ctr + 1;
    uint16_t seg = (uint16_t)cpu->gpr[3] << 8;
    cpu->ctr = seg | cpu->gpr[imm1];

    cpu->gpr[2] = ret & 0xFF;  // C   = low byte
    cpu->gpr[3] = ret >> 8;    // SEG = high byte
    return 0;
}

static int cpu_op_jmp(void* cpu_v, uint8_t opcode) {
    OPCODE_PREAMBLE_IMM2();
    uint16_t target = ((uint16_t)cpu->gpr[imm0] << 8) | cpu->gpr[imm1];
    cpu->ctr = target;
    return 0;
}

static int cpu_op_ldi(void* cpu_v, uint8_t opcode) {
    OPCODE_PREAMBLE_IMM1();

    cpu->gpr[imm0] = cpu->read(cpu->user, cpu_v, cpu->ctr + 1);
    return 2;
}

static void cpu_construct_opcode(cpu_t* cpu, cpu_op_t op_func, uint8_t opcode, int imm0_range, int imm1_range) {
    for(int imm0 = 0; imm0 <= imm0_range; imm0++) {
        for(int imm1 = 0; imm1 <= imm1_range; imm1++) {
            uint8_t c = ((imm0 << 3) | imm1) | opcode;

            cpu->opcodes[c] = op_func;
        }
    }
}

void cpu_initialize(cpu_t* cpu, bool plus, cpu_mem_read_t read, cpu_mem_write_t write, void* user) {
    //
    // Construct opcode tables
    for(int i = 0; i < 256; i++) {
        cpu->opcodes[i] = cpu_op_invalid;
    }

    if(!plus) {
        cpu_construct_opcode(cpu, cpu_op_mov, 0b00000000, 3, 3);
    } else {
        cpu_construct_opcode(cpu, cpu_op_plus_mov, 0b00000000, 7, 7);
    }

    cpu_construct_opcode(cpu, cpu_op_add,  0b01000000, 3, 3);
    cpu_construct_opcode(cpu, cpu_op_adc,  0b01000100, 3, 3);
    cpu_construct_opcode(cpu, cpu_op_nand, 0b01100000, 3, 3);
    cpu_construct_opcode(cpu, cpu_op_xor,  0b01100100, 3, 3);
    cpu_construct_opcode(cpu, cpu_op_lda,  0b10000000, 3, 3);
    cpu_construct_opcode(cpu, cpu_op_stra, 0b10000100, 3, 3);
    cpu_construct_opcode(cpu, cpu_op_biz,  0b10100000, 3, 3);
    cpu_construct_opcode(cpu, cpu_op_jmp,  0b10100100, 3, 3);
    cpu_construct_opcode(cpu, cpu_op_ldi,  0b11000000, 3, 0);

    //
    // Setup bus helpers
    cpu->read = read;
    cpu->write = write;
    cpu->user = user;

    // Clear registers. Only done at power on, not after reset
    memset(cpu->gpr, 0, sizeof(cpu->gpr));
    cpu->cry = 0;
    
    // Reset program counter now
    cpu_reset(cpu);
}

void cpu_reset(cpu_t* cpu) {
    cpu->halted = false;
}

void cpu_tick(cpu_t* cpu, uint64_t cycles) {
    for(uint64_t i = 0; i < cycles && !cpu->halted; i++) {
        uint8_t op = cpu->read(cpu->user, cpu, cpu->ctr);

        uint32_t next = (uint32_t)cpu->ctr + cpu->opcodes[op](cpu, op);
        if(next > 0xFFFF) {
            cpu->halted = true;
            break;
        }
        cpu->ctr = (uint16_t)next;
    }
}