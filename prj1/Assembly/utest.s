!Tuan Nguyen

! the answer should be 4 by adding 2 and 2
la $t0, main
jalr $t0, $ra


number1:
.word 2
number2:
.word 2
stack:
.word 0xF000
 
main:
la $sp, stack
lw $sp, 0x0($sp)
la $a0, number1
lw $a0, 0x0($a0)
la $a1, number2
lw $a1, 0x0($a0)
addi $sp, $sp, -1
sw $a0, 0x0($sp)
addi $sp, $sp, -1
sw $ra, 0x0($sp)
la $t0, multiply
jalr $t0, $ra
halt


multiply:

addi $sp, $sp, -1
sw $s2, 0x0($sp)

addi $s2, $sp, 0

addi $sp, $sp, -1
add $v0, $a0, $a1

lw $v0, 0x4($s2) 

jalr $ra, $v0


