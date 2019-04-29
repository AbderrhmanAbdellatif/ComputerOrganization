import argparse
import re
import pdb

DEBUG = 0
SYMBOL_TABLE = {}
VALID_OPCODES = {'add':'r', 
        'nand':'r', 
        'addi':'i', 
        'lw':'i', 
        'sw':'i', 
        'beq':'i', 
        'jalr':'j', 
        'spop':'s',
        'la': 'pseudo',
        'noop': 'pseudo',
        '.word': 'pseudo',
        'halt': 'pseudo'}
VALID_REGISTERS = ['zero', 'at', 'v0',
        'a0', 'a1', 'a2',
        't0', 't1', 't2',
        's0', 's1', 's2',
        'k0', 'sp', 'fp', 'ra']
BINARY_OPCODES = {'add':'000',
        'nand':'001',
        'addi':'010',
        'lw':'011',
        'sw':'100',
        'beq':'101',
        'jalr':'110',
        'spop':'111'}
BINARY_REGISTERS = {'zero':'0000', 'at':'0001', 'v0':'0010', 'a0':'0011', 
        'a1':'0100', 'a2':'0101', 't0':'0110', 't1':'0111', 
        't2':'1000', 's0':'1001', 's1':'1010', 's2':'1011',
        'k0':'1100', 'sp':'1101', 'fp':'1110', 'ra':'1111'}

pc = 0

def dbg(s):
    if DEBUG:
        print s

def imm5(off):
    # if immediate value provided return binary value without leading '0b'
    if type(off) is str:
        hex_regex = '[+-]?0x[0-9a-f]+'
        bin_regex = '[+-]?0b[0-1]+'
        if re.match(hex_regex, off):
            num = int(off, 16)
        elif re.match(bin_regex, off):
            num = int(off, 2)
        else:
            num = int(off)
    elif type(off) is int:
        num = off
    else:
        print "imm5 offset error: {}".format(off)
        exit(1)

    if num < -16 or 16 <= num:
        print "Given offset is greater than 5 bit signed offset field!"
        exit(1)
    return format(num if num >= 0 else (1 << 5) + num, '05b')

def is_blank(line):
    comment_regex = '^\s*(!.*)?'
    # want to check if the regex matches from beginning of line, so use re.match
    if re.match(comment_regex, line).group(1) or line.isspace():
        return True
    else:
        return False
    
def get_label_and_op(line):
    # match for optional label and opcode 
    opcode_regex = '^\s*((\w+):)?\s*(\.?[\w]+)?'
    m = re.match(opcode_regex, line)
    try:
        # return the label and opcode
        return m.group(2), m.group(3)
    except:
        return None
    
def parse_R(line):
    R_type_regex = '^\s*(\w+:)?\s*(\w+)\s+\$(\w+),\s*\$(\w+),\s*\$(\w+)\s*(!.*)?'
    m = re.match(R_type_regex, line)
    try:
        label = m.group(1)
        op = m.group(2)
        rx = m.group(3)
        ry = m.group(4)
        rz = m.group(5)
        comment = m.group(6)
        return op, rx, ry, rz
    except:
        return None

def parse_I(line):
    I_type_regex = '^\s*(\w+:)?\s*(\w+)\s+\$(\w+),\s*\$(\w+),\s*([+-]?\w+)\s*(\!.*)?'
    MEM_type_regex = '^\s*(\w+:)?\s*(\w+)\s+\$(\w+),\s*([+-]?[\w$()]+)\s*(!.*)?'

    label, op = get_label_and_op(line)
    if op is not None:
        # MEM instructions have different formats
        if op in ['lw', 'sw']:
            m = re.match(MEM_type_regex, line)
            label = m.group(1)
            op = m.group(2)
            rx = m.group(3)
            reg_and_off = m.group(4)
            reg_and_off_m = re.match('^([+-]?0x[0-9a-f]+|[+-]?\d+)\(\$(\w+)\)', reg_and_off)
            off = reg_and_off_m.group(1)
            ry = reg_and_off_m.group(2)
            return op, rx, ry, off
        else:
            m = re.match(I_type_regex, line)
            label = m.group(1)
            op = m.group(2)
            rx = m.group(3)
            ry = m.group(4)
            off = m.group(5)
            return op, rx, ry, off
    else:
        return None

def parse_J(line):
    J_type_regex = '^\s*(\w+:)?\s*(\w+)\s+\$(\w+),\s*\$(\w+)\s*(!.*)?'

    m = re.match(J_type_regex, line)
    try:
        label = m.group(1)
        op = m.group(2)
        rx = m.group(3)
        ry = m.group(4)
        comment = m.group(5)
        return op, rx, ry
    except:
        return None

def parse_la(line):
    la_regex = '^\s*(\w+:)?\s*(\w+)\s+\$(\w+),\s*(\w+)\s*(!.*)?'

    m = re.match(la_regex, line)
    try:
        label = m.group(1)
        op = m.group(2)
        rx = m.group(3)
        jump_label = m.group(4)
        comment = m.group(5)
        return op, rx, jump_label
    except:
        return None

def parse_S(line):
    S_type_regex = '^\s*(\w+:)?\s*(\w+)\s+(\w+)\s*(\!.*)?'

    m = re.match(S_type_regex, line)
    try:
        label = m.group(1)
        op = m.group(2)
        control_code = m.group(3)
        comment = m.group(4)
        return op, control_code
    except:
        return None

# Pass 1: create symbol table for labels
def pass1(f):
    dbg("Pass 1:\n")
    # use a program counter to keep track of addresses in the file
    global pc
    pc = 0

    for line in f:
        # skip comments and blank lines
        if is_blank(line):
            continue
        else:
            dbg('{}: {}'.format(pc, line[:-1]))
            line = line.lower()

            try:
                label, op = get_label_and_op(line)
                if label:
                    SYMBOL_TABLE[label] = pc
                    if op is None:
                        continue
                if op == 'la':
                    pc += 3
                pc += 1
            except:
                print "Invalid line: \n\t{}".format(line)

# Pass 2: Generate machine code for instructions
def pass2(f, outfile):
    dbg("\nPass 2:")
    global pc
    pc = 0

    # Rewind to begining of file
    f.seek(0)

    for line in f:
        # skip comments and blank lines
        if is_blank(line):
            continue

        dbg('{}: {}'.format(pc, line[:-1]))
        line = line.lower()
        
        label, op = get_label_and_op(line)
        if op in VALID_OPCODES:
            assemble_line(outfile, line)
        elif op is None:
            continue
        else:
            print "\tInvalid opcode or pseudo-op: \n\t\t{}".format(line)
            exit(1)

        if op == 'la':
            pc += 3
        pc += 1
        
def assemble_line(f, line):
    label, op = get_label_and_op(line)
    try:
        # R type instruction
        if VALID_OPCODES[op] == 'r':
            op, rx, ry, rz = parse_R(line)

            bin_op = BINARY_OPCODES[op]
            bin_rx = BINARY_REGISTERS[rx]
            bin_ry = BINARY_REGISTERS[ry]
            bin_rz = BINARY_REGISTERS[rz]
            unused = '0'

            bin_instr = bin_op + bin_rx + bin_ry + bin_rz + unused
            dbg(bin_instr)
            f.write(bin_instr + '\n')

        # I type instruction
        elif VALID_OPCODES[op] == 'i':
            op, rx, ry, off = parse_I(line)

            bin_op = BINARY_OPCODES[op]
            bin_rx = BINARY_REGISTERS[rx]
            bin_ry = BINARY_REGISTERS[ry]
            if op == 'beq':
                try:
                    # assume they are using beq with a label
                    bin_off = imm5(SYMBOL_TABLE[off] - (pc + 1))
                except:
                    # but if they are not, try it with a numerical offset
                    bin_off = imm5(off)
            else:
                bin_off = imm5(off)

            bin_instr = bin_op + bin_rx + bin_ry + bin_off
            dbg(bin_instr)
            f.write(bin_instr + '\n')

        # J type instruction
        elif VALID_OPCODES[op] == 'j':
            op, rx, ry = parse_J(line)

            bin_op = BINARY_OPCODES[op]
            bin_rx = BINARY_REGISTERS[rx]
            bin_ry = BINARY_REGISTERS[ry]
            unused = '0'*5

            bin_instr = bin_op + bin_rx + bin_ry + unused
            dbg(bin_instr)
            f.write(bin_instr + '\n')
            
        # S type instruction
        elif VALID_OPCODES[op] == 's':
            op, control_code = parse_S(line)

            bin_op = '111'
            unused = '0'*11
            cc = format(int(control_code), '02b')

            bin_instr = bin_op + unused + cc
            dbg(bin_instr)
            f.write(bin_instr + '\n')

        # Pseudo-instructions
        elif VALID_OPCODES[op] == 'pseudo':
            if op == 'noop':
                bin_instr = '0'*16
                dbg(bin_instr)
                f.write(bin_instr + '\n')
            elif op == '.word':
                word_regex = '^(\w+:)?\s*\.word\s+([+-]?[0-9a-fx]+)\s*(\!.*)?'
                word_regex_m = re.match(word_regex, line)
                value = word_regex_m.group(2)

                hex_regex = '[+-]?0x[0-9a-f]+'
                bin_regex = '[+-]?0b[01]+'
                if re.match(hex_regex, value):
                    value = int(value, 16)
                elif re.match(bin_regex, value):
                    value = int(value, 2)
                elif value.isdigit():
                    value = int(value)
                else:
                    raise ValueError
                bin_instr = bin(value)[2:].zfill(16)
                dbg(bin_instr)
                f.write(bin_instr + '\n')
            elif op == 'halt':
                bin_op = '111'
                unused = '0'*11
                cc = '0'*2

                bin_instr = bin_op + unused + cc
                dbg(bin_instr)
                f.write(bin_instr + '\n')
            elif op == 'la':
                # la =  jalr rx, rx     - to get current pc
                #       lw rx, 2(rx)    - to load the word hardcoded_label_value
                #       beq $zero, $zero, x0001
                #                       - to jump to the next instruction after label value
                #       .word hardcoded_label_value
            
                op, rx, jump_label = parse_la(line)

                bin_op = BINARY_OPCODES['jalr']
                bin_rx = BINARY_REGISTERS[rx]
                bin_ry = bin_rx
                unused = '0'*5
                jalr_instr = bin_op + bin_rx + bin_ry + unused
                dbg(jalr_instr)
                f.write(jalr_instr + '\n')

                bin_op = BINARY_OPCODES['lw']
                bin_rx = bin_rx
                bin_ry = bin_ry
                bin_offset = '00010'
                lw_instr = bin_op + bin_rx + bin_ry + bin_offset
                dbg(lw_instr)
                f.write(lw_instr + '\n')

                bin_op = BINARY_OPCODES['beq']
                bin_rx = BINARY_REGISTERS['zero']
                bin_ry = BINARY_REGISTERS['zero']
                offset = '00001'
                beq_instr = bin_op + bin_rx + bin_ry + offset
                dbg(beq_instr)
                f.write(beq_instr + '\n')
                
                label_offset = '{0:016b}'.format(SYMBOL_TABLE[jump_label])
                dbg(label_offset)
                f.write(label_offset + '\n')
    except KeyError:
        print "Invalid opcode, register, or label! \n\t{}".format(line)
        exit(1)
    except ValueError:
        print "Invalid offset! \n\t{}".format(line)
        exit(1)
    except:
        print "Error parsing line: \n\t{}".format(line)
        exit(1)

def print_symbol_table(args):
    if DEBUG:
        print "\nPrinting Symbol Table: "
        for symbol in SYMBOL_TABLE:
            print "{}: {}".format(symbol, SYMBOL_TABLE[symbol])
    
    symfile = args.asmfile.split('.')
    symfile = symfile[0] + '.sym'
    with open(symfile, 'w') as f:
        for symbol in SYMBOL_TABLE:
            f.write('{}: {}\n'.format(symbol, SYMBOL_TABLE[symbol]))

def print_logisim_file(binary_file):
    hexfile = binary_file[:-4] + '.hex'
    print "Printing logisim memory image to {}...".format(hexfile)
    with open(binary_file, 'r') as f:
        with open(hexfile, 'w') as g:
            for line in f:
                g.write('{:04X} '.format(int(line.rstrip(), 2)))
    print "Complete."

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("asmfile", help=".s file to be assembled")
    parser.add_argument("--debug", help="toggle debug mode", action="store_true")
    parser.add_argument("--logisim", help="output logisim compatible RAM image", 
            action="store_true")
    args = parser.parse_args()
    print "loading {}...\n".format(args.asmfile)

    if args.debug:
        DEBUG = 1

    with open(args.asmfile, 'r') as f:
        pass1(f)
        print_symbol_table(args)
        # create output binary file
        outfile = args.asmfile.split('.')
        outfile = outfile[0] + '.bin'

        print "writing to {}...".format(outfile)
        hexfile = open(outfile, 'w')
        pass2(f, hexfile)
        hexfile.close()

    print "Assemble complete.\n"

    if args.logisim:
        print_logisim_file(outfile)
