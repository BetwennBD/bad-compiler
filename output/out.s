	.text
	.abicalls
	.option	pic0
	.section	.mdebug.abi32,"",@progbits
	.nan	legacy
	.text
	.file	"source.txt"
	.globl	main
	.p2align	2
	.type	main,@function
	.set	nomicromips
	.set	nomips16
	.ent	main
main:
	.cfi_startproc
	.frame	$sp,0,$ra
	.mask 	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	.set	noat
	addiu	$2, $zero, 0
	slti	$1, $2, 10
	beqz	$1, $BB0_2
	lui	$3, %hi($a)
$BB0_1:
	lw	$1, %lo($a)($3)
	addiu	$1, $1, 1
	sw	$1, %lo($a)($3)
	addiu	$2, $2, 1
	slti	$1, $2, 10
	bnez	$1, $BB0_1
	nop
$BB0_2:
	jr	$ra
	addiu	$2, $zero, 0
	.set	at
	.set	macro
	.set	reorder
	.end	main
$func_end0:
	.size	main, ($func_end0)-main
	.cfi_endproc

	.section	".note.GNU-stack","",@progbits
	.text
