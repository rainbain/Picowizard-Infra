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
 * File: system.h
 * Data: 2026
 * License: GPLv2 (See included license file)
*/

#pragma once

#include <stdbool.h>
#include <stddef.h>

extern int system_initialize(bool cpu_plus);

extern void system_run();

extern void system_write_rom(const uint8_t* rom, size_t len);