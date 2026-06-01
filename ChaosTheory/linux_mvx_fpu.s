/* MSVC x87 math intrinsics used by mvxPlayerLib (FPU-register calling convention). */
	.text

/* long long __ftol(double in st(0))  -> truncate toward zero, result in edx:eax */
	.globl __ftol
__ftol:
	push   %ebp
	mov    %esp, %ebp
	sub    $20, %esp
	fnstcw -2(%ebp)
	movzwl -2(%ebp), %eax
	or     $0x0C00, %eax        /* RC = 11 (truncate toward zero) */
	mov    %ax, -4(%ebp)
	fldcw  -4(%ebp)
	fistpll -12(%ebp)
	fldcw  -2(%ebp)
	mov    -12(%ebp), %eax
	mov    -8(%ebp), %edx
	leave
	ret

/* double __CIpow(x=st(1), y=st(0)) -> x^y in st(0), pops both */
	.globl __CIpow
__CIpow:
	sub    $16, %esp
	fstpl  8(%esp)              /* [esp+8] = y (old st0) */
	fstpl  (%esp)              /* [esp]   = x (old st1) */
	call   pow
	add    $16, %esp
	ret

/* double __CIfmod(x=st(1), y=st(0)) -> fmod(x,y) in st(0), pops both */
	.globl __CIfmod
__CIfmod:
	sub    $16, %esp
	fstpl  8(%esp)
	fstpl  (%esp)
	call   fmod
	add    $16, %esp
	ret

	.section .note.GNU-stack,"",@progbits
