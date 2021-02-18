
	.text

	# Declare main as a global function
	.globl	main
	
# The label 'main' represents the starting point
main:
	# Run the print_string syscall which has code 4
	li	$v0,4		# Code for syscall: print_string
	la	$a0, msg	# Pointer to string (load the address of msg)
	syscall

	li $v0, 5
	syscall
	move $s0, $v0	# $s0 contains the number of points n

	li $a1, 1
	jal input
	move $s3, $s1
	move $s4, $s2

	addi $s0, $s0, -1

	li $s5, 0

	# increment a1
	addi $a1, $a1, 1
	j loop

loop:
	beq $s0, $zero, exit
	jal input
	jal area
	add $s5, $s5, $v0
	addi $s0, $s0, -1
	addi $a1, $a1, 1
	j loop
exit:
	# print area
	li	$v0,4		# Code for syscall: print_string
	la	$a0, area_string	# Pointer to string (load the address of msg)
	syscall

	li $v0, 1
	move $a0, $s5
	syscall
	
	li $v0, 17	# Exit2
	syscall
	
print_newline:
	li $v0, 4
	la $a0, newline
	syscall
	jr $ra

# NOTE: Returns the coordinates in $s1 and $s2 and index of the point in $a1
input:
	move $t4, $ra
	li	$v0,4		# Code for syscall: print_string
	la	$a0, msg_point_x
	syscall	
	# Print index of the point
	li $v0,1
	move $a0, $a1
	syscall
	jal print_newline
	
	# input 1st point
	# x coordinate
	li $v0, 5
	syscall
	move $s1, $v0

	############

	li	$v0,4		# Code for syscall: print_string
	la	$a0, msg_point_y
	syscall
	li $v0,1
	move $a0,$a1
	syscall
	jal print_newline

	# y coordinate
	li $v0, 5
	syscall
	move $s2, $v0
	
	move $ra, $t4
	jr $ra

# takes 4 points x1: $s3, y1: $s4, x2: $s1, y2: $s2 and returns the computed area in $v0
area: 
	sub $t0, $s1, $s3 # (x2 - x1)
	add $t1, $s4, $s2 # (y1 + y2)
	srl $t1, $t1, 1 # (y1+y2)/2
	mul $t3, $t0, $t1
	move $v0, $t3
	
	jr $ra
	

	.data

	# The .asciiz assembler directive creates
	# an ASCII string in memory terminated by
	# the null character. Note that strings are
	# surrounded by double-quotes
msg:	.asciiz	"Please Enter number of points: "
msg_point_x:	.asciiz "Please enter x coordinate of point: "
msg_point_y:	.asciiz "Please enter y coordinate of point: "
newline: .asciiz "\n"
area_string: .asciiz "The area is : "



# TODO
# 1) Fix floating points
# 2) Keep sanity and input checks
# 3) Reftacor code (use stack)
# 4) Write Tests