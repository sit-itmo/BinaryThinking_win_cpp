	.file	"hello.c"
	.text
	.globl	mega_table
	.bss
	.align 32
	.type	mega_table, @object
	.size	mega_table, 400
mega_table:
	.zero	400
	.globl	table
	.data
	.align 32
	.type	table, @object
	.size	table, 320
table:
	.long	1
	.long	170
	.long	2
	.long	187
	.long	3
	.long	204
	.long	4
	.long	221
	.long	5
	.long	238
	.long	6
	.long	255
	.long	7
	.long	7
	.long	8
	.long	0
	.long	1
	.long	170
	.long	2
	.long	187
	.long	3
	.long	204
	.long	4
	.long	221
	.long	5
	.long	238
	.long	6
	.long	255
	.long	7
	.long	7
	.long	8
	.long	0
	.long	1
	.long	170
	.long	2
	.long	187
	.long	3
	.long	204
	.long	4
	.long	221
	.long	5
	.long	238
	.long	6
	.long	255
	.long	7
	.long	7
	.long	8
	.long	0
	.long	1
	.long	170
	.long	2
	.long	187
	.long	3
	.long	204
	.long	4
	.long	221
	.long	5
	.long	238
	.long	6
	.long	255
	.long	7
	.long	7
	.long	8
	.long	0
	.long	1
	.long	170
	.long	2
	.long	187
	.long	3
	.long	204
	.long	4
	.long	221
	.long	5
	.long	238
	.long	6
	.long	255
	.long	7
	.long	7
	.long	8
	.long	0
	.section	.rodata
.LC0:
	.string	"\nHello World %d!\n"
	.text
	.globl	test
	.type	test, @function
test:
.LFB0:
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	movl	$10, %esi
	leaq	.LC0(%rip), %rax
	movq	%rax, %rdi
	movl	$0, %eax
	call	printf@PLT
	movl	$0, %eax
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE0:
	.size	test, .-test
	.section	.rodata
.LC1:
	.string	"\nHello World!"
	.text
	.globl	main
	.type	main, @function
main:
.LFB1:
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	leaq	.LC1(%rip), %rax
	movq	%rax, %rdi
	call	puts@PLT
	movl	$7, %edi
	call	putchar@PLT
	movl	$7, %edi
	call	putchar@PLT
	movl	$0, %eax
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE1:
	.size	main, .-main
	.ident	"GCC: (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0"
	.section	.note.GNU-stack,"",@progbits
	.section	.note.gnu.property,"a"
	.align 8
	.long	1f - 0f
	.long	4f - 1f
	.long	5
0:
	.string	"GNU"
1:
	.align 8
	.long	0xc0000002
	.long	3f - 2f
2:
	.long	0x3
3:
	.align 8
4:
