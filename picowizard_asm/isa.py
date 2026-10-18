"""
Picowizard Assembler ISA

Defines the Instruction Set Architecture of the DSP.

Author: Samuel Fitzsimons (rainbain)
File: isa.py
Date: 2026
"""

from utils import TokenConsumer, assembly_error, name_token

def copy_bits(input, bits, error_token, error):
    output = 0

    width = len(bits)
    mask = (1 << width) - 1
    input &= mask

    for i in range(len(bits)):
        dst_bit = bits[i]
        src_bit = len(bits) - i - 1

        output |= ((input >> src_bit) & 1) << dst_bit
        input &= ~(1 << src_bit)
    
    # Bits left over, thats bad.
    if input != 0:
        assembly_error(error_token, error)
    
    return output


class RegisterField:
    def __init__(self, name, bits, register_listing):
        self.name = name
        self.bits = bits
        self.register_listing = register_listing
    
    def evaluate(self, tokens, error_token, _):
        # We only expect 1 token, the register
        if len(tokens) != 1 or tokens[0].type != "REGISTER":
            assembly_error(error_token, f"Expected register")
        
        # Find the register
        name = tokens[0].value.lower()
        register_index = None
        i = 0
        for register in self.register_listing:
            if name in register:
                register_index = i

            i += 1
        
        if register_index == None:
            assembly_error(error_token, f"Register {name} is not a valid register to operand of {error_token.value}")
        
        return copy_bits(register_index, self.bits, error_token, f"Could not fit register index of {name_token(error_token)} into instruction, this is usually a bug, please report it,")

class ImmediateField:
    def __init__(self, name, bits):
        self.name = name
        self.bits = bits
    
    def evaluate(self, tokens, error_token, program):
        # Evaluate expression
        if len(tokens) == 0:
            assembly_error(error_token, "Expected expression")
        
        value = program.evaluate_expression(tokens)

        return copy_bits(value, self.bits, error_token, f"Could not fit immediate into {name_token(error_token)}, make sure your immediate value is not too large")

class OpcodeData:
    def __init__(self, program, opcode, endian="big"):
        self.program = program
        self.opcode = opcode
        self.endian = endian
        self.extended_opcode = None
    
    def consume(self, token, consumer: TokenConsumer, extended: bool):
        # Dont consume if no arguments
        if extended and len(self.opcode.fields) == 0:
            arguments = []
        else:
            arguments = consumer.consume_list(["COLON", "NEWLINE"])

        # Token for throwing errors
        self.error_token = token

        if len(arguments) != len(self.opcode.fields):
            assembly_error(token, f"Opcode {self.opcode.name} expects {len(self.opcode.fields)} fields, got {len(arguments)}")
        
        # Reorder arguments
        reordered = [arguments[i] for i in self.opcode.field_order]
        
        self.arguments = reordered
    
    def value(self):
        output = self.opcode.constant

        # Resolve each field
        for i in range(len(self.opcode.fields)):
            # Resolve expression
            output |= self.opcode.fields[i].evaluate(self.arguments[i], self.error_token, self.program)

        # Extended Opcode
        if self.extended_opcode:
            if not self.opcode.extended_opcode:
                assembly_error(self.error_token, f"Instruction {self.opcode.name} does not support extension")

            ext_output = self.extended_opcode.value()

            output |= self.opcode.extended_opcode.evaluate(ext_output, self.error_token)
        return output
    
    def serialize(self, pc):
        output = self.value()

        # Byte 0 of the format string lives in bits 7..0, so emit LSB-first
        return bytearray((output >> (8 * i)) & 0xFF
                     for i in range(self.opcode.opcode_length))

    def length(self, pc):
        return self.opcode.opcode_length
    
        def set_extension(self, opcode):
            self.extended_opcode = opcode

class Opcode:
    def __init__(self, name: str, fmt: str, registers = [], field_order = []):
        self.name = name
        self.registers = registers
        self.field_order = field_order
        self.extended_opcode = None

        # Remove spaces from format string
        fmt = fmt.replace(" ", "")

        # Remove dots to
        fmt = fmt.replace(".", "")

        # Instruction must be a multiple of 8 bits.
        if len(fmt) % 8 != 0:
            raise RuntimeError(f"Opcode {self.name} does not have a format thats a multiple of 7 bits.")
    
        # 8 bit opcodes
        self.word_bits = 8
        
        self.opcode_length = len(fmt) // 8

        # Decode format
        self.constant = 0
        self.fields = []

        field_type = None
        field_bits = []

        for i in range(len(fmt)):
            c = fmt[i]

            # It goes in order word_bits-0, then word_bits-0, and so on
            word = i // self.word_bits
            bit = (word * self.word_bits) + (self.word_bits - 1 - (i % self.word_bits))
            if c == '0' or c == '1':
                self.constant |= int(c) << bit
            else:
                # Ony accumulate field after checking 1 or 0
                # To allow for constants interested in a field

                # Field Changed?
                if field_type != c:
                    # Flush
                    if field_type != None:
                        self.add_field(field_type, field_bits)
                    
                    # Begin a new
                    field_type = c
                    field_bits = []

                field_bits.append(bit)
        
        # Flush remaining
        if field_type != None:
            self.add_field(field_type, field_bits)
        
        # Make sure no register listings remain
        if len(self.registers) != 0:
            raise RuntimeError(f"Opcode {self.name} has incorrect number of register listings for the fields provided.")
        
        # Make sure each field has an argument order
        if len(self.field_order) != len(self.fields):
            raise RuntimeError(f"Opcode {self.name} does not have a field order for each field.")
    
    def pop_register_listing(self):
        if len(self.registers) == 0:
            raise RuntimeError(f"Opcode {self.name} has incorrect number of register listings for the fields provided.")
        
        # Grab the first entry (not the last one hints the zero.)
        return self.registers.pop(0)
    
    def add_field(self, type, bits):
        field = None
        if type in ['T', 'S', 'A', 'C']:
            field = RegisterField(type, bits, self.pop_register_listing())
        elif type in ['V']:
            field = ImmediateField(type, bits)
        else:
            raise RuntimeError(f"Unknown field type in opcode description \"{type}\"")
        
        self.fields.append(field)
    
    def instantiate(self, program, token, consumer: TokenConsumer, extended: bool):
        op = OpcodeData(program, self)
        op.consume(token, consumer, extended)
        return op

OPCODE_LISTING = {}

EXT_OPCODE_LISTING = {}

REGISTERS_BASE = [
    ["$r0"],
    ["$r1"],
    ["$r2"],
    ["$r3"]
]

REGISTERS_PICOWIZARD_PLUS = [
    ["$r4"],
    ["$r5"],
    ["$r6"],
    ["$r7"],
]

def add_opcode(opcode: Opcode):
    OPCODE_LISTING[opcode.name] = opcode

def add_picowizard_opcodes(plus: bool):
    REGISTERS = REGISTERS_BASE

    # Picowizard plus supports extra registers
    if plus:
        REGISTERS += REGISTERS_PICOWIZARD_PLUS

    # Plus ISA version dependent instructions
    if not plus:
        add_opcode(Opcode("MOV", "000T.T0SS", [REGISTERS, REGISTERS], [0, 1]))
    else:
        add_opcode(Opcode("MOV", "00TT.TSSS", [REGISTERS, REGISTERS], [0,1]))

    add_opcode(Opcode("ADD",  "010T.T0SS", [REGISTERS_BASE, REGISTERS_BASE], [0, 1]))
    add_opcode(Opcode("ADC",  "010T.T1SS", [REGISTERS_BASE, REGISTERS_BASE], [0, 1]))
    add_opcode(Opcode("NAND", "011T.T0SS", [REGISTERS_BASE, REGISTERS_BASE], [0, 1]))
    add_opcode(Opcode("XOR",  "011T.T1SS", [REGISTERS_BASE, REGISTERS_BASE], [0, 1]))
    add_opcode(Opcode("LDA",  "100T.T0AA", [REGISTERS_BASE, REGISTERS_BASE], [0, 1]))
    add_opcode(Opcode("STRA", "100S.S1AA", [REGISTERS_BASE, REGISTERS_BASE], [0, 1]))
    add_opcode(Opcode("BIZ",  "101C.C0AA", [REGISTERS_BASE, REGISTERS_BASE], [0, 1]))
    add_opcode(Opcode("JMP",  "101S.S1AA", [REGISTERS_BASE, REGISTERS_BASE], [0, 1]))
    add_opcode(Opcode("LDI",  "110T.T000 VVVV.VVVV", [REGISTERS_BASE], [0, 1]))