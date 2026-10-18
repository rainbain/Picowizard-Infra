/* Picowizard Assembler Quick Test
 *
 * Outputs each opcode and its combinations.
 *
 * Auther: Samuel Fitzsimons (rainbain)
 * File: test.S
 * Data: 2026
*/

#if PICOWIZARD_ISA_VERSION != 120
    #error "Test assembly only supports Picowizard ISA Version 1.2 and 1.2 Plus"
#endif

    MOV $r0, $r1
    MOV $r3, $r2
    MOV $r2, $r3
    MOV $r1, $r0

    // More registers on isa plus
#ifdef PICOWIZARD_ISA_PLUS
    MOV $r4, $r4
    MOV $r7, $r5
    MOV $r6, $r6
    MOV $r5, $r7
#endif


    ADD $r0, $r1
    ADD $r3, $r2
    ADD $r2, $r3
    ADD $r1, $r0

    ADC $r0, $r1
    ADC $r3, $r2
    ADC $r2, $r3
    ADC $r1, $r0

    NAND $r0, $r1
    NAND $r3, $r2
    NAND $r2, $r3
    NAND $r1, $r0

    XOR $r0, $r1
    XOR $r3, $r2
    XOR $r2, $r3
    XOR $r1, $r0

    LDA $r0, $r1
    LDA $r3, $r2
    LDA $r2, $r3
    LDA $r1, $r0

    STRA $r0, $r1
    STRA $r3, $r2
    STRA $r2, $r3
    STRA $r1, $r0

    BIZ $r0, $r1
    BIZ $r3, $r2
    BIZ $r2, $r3
    BIZ $r1, $r0

    JMP $r0, $r1
    JMP $r3, $r2
    JMP $r2, $r3
    JMP $r1, $r0

    LDI $r0, 0xDE
    LDI $r3, 0xAD
    LDI $r2, 0xBE
    LDI $r1, (FAR_OFF_LAND >> 8)

// In a far off land
.org (0xEF<<8)
FAR_OFF_LAND:
    .word 0x00112233
    .string "I am data <3\n"
