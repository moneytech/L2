.global getb
.global get1b
.global get2b
.global get4b
.global get8b
.global get
.global setb
.global set1b
.global set2b
.global set4b
.global set8b
.global set
.global add
.global subtract
.global multiply
.global divide
.global rem
.global equals
.global greaterthan
.global lesserthan
.global leftshift
.global rightshift
.global land
.global lor
.global lnot
.global allocate
.global syscall

.text
jmp l2rt_end

get:
get8b:
movq 0(%rdi), %rax
ret

getb:
get1b:
xorq %rax, %rax
movb 0(%rdi), %al
ret

get2b:
xorq %rax, %rax
movw 0(%rdi), %ax
ret

get4b:
xorq %rax, %rax
movl 0(%rdi), %eax
ret

set:
set8b:
movq %rsi, 0(%rdi)
ret

setb:
set1b:
movq %rsi, %rax
movb %al, 0(%rdi)
ret

set2b:
movq %rsi, %rax
movw %ax, 0(%rdi)
ret

set4b:
movq %rsi, %rax
movl %eax, 0(%rdi)
ret

add:
movq %rdi, %rax
addq %rsi, %rax
ret

subtract:
movq %rdi, %rax
subq %rsi, %rax
ret

multiply:
movq %rdi, %rax
mulq %rsi
ret

divide:
xorq %rdx, %rdx
movq %rdi, %rax
divq %rsi
ret

rem:
xorq %rdx, %rdx
movq %rdi, %rax
divq %rsi
movq %rdx, %rax
ret

equals:
xorq %rax, %rax
subq %rsi, %rdi
setz %al
negq %rax
ret

greaterthan:
xorq %rax, %rax
subq %rdi, %rsi
setc %al
negq %rax
ret

lesserthan:
xorq %rax, %rax
subq %rsi, %rdi
setc %al
negq %rax
ret

leftshift:
movq %rdi, %rax
movq %rsi, %rcx
shl %cl, %rax
ret

rightshift:
movq %rdi, %rax
movq %rsi, %rcx
shr %cl, %rax
ret

land:
movq %rdi, %rax
andq %rsi, %rax
ret

lor:
movq %rdi, %rax
orq %rsi, %rax
ret

lnot:
movq %rdi, %rax
notq %rax
ret

allocate:
/* All sanctioned by L2 ABI: */
movq 48(%rsi), %r15
movq 40(%rsi), %r12
movq 32(%rsi), %rbx
movq 24(%rsi), %r13
movq 16(%rsi), %r14
movq 0(%rsi), %rbp
subq %rdi, %rsp
andq $0xFFFFFFFFFFFFFFF8, %rsp
movq %rsp, 56(%rsi)
jmp *8(%rsi)

/*
 * Do syscall with the syscall number being provided by the first argument to
 * this function and its six arguments being provided by arguments 2 to 7 of
 * this function.
 */
syscall:
	movq %rdi, %rax
	movq %rsi, %rdi
	movq %rdx, %rsi
	movq %rcx, %rdx
	movq %r8, %r10
	movq %r9, %r8
	movq 8(%rsp), %r9
	syscall
	ret

l2rt_end:
ret
