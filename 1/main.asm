
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

	li $s6, 1	# s6 stores the index of the point
	move $a0, $s6
	jal input
	
	# store x1, y1 in s1 and s2
	move $s1, $v0
	move $s2, $v1

	addi $s0, $s0, -1

#	li $s5, 0
	mtc1 $zero, $f12

	# increment s6
	addi $s6, $s6, 1
	j loop

loop:
	beq $s0, $zero, exit
	
	move $a0, $s6
	jal input

	# store x2, y2 in s3, s4
	move $s3, $v0
	move $s4, $v1

	jal area
	add.s $f12, $f12, $f0
	addi $s0, $s0, -1
	addi $s6, $s6, 1

	move $s1, $s3
	move $s2, $s4
	j loop
exit:
	# print area
	li	$v0,4		# Code for syscall: print_string
	la	$a0, area_string	# Pointer to string (load the address of msg)
	syscall

	li $v0, 2
	# move $a0, $s5
	syscall
	
	li $v0, 17	# Exit2
	syscall
	
print_newline:
	li $v0, 4
	la $a0, newline
	syscall
	jr $ra

# Returns the coordinates in $v0 and $v1 and takes index of the point in $a0
input:
	move $t0, $a0	# t0 stores the index of the input
	move $t1, $ra	# t1 stores return address
	li	$v0,4		# Code for syscall: print_string
	la	$a0, msg_point_x
	syscall	
	# Print index of the point
	li $v0,1
	move $a0, $t0
	syscall
	jal print_newline
	
	# input 1st point
	# x coordinate
	li $v0, 5
	syscall
	move $t2, $v0	# t2 stores the x coordinate

	############

	li	$v0,4		# Code for syscall: print_string
	la	$a0, msg_point_y
	syscall
	li $v0,1
	move $a0,$t0
	syscall
	jal print_newline

	# y coordinate
	li $v0, 5
	syscall
	move $t3, $v0 # t3 stores the y coordinate
	
	move $v0, $t2 # store values of x and y coordinates for return
	move $v1, $t3

	move $ra, $t1
	jr $ra

# takes 4 points x1: $s1, y1: $s2, x2: $s3, y2: $s4 and returns the computed area in $f0
# uses f2, f4, f6 for internal computation
area: 
	sub $t0, $s3, $s1 # (x2 - x1)
	add $t1, $s2, $s4 # (y1 + y2)

	mtc1 $t0, $f0
	cvt.s.w $f0, $f0

	mtc1 $t1, $f2
	cvt.s.w $f2, $f2

	li $t5, 2
	mtc1 $t5, $f4
	cvt.s.w $f4, $f4

#	 srl $t1, $t1, 1 # (y1+y2)/2
	mul.s $f6, $f0, $f2
	div.s $f6, $f6, $f4

	mov.s $f0, $f6
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
# 2) Keep sanity and input checks
# 4) Write Tests