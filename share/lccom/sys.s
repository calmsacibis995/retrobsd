	.text

#
# int open( char* file, int flags, int mode )
#
open:	

	# errno handling code after syscall is not ideal,
	# but I don't think the assembler handles specifying
	# relocations for %hi and %lo yet, so the handling
	# shown in the retrobsd code is not really doable
	
	syscall	5
	nop
	j	serrn
	nop

	jr	$ra
	nop

#
# int read( int fd, void* dest, int count)
# returns: count of chars read or -1 if error (see errno)
#
read:

	# errno handling code after syscall is not ideal,
	# but I don't think the assembler handles specifying
	# relocations for %hi and %lo yet, so the handling
	# shown in the retrobsd code is not really doable
	
	syscall	3
	nop
	j	serrn
	nop

	jr	$ra
	nop



#
# int write( int fd, void* string, int count );
# returns: count of chars written or -1 if error (see errno)
#
write:	
	# errno handling code after syscall is not ideal,
	# but I don't think the assembler handles specifying
	# relocations for %hi and %lo yet, so the handling
	# shown in the retrobsd code is not really doable
	
	syscall	4
	nop
	j	serrn
	nop

	jr	$ra
	nop

#
# int close( int fd );
#
close:
	# errno handling code after syscall is not ideal,
	# but I don't think the assembler handles specifying
	# relocations for %hi and %lo yet, so the handling
	# shown in the retrobsd code is not really doable
	
	syscall	6
	nop
	j	serrn
	nop

	jr	$ra
	nop

#
# exit( int n );
#
exit:
	syscall	1
	nop


serrn:
	la	$t1, _errno
	sw	$t0, 0($t1)
	jr	$ra
	nop

	.data
_errno:	.byte	0,0,0,0

	.globl open
	.globl read
	.globl write
	.globl close
	.globl exit
        .globl _errno
        
