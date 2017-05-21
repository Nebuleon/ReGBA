.global ReGBA_MakeCodeVisible

.set noat
.set noreorder

.ent ReGBA_MakeCodeVisible

# Register assignment:
# $4 = parameter #1: void* Code
# $5 = parameter #2: unsigned int CodeLength
# Clobbered: $1, $4, $5
ReGBA_MakeCodeVisible:
  beq   $5, $0, 2f
  addu  $5, $4, $5        # $5 is now End, derived from CodeLength (delay)
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
  cache 0x19, ($4)        # Data Writeback (needs to be 0x15, WB/Invalidate,
                          #  on general MIPS processors)
  cache 0x10, ($4)        # Instruction Invalidate
  addiu $4, $4, 32        # go to the next cache line
  sltu  $1, $4, $5        # if Code < End
  bne   $1, $0, 1b        #   goto 1
  nop                     # cannot delay usefully here

  sync                    # guard against memory hazards

2:
  jr    $ra               # return to caller
  nop                     # cannot delay usefully here

.end ReGBA_MakeCodeVisible
