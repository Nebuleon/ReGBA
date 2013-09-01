.global ReGBA_MakeCodeVisible

.set noat
.set noreorder

.ent ReGBA_MakeCodeVisible

# Register assignment:
# $4 = parameter #1: void* Code
# $5 = parameter #2: unsigned int CodeLength
# Clobbered: $1, $4, $5
ReGBA_MakeCodeVisible:
#ifdef MIPS_32R2
  beq   $5, $0, 2f
  add   $5, $4, $5        # $5 is now End, derived from CodeLength (delay)
  srl   $4, $4, 5         # Round $4 down to the cache line containing it
  sll   $4, $4, 5         #   continue

  # The rounding down was necessary above in order to allow a partial
  # cache line near the end of the code to be flushed. Otherwise, it may
  # have been ignored after flushing one cache line's worth of bytes.
  # Consider:
  # Cache line 1 |                [flushflushflushf]
  # Cache line 2 | lushflushflu
  # Cache line 2 does not start a line to be flushed, so it isn't flushed.
  # The rounding down is required to make it this instead:
  # Cache line 1 |[padpadpadpadpad flushglushflushf]
  # Cache line 2 |[lushflushflu                    ]
  
  # Now git 'er done.
1:
.set mips32r2
  synci ($4)              # Combined Data Writeback-Instruction Invalidate (R2)
.set mips0
  addiu $4, $4, 32        # go to the next cache line
  sltu  $1, $4, $5        # if Code < End
  bne   $1, $0, 1b        #   goto 1
  nop                     # cannot delay usefully here

2:
  jr    $ra               # return to caller
  nop                     # cannot delay usefully here
#else
  # Issue the cacheflush kernel call. Preserve registers first.
  addiu $sp, $sp, -60
  sw    $3, 0($sp)
  sw    $7, 4($sp)
  sw    $8, 8($sp)
  sw    $9, 12($sp)
  sw    $10, 16($sp)
  sw    $11, 20($sp)
  sw    $12, 24($sp)
  sw    $13, 28($sp)
  sw    $14, 32($sp)
  sw    $15, 36($sp)
  sw    $24, 40($sp)
  sw    $25, 44($sp)
  sw    $18, 48($sp)
  sw    $28, 52($sp)
  sw    $30, 56($sp)

  # Pass $4 and $5 through to the call.
  jal   cacheflush
  ori   $6, $0, 3                 # parameter #3: BCACHE (1|2) (branch delay)

  lw    $3, 0($sp)
  lw    $7, 4($sp)
  lw    $8, 8($sp)
  lw    $9, 12($sp)
  lw    $10, 16($sp)
  lw    $11, 20($sp)
  lw    $12, 24($sp)
  lw    $13, 28($sp)
  lw    $14, 32($sp)
  lw    $15, 36($sp)
  lw    $24, 40($sp)
  lw    $25, 44($sp)
  lw    $18, 48($sp)
  lw    $28, 52($sp)
  lw    $30, 56($sp)

  jr    $ra                       # return to the caller
  addiu $sp, $sp, 60              # adjust the stack back (branch delay)
#endif

.end ReGBA_MakeCodeVisible
