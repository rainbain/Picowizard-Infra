# Picowizard Assembler
Python based assembler for assembling code into picowizzard binaries.

This code is based on [PowerBlock's Wii/Gamecube Audio DSP Microcode Assembler](https://github.com/rainbain/PowerBlocks/tree/main/tools/dspasm)

Example usage:
`python3 picowizard_asm/asm_cli.py test.S -o out.bin`

## Supported ISA Versions
When assembling, the option `--isa` selects the ISA version to use.
Options:
```
1.2   - Picowizard 1.2
1.2P  - Picowizard 1.2P
```

Apart from enabling the appropriate instructions, the following definitions are added:

### PICOWIZARD_ISA_VERSION
An integer representation of the ISA. Like `120` for version 1.2

### PICOWIZARD_ISA_PLUS
Defined if this is the plus version.