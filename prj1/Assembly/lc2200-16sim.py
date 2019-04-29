import argparse
import struct
import pdb
import re

PC = 0
MEM = {}
REGS = {0:0, 1:0, 2:0, 3:0, 4:0, 5:0, 6:0, 7:0,
        8:0, 9:0, 10:0, 11:0, 12:0, 13:0, 14:0, 15:0}

OP_TYPE = {'000':'r', '001':'r', '010':'i', '011':'i',
           '100':'i', '101':'i', '110':'j', '111':'s'}
OP_NAME = {'000':'add', '001':'nand', '010':'addi', '011':'lw',
           '100':'sw', '101':'beq', '110':'jalr', '111':'spop'}
REG_NAME = {'0000':'$zero', '0001':'$at', '0010':'$v0', '0011':'$a0',
           '0100':'$a1', '0101':'$a2', '0110':'$t0', '0111':'$t1',
           '1000':'$t2', '1001':'$s0', '1010':'$s1', '1011':'$s2',
           '1100':'$k0', '1101':'$sp', '1110':'$fp', '1111':'$ra'}
BREAKPOINTS = []
LABELS = {}

HALTED = False
QUIT = False

def imm5(num):
    if (1<<4) & num:
        return num - (1<<5)
    else:
        return num

def imm16(num):
    if (1<<15) & num:
        return num - (1<<16)
    else:
        return num

def load_program(runfile):
    addr = 0
    with open(runfile, 'r') as f:
        for line in f:
            # parse binary string into integer and strip newline
            MEM[addr] = int(line[:-1], 2)
            addr += 1

    symfile = runfile.split('.')
    symfile = symfile[0] + '.sym'
    with open(symfile, 'r') as f:
        for line in f:
            symbol_regex = '(\w+): (\d+)'
            symbol_m = re.match(symbol_regex, line)
            label = symbol_m.group(1)
            addr = int(symbol_m.group(2))
            LABELS[addr] = label

def access_mem(addr):
    try:
        return MEM[addr]
    except KeyError:
        return 0

def print_sim_state():
    global PC
    print " ze {:04X} {:<10} a1 {:04X} {:<10} t2 {:04X} {:<10} k0 {:04X} {:<10}".format(
            REGS[0], reg_contents(0), REGS[4], reg_contents(4), 
            REGS[8], reg_contents(8), REGS[12], reg_contents(12))
    print " at {:04X} {:<10} a2 {:04X} {:<10} s0 {:04X} {:<10} sp {:04X} {:<10}".format(
            REGS[1], reg_contents(1), REGS[5], reg_contents(5), 
            REGS[9], reg_contents(9), REGS[13], reg_contents(13))
    print " v0 {:04X} {:<10} t0 {:04X} {:<10} s1 {:04X} {:<10} fp {:04X} {:<10}".format(
            REGS[2], reg_contents(2), REGS[6], reg_contents(6), 
            REGS[10], reg_contents(10), REGS[14], reg_contents(14))
    print " a0 {:04X} {:<10} t1 {:04X} {:<10} s2 {:04X} {:<10} ra {:04X} {:<10}".format(
            REGS[3], reg_contents(3), REGS[7], reg_contents(7), 
            REGS[11], reg_contents(11), REGS[15], reg_contents(15))
    print " - Now at x{:04X}: {}".format(PC, print_instruction())

def reg_contents(i):
    try:
        return '{}'.format(LABELS[REGS[i]])
    except:
        return '{}'.format(imm16(REGS[i]))

def print_instruction():
    global PC
    instruction = access_mem(PC)
    IREG = format(instruction, '016b')
    op = IREG[0:3]

    if IREG == '0000000000000000':
        return 'noop'

    if OP_TYPE[op] == 'r':
        rx = IREG[3:7]
        ry = IREG[7:11]
        rz = IREG[11:15]
        return '{} {}, {}, {}'.format(OP_NAME[op], REG_NAME[rx], REG_NAME[ry], REG_NAME[rz])
    elif OP_TYPE[op] == 'i':
        rx = IREG[3:7]
        ry = IREG[7:11]
        off = IREG[11:]
        if OP_NAME[op] in ['lw', 'sw']:
            return '{0} {1}, #{3}({2})'.format(OP_NAME[op], 
                                               REG_NAME[rx], REG_NAME[ry], 
                                               imm5(int(off, 2)))
        elif OP_NAME[op] == 'beq':
            return '{} {}, {}, x{:04X}'.format(OP_NAME[op], 
                                          REG_NAME[rx], REG_NAME[ry],
                                          imm5(int(off, 2)))
        else:
            return '{} {}, {}, #{}'.format(OP_NAME[op], 
                                          REG_NAME[rx], REG_NAME[ry],
                                          imm5(int(off, 2)))
    elif OP_TYPE[op] == 'j':
        rx = IREG[3:7]
        ry = IREG[7:11]
        return '{} {}, {}'.format(OP_NAME[op], REG_NAME[rx], REG_NAME[ry])
    elif OP_TYPE[op] == 's':
        return "halt"
    else:
        return "This shouldn't be reached"

def print_help():
    print """ Welcome to the simulator text interface. The available commands are:

    r[un] or c[ontinue]             resume execution until next breakpoint
    s[tep]                          execute one instruction
    b[reak] <addr or label>         view and set breakpoints 
                                    ex) 'break'
                                    ex) 'break 0x3'
                                    ex) 'break main'
    print <lowaddr>-<highaddr>      print the values in memory between <lowaddr> and <highaddr>
                                    ex) 'print 0x20-0x30'
    q[uit]                          exit the simulator
    """

def run():
    global HALTED
    while HALTED == False:
        step_instruction()
        if PC in BREAKPOINTS:
            break

def step_instruction():
    global PC
    instruction = access_mem(PC)
    PC += 1
    # decode instruction
    IREG = format(instruction, '016b')
    op = IREG[0:3]
    if OP_NAME[op] == 'add':
        rx = int(IREG[3:7], 2)
        ry = int(IREG[7:11], 2)
        rz = int(IREG[11:15], 2)
        num = imm16(REGS[ry]) + imm16(REGS[rz])
        REGS[rx] = int(format(num if num>=0 else (1<<16) + num, '016b'), 2)
    elif OP_NAME[op] == 'nand':
        rx = int(IREG[3:7], 2)
        ry = int(IREG[7:11], 2)
        rz = int(IREG[11:15], 2)
        num = ~(REGS[ry] & REGS[rz]) 
        REGS[rx] = int(format(num if num>=0 else (1<<16) + num, '016b'), 2)
    elif OP_NAME[op] == 'addi':
        rx = int(IREG[3:7], 2)
        ry = int(IREG[7:11], 2)
        off = imm5(int(IREG[11:], 2))
        num = REGS[ry] + off
        REGS[rx] = int(format(num if num>=0 else (1<<16) + num, '016b'), 2)
    elif OP_NAME[op] == 'lw':
        rx = int(IREG[3:7], 2)
        ry = int(IREG[7:11], 2)
        off = imm5(int(IREG[11:], 2))
        REGS[rx] = access_mem(REGS[ry] + off)
    elif OP_NAME[op] == 'sw':
        rx = int(IREG[3:7], 2)
        ry = int(IREG[7:11], 2)
        off = imm5(int(IREG[11:], 2))
        MEM[REGS[ry]+off] = REGS[rx]
    elif OP_NAME[op] == 'beq':
        rx = int(IREG[3:7], 2)
        ry = int(IREG[7:11], 2)
        off = imm5(int(IREG[11:], 2))
        if REGS[rx] == REGS[ry]:
            PC = PC + off
    elif OP_NAME[op] == 'jalr':
        rx = int(IREG[3:7], 2)
        ry = int(IREG[7:11], 2)
        REGS[ry] = PC
        PC = REGS[rx]
    elif op == '111':
        # undo last pcinc
        PC -= 1
        global HALTED
        HALTED = True

    # make sure zero register remains 0
    REGS[0] = 0

    if PC in BREAKPOINTS:
        print "Breakpoint {} reached: PC = 0x{:X}".format(BREAKPOINTS.index(PC), PC)

def add_breakpoint(bp):
    try:
        bp = int(bp, 0)
    except ValueError:
        if bp in LABELS.values():
            # get PC val key by label value
            bp = LABELS.keys()[LABELS.values().index(bp)]
    if bp not in BREAKPOINTS:
        BREAKPOINTS.append(bp)
    print "[sim] Added breakpoint: PC = 0x{:X}".format(bp)

def print_breakpoints():
    if len(BREAKPOINTS) > 0:
        for i, bp in enumerate(BREAKPOINTS):
            print "[sim] Breakpoint {}: PC = 0x{:X}".format(i, bp)
    else:
        print "[sim] No breakpoints currently enabled"

def print_mem(lowaddr, highaddr):
    for i in xrange(lowaddr, highaddr+1):
        print "[0x{:X}]: 0x{:04X}".format(i, access_mem(i))
    print ""

def prompt_input():
    break_regex = '(break|b)\s*(.+)?'
    mem_regex = 'print (\w+)-(\w+)'

    input_command = raw_input("(sim) ")
    if input_command == 'help':
        print_help()
    elif input_command in ['run', 'r']:
        run()
    elif input_command in ['step', 's']:
        step_instruction()
    elif input_command in ['continue', 'c']:
        run()
    elif input_command in ['quit', 'q']:
        global QUIT
        QUIT = True
    elif re.match(break_regex, input_command):
        try:
            m = re.match(break_regex, input_command)
            if m.group(2):
                bp = m.group(2)
                add_breakpoint(bp)
            print_breakpoints()
        except:
            print "[sim] Error parsing requested breakpoint: {}".format(input_command)
    elif re.match(mem_regex, input_command):
        try:
            m = re.match(mem_regex, input_command)
            lowaddr = int(m.group(1), 0)
            highaddr = int(m.group(2), 0)
            print_mem(lowaddr, highaddr)
        except:
            print "[sim] Error parsing requested print: {}".format(input_command)
    else:
        print "[sim] Unknown command: {}".format(input_command)
        print "[sim] For help on commands and usage, use the 'help' command"

def run_sim():
    global QUIT
    while QUIT == False:
        try:
            print_sim_state()
            prompt_input()
        except KeyboardInterrupt:
            print "[sim] Simulation interrupted."

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('runfile', help=".bin file to be run")
    args = parser.parse_args()
    load_program(args.runfile)
    run_sim()
