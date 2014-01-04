	.global	syscall4
syscall4:
	movl	16(%esp), %edx
	movl	12(%esp), %ecx
	movl	8(%esp), %ebx
	movl	4(%esp), %eax
	int		$0x80
	movl	$0, %eax
	leave
	ret

	.global	syscall2
syscall2:
	pop		%ebx
	pop		%eax
	int		$0x80
	ret
