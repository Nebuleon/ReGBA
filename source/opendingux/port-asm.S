.global ReGBA_MakeCodeVisible

.set noat
.set noreorder

.ent ReGBA_MakeCodeVisible

# Register assignment:
# $4 = parameter #1: void* Code
# $5 = parameter #2: unsigned int CodeLength
# Clobbered: $1, $4, $5
ReGBA_MakeCodeVisible:
#ifdef MIPS_USE_SYNCI
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
  addiu $sp, $sp, -68
  sw    $ra, 0($sp)
  sw    $3, 4($sp)
  sw    $6, 8($sp)                # preserve the GBA PC register
  sw    $7, 12($sp)
  sw    $8, 16($sp)
  sw    $9, 20($sp)
  sw    $10, 24($sp)
  sw    $11, 28($sp)
  sw    $12, 32($sp)
  sw    $13, 36($sp)
  sw    $14, 40($sp)
  sw    $15, 44($sp)
  sw    $24, 48($sp)
  sw    $25, 52($sp)
  sw    $18, 56($sp)
  sw    $28, 60($sp)
  sw    $30, 64($sp)

  # Pass $4 and $5 through to the call.
  jal   cacheflush
  ori   $6, $0, 3                 # parameter #3: BCACHE (1|2) (branch delay)

  lw    $ra, 0($sp)               # restore the return address first, because
  lw    $3, 4($sp)                # restoring it last woule cause an interlock
  lw    $6, 8($sp)                # $6 = GBA PC, also restore this early
  lw    $7, 12($sp)
  lw    $8, 16($sp)
  lw    $9, 20($sp)
  lw    $10, 24($sp)
  lw    $11, 28($sp)
  lw    $12, 32($sp)
  lw    $13, 36($sp)
  lw    $14, 40($sp)
  lw    $15, 44($sp)
  lw    $24, 48($sp)
  lw    $25, 52($sp)
  lw    $18, 56($sp)
  lw    $28, 60($sp)
  lw    $30, 64($sp)

  jr    $ra                       # return to the caller
  addiu $sp, $sp, 68              # adjust the stack back (branch delay)
#endif

.end ReGBA_MakeCodeVisible
