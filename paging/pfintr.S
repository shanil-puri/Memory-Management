/* page fault interrupt service routine */

.text
errorcode: .long 0
.globl pfintr,errorcode
pfintr:
	popl errorcode
	pushfl
	cli
	pushal
	call pfint	/* calls interrupt routine */
	popal
	popfl
	iret
