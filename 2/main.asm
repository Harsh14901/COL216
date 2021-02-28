
	.text

	# Declare main as a global function
	.globl	main
	
# The label 'main' represents the starting point
main:
	li $v0, 4
	la $a0, msg
	syscall

	jal input
	la $s0, input_str	# s0 is the pointer to string

	j loop

loop:
	# s0 is the pointer to the current character
	lb $s1, 0($s0) # s1 is the current character
	lw $a0, 0($sp)
	li $t0, 10	# 0x0a is code for newline
	beq $s1, $zero, exit
	beq $s1, $t0, exit
	
	# check if character is an operand
	move $a0, $s1
	jal is_operand
	bne $v0, $zero, push_character

	j not_operand

not_operand:
	# check if character is an operator
	jal is_operator
	beq $v0, $zero, exit
	
	jal evaluate_operand

	move $a0, $v0
	jal push 

	j after_push

push_character:
	addi $a0, $a0, -48
	jal push
	j after_push

after_push:
	addi $s0, $s0, 1
	j loop

evaluate_operand:
	move $t0, $ra
	
	jal pop
	move $t2, $v0
	jal pop
	move $t1, $v0


	li $t3, 0

	beq $a0, 42, multiply
	beq $a0, 43, addition
	beq $a0, 45, subtract

return_operand:
	move $v0, $t3
	move $ra, $t0
	jr $ra

addition:
	add $t3, $t1, $t2
	j return_operand
	
subtract:
	sub $t3, $t1, $t2
	j return_operand

multiply:
	mul $t3, $t1, $t2
	j return_operand

abort:


exit:
	move $t0, $a0
	li	$v0,4		# Code for syscall: print_string
	la	$a0, value	# Pointer to string (load the address of msg)
	syscall

	move $a0, $t0
	li $v0, 1
	syscall	# prints contents of a0
		
	li $v0, 17	# Exit2
	move $a0, $zero
	syscall
	
print_newline:
	li $v0, 4
	la $a0, newline
	syscall
	jr $ra

input:
	li $v0 , 8
	la $a0 , input_str
	li $a1, 1024
	syscall

	jr $ra

push:
	subu $sp, $sp, 4
	sw $a0, ($sp)
	jr $ra

pop:
	lw $v0, ($sp)
	addu $sp, $sp, 4
	jr $ra

is_operand:
	li $t1, 48
	li $t2, 57
	blt $a0, $t1, return_false
	bgt $a0, $t2, return_false
	j return_true

return_false:
	li $v0, 0
	jr $ra
return_true:
	li $v0, 1
	jr $ra

is_operator:
	beq $a0, 42, return_true
	beq $a0, 43, return_true
	beq $a0, 45, return_true
	j return_false

.data

msg:	.asciiz	"Enter the postfix expression: \n"
newline: .asciiz "\n"
value: .asciiz "The value is : "
input_str: .space 1024
abort_msg: .asciiz "Fatal Error: cannot recover"
